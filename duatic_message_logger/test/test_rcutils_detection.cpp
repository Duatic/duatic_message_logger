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

#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

#include <rcutils/logging.h>  // NOLINT(build/include_order)

#include "duatic_message_logger/logging.hpp"

namespace
{
std::vector<std::string> messages;

// Fails on a missing location, as the rosout handler of a ROS 2 node would crash on it
void capture(const rcutils_log_location_t* location, int /*severity*/, const char* name,
             rcutils_time_point_value_t /*timestamp*/, const char* format, va_list* args)
{
  ASSERT_NE(location, nullptr);
  char buffer[256];
  va_list copy;
  va_copy(copy, *args);
  std::vsnprintf(buffer, sizeof(buffer), format, copy);
  va_end(copy);
  messages.push_back(std::string(name) + ": " + buffer);
}
}  // namespace

// The test links rcutils, as a ROS 2 process does; the logger itself does not
TEST(RcutilsDetection, DefaultSinkWritesThroughRcutils)
{
  auto logger = duatic::message_logger::get_logger_with_default_sink("detected");
  ASSERT_EQ(rcutils_logging_initialize(), RCUTILS_RET_OK);
  rcutils_logging_set_output_handler(capture);

  duatic::message_logger::info(logger, "hello {}", 1);

  ASSERT_FALSE(messages.empty());
  EXPECT_EQ(messages.back(), "detected: hello 1");
}
