// Copyright 2024 Google LLC.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "tensorflow/lite/experimental/lrt/vendors/qualcomm/dispatch/litert_dispatch_invocation_context.h"

#include <cstddef>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "third_party/qairt/latest/include/QNN/QnnCommon.h"
#include "third_party/qairt/latest/include/QNN/QnnContext.h"
#include "third_party/qairt/latest/include/QNN/QnnInterface.h"
#include "third_party/qairt/latest/include/QNN/QnnTypes.h"
#include "tensorflow/lite/experimental/lrt/c/litert_common.h"
#include "tensorflow/lite/experimental/lrt/c/litert_model.h"
#include "tensorflow/lite/experimental/lrt/c/litert_support.h"
#include "tensorflow/lite/experimental/lrt/c/litert_tensor_buffer.h"
#include "tensorflow/lite/experimental/lrt/c/litert_tensor_buffer_requirements.h"
#include "tensorflow/lite/experimental/lrt/core/utils.h"
#include "tensorflow/lite/experimental/lrt/vendors/c/litert_dispatch.h"
#include "tensorflow/lite/experimental/lrt/vendors/qualcomm/context_binary_info.h"
#include "tensorflow/lite/experimental/lrt/vendors/qualcomm/dispatch/litert_dispatch_device_context.h"
#include "tensorflow/lite/experimental/lrt/vendors/qualcomm/qnn_manager.h"

using ::litert::qnn::QnnManager;
using ::litert::qnn::config::GetDefaultContextConfigs;

LiteRtDispatchInvocationContextT::LiteRtDispatchInvocationContextT(
    litert::qnn::QnnManager& qnn_manager,
    const litert::qnn::ContextBinaryInfo& context_binary_info,
    LiteRtDispatchDeviceContextT& device_context,
    Qnn_ProfileHandle_t profile_handle, int graph_index,
    Qnn_GraphHandle_t graph_handle)
    : qnn_manager_(qnn_manager),
      device_context_(device_context),
      profile_handle_(profile_handle),
      graph_index_(graph_index),
      graph_handle_(graph_handle),
      inputs_(context_binary_info.Graphs()[graph_index].Inputs()),
      outputs_(context_binary_info.Graphs()[graph_index].Outputs()) {}

absl::StatusOr<LiteRtDispatchInvocationContextT::Ptr>
LiteRtDispatchInvocationContextT::Create(
    QnnManager& qnn, LiteRtDispatchDeviceContextT& device_context,
    const void* exec_bytecode_ptr, size_t exec_bytecode_size,
    const char* function_name) {
  auto context_binary_info = litert::qnn::ContextBinaryInfo::Create(
      qnn, exec_bytecode_ptr, exec_bytecode_size);
  if (!context_binary_info.ok()) {
    return context_binary_info.status();
  }

  int graph_index = -1;
  const auto& graphs = context_binary_info->Graphs();
  for (auto i = 0; i < graphs.size(); ++i) {
    const auto& graph = graphs[i];
    if (graph.Name() == absl::string_view(function_name)) {
      graph_index = i;
      break;
    }
  }
  if (graph_index < 0) {
    return absl::InternalError("Function name not found");
  }

  Qnn_ProfileHandle_t profile_handle = nullptr;
  if (auto status = qnn.Api()->contextCreateFromBinary(
          qnn.BackendHandle(), /*deviceHandle*/ nullptr,
          GetDefaultContextConfigs().data(), exec_bytecode_ptr,
          exec_bytecode_size, &qnn.ContextHandle(), profile_handle);
      status != QNN_SUCCESS) {
    return absl::InternalError("Failed to create context from context binary");
  }

  Qnn_GraphHandle_t graph_handle;
  if (auto status = qnn.Api()->graphRetrieve(qnn.ContextHandle(), function_name,
                                             &graph_handle);
      status != QNN_SUCCESS) {
    return absl::InternalError("Failed to retrieve graph");
  }

  return Ptr(new LiteRtDispatchInvocationContextT(
      qnn, std::move(*context_binary_info), device_context, profile_handle,
      graph_index, graph_handle));
}

