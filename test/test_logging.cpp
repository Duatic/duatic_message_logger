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
#include "duatic_message_logger/logging.hpp"

#include "spdlog/sinks/stdout_color_sinks.h"

TEST(Logging, DefaultOutput)
{
  // Test stream like usage
  duatic::message_logger::debug() << "Test debug"
                                  << "test" << std::endl;
  duatic::message_logger::info() << "Test info";
  duatic::message_logger::warning() << "Test warning";
  duatic::message_logger::error() << "Test error";
  duatic::message_logger::fatal() << "Test fatal";

  // Test fmt style usage
  duatic::message_logger::debug("Debug {}", 1);
  duatic::message_logger::info("Info {}", 1);
  duatic::message_logger::warning("Warning {}", 1);
  duatic::message_logger::error("Error {}", 1);
  duatic::message_logger::fatal("Fatal {}", 1);
}

TEST(Logging, StandardOut)
{
  // Reconfigure to use a different output sink
  auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  // ROS2-like pattern:
  // %^   = start color
  // %l   = log level
  // %$   = reset color
  // %n   = logger name (node name)
  // %v   = message
  // %Y-%m-%d %H:%M:%S.%f = timestamp with microseconds
  sink->set_color_mode(spdlog::color_mode::always);
  sink->set_pattern("[%^%l%$] [%Y-%m-%d %H:%M:%S.%e] [%n]: %v");
  spdlog::logger logger{ "test", sink };
  duatic::message_logger::configure_logger(logger);

  duatic::message_logger::debug() << "Test debug"
                                  << "test" << std::endl;
  duatic::message_logger::info() << "Test info";
  duatic::message_logger::warning() << "Test warning";
  duatic::message_logger::error() << "Test error";
  duatic::message_logger::fatal() << "Test fatal";
}
