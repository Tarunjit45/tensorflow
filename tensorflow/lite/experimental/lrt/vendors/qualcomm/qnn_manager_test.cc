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

#include "tensorflow/lite/experimental/lrt/vendors/qualcomm/qnn_manager.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "tensorflow/lite/experimental/lrt/test/common.h"
#include "tensorflow/lite/experimental/lrt/vendors/qualcomm/tools/dump.h"

namespace {

using ::litert::qnn::QnnManager;
using ::litert::qnn::SetupAll;
using ::litert::qnn::internal::Dump;
using ::testing::HasSubstr;

// NOTE: This tests that all of the dynamic loading works properly and
// the QNN SDK instance can be properly initialized and destroyed.

TEST(QnnManagerTest, SetupQnnManager) {
  QnnManager qnn;
  ASSERT_STATUS_OK(SetupAll(/*soc_model=*/std::nullopt, qnn));
}

TEST(QnnManagerTest, SetupQnnManagerWithSystem) {
  QnnManager qnn;
  ASSERT_STATUS_OK(
      SetupAll(/*soc_model=*/std::nullopt, qnn, /*load_system=*/true));
}

TEST(QnnManagerTest, SetupQnnManagerWithContext) {
  QnnManager qnn;
  ASSERT_STATUS_OK(SetupAll(/*soc_model=*/std::nullopt, qnn,
                            /*load_system=*/false, /*load_context=*/true));
}

TEST(QnnManagerTest, Dump) {
  QnnManager qnn;
  ASSERT_STATUS_OK(
      SetupAll(/*soc_model=*/std::nullopt, qnn, /*load_system=*/true))

  std::ostringstream dump;
  Dump(qnn, dump);

  EXPECT_THAT(dump.str(), HasSubstr("< QnnInterface_t >"));
  EXPECT_THAT(dump.str(), HasSubstr("< QnnSystemInterface_t >"));
}

}  // namespace
