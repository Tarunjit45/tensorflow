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

#include "tensorflow/lite/experimental/lrt/vendors/qualcomm/dispatch/litert_dispatch_device_context.h"

#include <cstddef>
#include <cstdint>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "third_party/qairt/latest/include/QNN/HTP/QnnHtpMem.h"
#include "third_party/qairt/latest/include/QNN/QnnBackend.h"
#include "third_party/qairt/latest/include/QNN/QnnCommon.h"
#include "third_party/qairt/latest/include/QNN/QnnInterface.h"
#include "third_party/qairt/latest/include/QNN/QnnMem.h"
#include "third_party/qairt/latest/include/QNN/QnnTypes.h"
#include "tensorflow/lite/experimental/lrt/c/litert_common.h"
#include "tensorflow/lite/experimental/lrt/c/litert_model.h"
#include "tensorflow/lite/experimental/lrt/c/litert_tensor_buffer.h"
#include "tensorflow/lite/experimental/lrt/vendors/c/litert_dispatch.h"
#include "tensorflow/lite/experimental/lrt/vendors/qualcomm/common.h"
#include "tensorflow/lite/experimental/lrt/vendors/qualcomm/qnn_manager.h"

using ::litert::qnn::QnnManager;

absl::StatusOr<LiteRtDispatchDeviceContextT::Ptr>
LiteRtDispatchDeviceContextT::Create(QnnManager& qnn) {
  return Ptr(new LiteRtDispatchDeviceContextT(qnn));
}

absl::StatusOr<LiteRtTensorBuffer>
LiteRtDispatchDeviceContextT::GetTensorBuffer(
    LiteRtTensorBufferHandle tensor_buffer_handle) {
  auto registry_entry = tensor_buffer_registry_.Get(tensor_buffer_handle);
  if (!registry_entry.ok()) {
    return registry_entry.status();
  }

  return (*registry_entry)->tensor_buffer;
}

absl::StatusOr<Qnn_MemHandle_t> LiteRtDispatchDeviceContextT::GetMemHandle(
    LiteRtTensorBufferHandle tensor_buffer_handle, const Qnn_Tensor_t& tensor) {
  auto registry_entry = tensor_buffer_registry_.Get(tensor_buffer_handle);
  if (!registry_entry.ok()) {
    return registry_entry.status();
  }

  if (!(*registry_entry)->qnn_mem_handle) {
    auto qnn_mem_handle =
        RegisterTensorBuffer((*registry_entry)->tensor_buffer, tensor);
    if (!qnn_mem_handle.ok()) {
      return qnn_mem_handle.status();
    }
    (*registry_entry)->qnn_mem_handle = *qnn_mem_handle;
  }

  return (*registry_entry)->qnn_mem_handle;
}

absl::StatusOr<Qnn_MemHandle_t>
LiteRtDispatchDeviceContextT::RegisterTensorBuffer(
    LiteRtTensorBuffer tensor_buffer, const Qnn_Tensor_t& tensor) {
  LiteRtTensorBufferType tensor_buffer_type;
  if (auto status =
          LiteRtGetTensorBufferType(tensor_buffer, &tensor_buffer_type);
      status != kLiteRtStatusOk) {
    return absl::InternalError("Failed to get tensor buffer type");
  }

  if (tensor_buffer_type != kLiteRtTensorBufferTypeFastRpc) {
    return absl::InternalError("Unsupported tensor buffer type");
  }

  size_t tensor_buffer_size;
  if (auto status =
          LiteRtGetTensorBufferSize(tensor_buffer, &tensor_buffer_size);
      status != kLiteRtStatusOk) {
    return absl::InternalError("Failed to get tensor buffer size");
  }

  size_t tensor_buffer_offset;
  if (auto status =
          LiteRtGetTensorBufferOffset(tensor_buffer, &tensor_buffer_offset);
      status != kLiteRtStatusOk) {
    return absl::InternalError("Failed to get tensor buffer offset");
  }

  LiteRtRankedTensorType tensor_type;
  if (auto status =
          LiteRtGetTensorBufferTensorType(tensor_buffer, &tensor_type);
      status != kLiteRtStatusOk) {
    return absl::InternalError("Failed to get tensor buffer's type");
  }

  Qnn_DataType_t tensor_data_type;
  if (kLiteRtStatusOk !=
      LegalizeElementType(tensor_type.element_type, &tensor_data_type)) {
    return absl::InternalError("Failed to legalize datatype");
  }

  uint32_t tensor_rank = tensor_type.layout.rank;
  uint32_t* tensor_dimensions = reinterpret_cast<uint32_t*>(
      const_cast<int32_t*>(tensor_type.layout.dimensions));
  auto* tensor_strides = tensor_type.layout.strides;
  if (tensor_strides != nullptr) {
    return absl::InternalError("Tensor strides are not supported by QNN");
  }

  void* fastrpc_buffer_addr;
  int fastrpc_buffer_fd;
#if LITERT_HAS_FASTRPC_SUPPORT
  if (auto status = LiteRtGetTensorBufferFastRpcBuffer(
          tensor_buffer, &fastrpc_buffer_addr, &fastrpc_buffer_fd);
      status != kLiteRtStatusOk) {
    return absl::InternalError("Failed to get FastRPC buffer");
  }
#else
  (void)fastrpc_buffer_addr;
  (void)fastrpc_buffer_fd;
  return absl::InternalError("FastRPC support is missing on this platform");
#endif  // LITERT_HAS_FASTRPC_SUPPORT

  QnnMemHtp_Descriptor_t mem_htp_descriptor = {};
  mem_htp_descriptor.type = QNN_HTP_MEM_SHARED_BUFFER;
  mem_htp_descriptor.size = tensor_buffer_size;
  mem_htp_descriptor.sharedBufferConfig = {fastrpc_buffer_fd,
                                           tensor_buffer_offset};

  Qnn_MemDescriptor_t mem_descriptor = {};
  mem_descriptor.memShape = {tensor_rank, tensor_dimensions, nullptr};
  mem_descriptor.dataType = tensor_data_type;
  mem_descriptor.memType = QNN_MEM_TYPE_CUSTOM;
  mem_descriptor.customInfo = &mem_htp_descriptor;

  Qnn_MemHandle_t mem_handle = nullptr;
  if (auto status = qnn_manager_.Api()->memRegister(
          qnn_manager_.ContextHandle(), &mem_descriptor, 1UL, &mem_handle);
      status != QNN_SUCCESS) {
    return absl::InternalError("Failed to register tensor buffer");
  }

  if (!mem_handle) {
    return absl::InternalError("Failed to register buffer: null mem_handle");
  }

  return mem_handle;
}
