# Copyright 2024 Google LLC.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

package(
    # copybara:uncomment default_applicable_licenses = ["//tensorflow:license"],
    default_visibility = ["//tensorflow/lite/experimental/lrt:__subpackages__"],
)

cc_library(
    name = "api_internal",
    srcs = ["litert_common.cc"],
    hdrs = [
        "//tensorflow/lite/experimental/lrt/c:litert_common.h",
        "//tensorflow/lite/experimental/lrt/c:litert_model.h",
        "//tensorflow/lite/experimental/lrt/c:litert_op_code.h",
        "//tensorflow/lite/experimental/lrt/c:litert_options.h",
        "//tensorflow/lite/experimental/lrt/c:litert_support.h",
        "//tensorflow/lite/experimental/lrt/cc:litert_support.h",
        "//tensorflow/lite/experimental/lrt/vendors/c:litert_compiler_plugin.h",
    ],
    deps = [
        "//tensorflow/lite:builtin_ops",
        "//tensorflow/lite/core/c:c_api_types",
        "@com_google_absl//absl/types:span",
    ],
)

cc_library(
    name = "model",
    srcs = [
        "model.cc",
    ],
    hdrs = [
        "model.h",
    ],
    deps = [
        ":api_internal",
        "//tensorflow/compiler/mlir/lite/core:model_builder_base",
        "//tensorflow/lite/c:c_api_types",
        "//tensorflow/lite/core/c:c_api_types",
        "//tensorflow/lite/schema:schema_fbs",
        "@com_google_absl//absl/strings:string_view",
        "@com_google_absl//absl/types:span",
    ],
)

cc_library(
    name = "option",
    srcs = ["option.cc"],
    hdrs = [
        "model.h",
    ],
    deps = [
        ":api_internal",
        "//tensorflow/compiler/mlir/lite/core:model_builder_base",
        "//tensorflow/lite/c:c_api_types",
        "//tensorflow/lite/core/c:c_api_types",
        "//tensorflow/lite/schema:schema_fbs",
        "@com_google_absl//absl/strings:string_view",
        "@com_google_absl//absl/types:span",
    ],
)

cc_library(
    name = "litert_model_init",
    srcs = ["litert_model_init.cc"],
    hdrs = ["litert_model_init.h"],
    deps = [
        ":api_internal",
        ":model",
        ":option",
        "//tensorflow/compiler/mlir/lite/core:model_builder_base",
        "//tensorflow/lite:allocation",
        "//tensorflow/lite:framework",
        "//tensorflow/lite:stderr_reporter",
        "//tensorflow/lite/c:c_api_types",
        "//tensorflow/lite/schema:schema_fbs",
        "@com_google_absl//absl/log:check",
        "@flatbuffers//:runtime_cc",
    ],
)

cc_test(
    name = "model_test",
    srcs = ["model_test.cc"],
    data = [
        "//tensorflow/lite/experimental/lrt/test:tflite_test_data",
    ],
    tags = ["no_oss"],
    deps = [
        ":api_internal",
        ":graph_tools",
        ":litert_model_init",
        "//tensorflow/lite/experimental/lrt/test:common",
        "//tensorflow/lite/schema:schema_fbs",
        "@com_google_googletest//:gtest_main",
        "@llvm-project//llvm:Support",
    ],
)

cc_test(
    name = "option_test",
    srcs = ["option_test.cc"],
    data = [
        "//tensorflow/lite/experimental/lrt/test:tflite_test_data",
    ],
    tags = ["no_oss"],
    deps = [
        ":api_internal",
        ":graph_tools",
        ":litert_model_init",
        "//tensorflow/lite/experimental/lrt/test:common",
        "//tensorflow/lite/schema:schema_fbs",
        "@com_google_googletest//:gtest_main",
        "@flatbuffers//:runtime_cc",
        "@llvm-project//llvm:Support",
    ],
)

cc_library(
    name = "graph_tools",
    hdrs = [
        "graph_tools.h",
    ],
    deps = [
        ":api_internal",
        "//tensorflow/compiler/mlir/lite/core:model_builder_base",
        "//tensorflow/lite/c:c_api_types",
        "@com_google_absl//absl/strings:string_view",
        "@com_google_absl//absl/types:span",
        "@llvm-project//llvm:Support",
    ],
)

cc_library(
    name = "dynamic_loading",
    srcs = ["dynamic_loading.cc"],
    hdrs = ["dynamic_loading.h"],
    linkopts = ["-ldl"],
    deps = [
        ":logging",
        "//tensorflow/lite/experimental/lrt/c:litert_c_api",
        "//tensorflow/lite/experimental/lrt/cc:litert_cc_api",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/strings:string_view",
    ],
)