namespace {

absl::StatusOr<LiteRtTensorBufferRequirements> GetTensorBufferRequirements(
    const LiteRtRankedTensorType& tensor_type) {
  auto* tensor_strides = tensor_type.layout.strides;
  if (tensor_strides != nullptr) {
    return absl::InternalError("Tensor strides are not supported by QNN");
  }

  static constexpr LiteRtTensorBufferType supported_tensor_buffer_types[] = {
      kLiteRtTensorBufferTypeFastRpc,
  };
  int num_supported_tensor_buffer_types =
      sizeof(supported_tensor_buffer_types) /
      sizeof(supported_tensor_buffer_types[0]);

  auto buffer_size = litert::internal::GetNumPackedBytes(tensor_type);
  if (!buffer_size.ok()) {
    return buffer_size.status();
  }

  LiteRtTensorBufferRequirements requirements;
  if (auto status = LiteRtCreateTensorBufferRequirements(
          num_supported_tensor_buffer_types, supported_tensor_buffer_types,
          *buffer_size, &requirements);
      status != kLiteRtStatusOk) {
    return absl::InternalError("Not implemented");
  }

  return requirements;
}

}  // namespace

absl::StatusOr<LiteRtTensorBufferRequirements>
LiteRtDispatchInvocationContextT::GetInputRequirements(
    int input_index, const LiteRtRankedTensorType& tensor_type) {
  return GetTensorBufferRequirements(tensor_type);
}

absl::StatusOr<LiteRtTensorBufferRequirements>
LiteRtDispatchInvocationContextT::GetOutputRequirements(
    int output_index, const LiteRtRankedTensorType& tensor_type) {
  return GetTensorBufferRequirements(tensor_type);
}

absl::Status LiteRtDispatchInvocationContextT::AttachInput(
    int graph_input_index, LiteRtTensorBufferHandle tensor_buffer_handle) {
  if (graph_input_index < 0 || graph_input_index >= inputs_.size()) {
    return absl::InternalError("Invalid graph_input_index");
  }

  auto& tensor = inputs_[graph_input_index];
  return AttachBuffer(tensor.Tensor(), tensor_buffer_handle);
}

absl::Status LiteRtDispatchInvocationContextT::AttachOutput(
    int graph_output_index, LiteRtTensorBufferHandle tensor_buffer_handle) {
  if (graph_output_index < 0 || graph_output_index >= outputs_.size()) {
    return absl::InternalError("Invalid graph_output_index");
  }

  auto& tensor = outputs_[graph_output_index];
  return AttachBuffer(tensor.Tensor(), tensor_buffer_handle);
}

absl::Status LiteRtDispatchInvocationContextT::AttachBuffer(
    Qnn_Tensor_t& tensor, LiteRtTensorBufferHandle tensor_buffer_handle) {
  auto tensor_buffer = device_context_.GetTensorBuffer(tensor_buffer_handle);
  if (!tensor_buffer.ok()) {
    return tensor_buffer.status();
  }

  auto mem_handle = device_context_.GetMemHandle(tensor_buffer_handle, tensor);
  if (!mem_handle.ok()) {
    return mem_handle.status();
  }

  if (tensor.version == QNN_TENSOR_VERSION_1) {
    tensor.v1.memType = QNN_TENSORMEMTYPE_MEMHANDLE;
    tensor.v1.memHandle = *mem_handle;

  } else if (tensor.version == QNN_TENSOR_VERSION_2) {
    if (tensor.v2.isDynamicDimensions != nullptr) {
      return absl::InternalError("Dynamic dimensions not yet supported");
    }
    tensor.v2.memType = QNN_TENSORMEMTYPE_MEMHANDLE;
    tensor.v2.memHandle = *mem_handle;

  } else {
    return absl::InternalError("Unsupported QNN tensor version");
  }

  return {};
}

absl::Status LiteRtDispatchInvocationContextT::Execute() {
  const size_t num_ins = inputs_.size();
  LITERT_STACK_ARRAY(Qnn_Tensor_t, inputs, num_ins, QNN_TENSOR_INIT);
  for (size_t i = 0; i < num_ins; ++i) {
    *(inputs + i) = inputs_.at(i).Tensor();
  }

  const size_t num_outs = outputs_.size();
  LITERT_STACK_ARRAY(Qnn_Tensor_t, outputs, num_outs, QNN_TENSOR_INIT);
  for (size_t i = 0; i < num_outs; ++i) {
    *(outputs + i) = outputs_.at(i).Tensor();
  }

  if (auto status = qnn_manager_.Api()->graphExecute(
          graph_handle_, inputs, num_ins, outputs, num_outs,
          /*profileHandle=*/nullptr, /*signalHandle=*/nullptr);
      status != QNN_SUCCESS) {
    return absl::InternalError("Failed to execute graph");
  }

  return {};
}
