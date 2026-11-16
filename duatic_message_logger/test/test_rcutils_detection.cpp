/*
 * Copyright 2026 Duatic AG
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 * following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <gtest/gtest.h>

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

#include <rcutils/logging.h>  // NOLINT(build/include_order)

#include "duatic_message_logger/logging.hpp"

namespace
{
struct Record
{
  std::string text;  ///< "<name>: <message>"
  int severity;
  rcutils_time_point_value_t timestamp;
};
std::vector<Record> records;

// Fails on a missing location, as the rosout handler of a ROS 2 node would crash on it
void capture(const rcutils_log_location_t* location, int severity, const char* name,
             rcutils_time_point_value_t timestamp, const char* format, va_list* args)
{
  ASSERT_NE(location, nullptr);
  char buffer[256];
  va_list copy;
  va_copy(copy, *args);
  std::vsnprintf(buffer, sizeof(buffer), format, copy);
  va_end(copy);
  records.push_back({ std::string(name) + ": " + buffer, severity, timestamp });
}

class RcutilsDetection : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(rcutils_logging_initialize(), RCUTILS_RET_OK);
    rcutils_logging_set_output_handler(capture);
    records.clear();
  }
};
}  // namespace

// The test links rcutils, as a ROS 2 process does; the logger itself does not
TEST_F(RcutilsDetection, DefaultSinkWritesThroughRcutils)
{
  auto logger = duatic::message_logger::get_logger("detected");
  const auto before = std::chrono::system_clock::now();

  duatic::message_logger::info(logger, "hello {}", 1);

  ASSERT_FALSE(records.empty());
  EXPECT_EQ(records.back().text, "detected: hello 1");
  EXPECT_EQ(records.back().severity, RCUTILS_LOG_SEVERITY_INFO);
  const auto since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(before.time_since_epoch()).count();
  EXPECT_GE(records.back().timestamp, since_epoch);
}

// ROS only handles its own loggers: its levels neither drop our records nor change through our level settings
TEST_F(RcutilsDetection, RosLevelsDoNotFilterOurLoggers)
{
  auto logger = duatic::message_logger::get_logger("ros_filtered");
  ASSERT_EQ(rcutils_logging_set_logger_level("ros_filtered", RCUTILS_LOG_SEVERITY_ERROR), RCUTILS_RET_OK);

  duatic::message_logger::info(logger, "still delivered");
  ASSERT_FALSE(records.empty());
  EXPECT_EQ(records.back().text, "ros_filtered: still delivered");

  duatic::message_logger::set_level("ros_filtered", duatic::message_logger::LogLevel::Debug);
  EXPECT_EQ(rcutils_logging_get_logger_level("ros_filtered"), RCUTILS_LOG_SEVERITY_ERROR);
  duatic::message_logger::debug(logger, "debug delivered");
  EXPECT_EQ(records.back().text, "ros_filtered: debug delivered");
  EXPECT_EQ(records.back().severity, RCUTILS_LOG_SEVERITY_DEBUG);
  duatic::message_logger::reset_level("ros_filtered");
}

// A record logged after rcutils logging was shut down is still printed, not rejected by the console handler
TEST_F(RcutilsDetection, RecordAfterShutdownInitializesAgain)
{
  auto logger = duatic::message_logger::get_logger("after_shutdown");
  ASSERT_EQ(rcutils_logging_shutdown(), RCUTILS_RET_OK);

  testing::internal::CaptureStderr();
  duatic::message_logger::info(logger, "still printed");
  const std::string output = testing::internal::GetCapturedStderr();

  EXPECT_NE(output.find("still printed"), std::string::npos) << output;
  EXPECT_EQ(output.find("isn't initialized"), std::string::npos) << output;
}