# copybara:uncomment_begin(no OSS for unique-test-directory)
# cc_test(
#     name = "dynamic_loading_test",
#     srcs = ["dynamic_loading_test.cc"],
#     tags = [
#         # Sanitizer runtimes are incompatible with RTLD_DEEPBIND.
#         "noasan",
#         "nomsan",
#         "nosan",
#     ],
#     deps = [
#         ":dynamic_loading",
#         "@com_google_googletest//:gtest_main",
#         "//testing/base/public:unique-test-directory",
#         "@com_google_absl//absl/strings:string_view",
#         "//tensorflow/lite/experimental/lrt/c:litert_c_api",
#         "//tensorflow/lite/experimental/lrt/test:common",
#     ],
# )
# copybara:uncomment_end

cc_test(
    name = "layout_cc_test",
    srcs = [
        "layout_cc_test.cc",
    ],
    deps = [
        "//tensorflow/lite/experimental/lrt/c:litert_c_api",
        "//tensorflow/lite/experimental/lrt/cc:litert_cc_api",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "ranked_tensor_type_cc_test",
    srcs = [
        "ranked_tensor_type_cc_test.cc",
    ],
    deps = [
        "//tensorflow/lite/experimental/lrt/c:litert_c_api",
        "//tensorflow/lite/experimental/lrt/cc:litert_cc_api",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "tensor_buffer",
    srcs = [
        "ahwb_buffer.cc",
        "dmabuf_buffer.cc",
        "fastrpc_buffer.cc",
        "ion_buffer.cc",
        "litert_event.cc",
        "litert_tensor_buffer.cc",
        "litert_tensor_buffer_requirements.cc",
        "tensor_buffer.cc",
    ],
    hdrs = [
        "ahwb_buffer.h",
        "dmabuf_buffer.h",
        "fastrpc_buffer.h",
        "ion_buffer.h",
        "tensor_buffer.h",
    ],
    deps = [
        ":logging",
        ":utils",
        "//tensorflow/lite/experimental/lrt/c:litert_c_api",
        "//tensorflow/lite/experimental/lrt/c:litert_tensor_buffer_c_api",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:node_hash_map",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/types:span",
    ],
)

cc_test(
    name = "tensor_buffer_test",
    srcs = [
        "tensor_buffer_test.cc",
    ],
    linkopts = select({
        "//tensorflow:android": ["-landroid"],
        "//conditions:default": [],
    }),
    deps = [
        ":tensor_buffer",
        "//tensorflow/lite/experimental/lrt/c:litert_c_api",
        "//tensorflow/lite/experimental/lrt/c:litert_tensor_buffer_c_api",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "tensor_buffer_cc_test",
    srcs = [
        "tensor_buffer_cc_test.cc",
    ],
    linkopts = select({
        "//tensorflow:android": ["-landroid"],
        "//conditions:default": [],
    }),
    deps = [
        ":tensor_buffer",
        "//tensorflow/lite/experimental/lrt/c:litert_c_api",
        "//tensorflow/lite/experimental/lrt/c:litert_tensor_buffer_c_api",
        "//tensorflow/lite/experimental/lrt/cc:litert_cc_api",
        "//tensorflow/lite/experimental/lrt/cc:litert_tensor_buffer_cc_api",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "tensor_buffer_requirements_test",
    srcs = [
        "tensor_buffer_requirements_test.cc",
    ],
    deps = [
        ":tensor_buffer",
        "//tensorflow/lite/experimental/lrt/c:litert_c_api",
        "//tensorflow/lite/experimental/lrt/c:litert_tensor_buffer_c_api",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "tensor_buffer_requirements_cc_test",
    srcs = [
        "tensor_buffer_requirements_cc_test.cc",
    ],
    deps = [
        ":tensor_buffer",
        "//tensorflow/lite/experimental/lrt/c:litert_c_api",
        "//tensorflow/lite/experimental/lrt/c:litert_tensor_buffer_c_api",
        "//tensorflow/lite/experimental/lrt/cc:litert_cc_api",
        "//tensorflow/lite/experimental/lrt/cc:litert_tensor_buffer_cc_api",
        "@com_google_absl//absl/types:span",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "utils",
    srcs = [
        "utils.cc",
    ],
    hdrs = [
        "utils.h",
    ],
    deps = [
        "//tensorflow/lite/experimental/lrt/c:litert_c_api",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:span",
    ],
)

cc_library(
    name = "tfl_utils",
    srcs = [
        "tfl_utils.cc",
    ],
    hdrs = [
        "tfl_utils.h",
    ],
    deps = [
        "//tensorflow/lite/c:c_api",
        "//tensorflow/lite/c:c_api_opaque",
        "//tensorflow/lite/c:c_api_types",
        "//tensorflow/lite/c:common",
        "//tensorflow/lite/experimental/lrt/c:litert_c_api",
        "//tensorflow/lite/experimental/lrt/cc:litert_cc_api",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:span",
    ],
)

cc_library(
    name = "logging",
    srcs = [
        "logging.cc",
    ],
    hdrs = [
        "logging.h",
    ],
    deps = ["//tensorflow/lite:minimal_logging"],
)
