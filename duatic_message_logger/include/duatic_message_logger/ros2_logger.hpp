
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
#pragma once

#include <string>
#include <utility>
#include <memory>

#include <rclcpp/rclcpp.hpp>

#include <spdlog/sinks/sink.h>       // NOLINT(build/include_order)
#include <spdlog/details/log_msg.h>  // NOLINT(build/include_order)
#include <spdlog/common.h>           // NOLINT(build/include_order)

namespace duatic::message_logger
{

class ROS2Sink : public spdlog::sinks::sink
{
public:
  ROS2Sink()
  {
  }

  void log(const spdlog::details::log_msg& msg) override
  {
    auto ros_logger = rclcpp::get_logger(std::string(msg.logger_name.begin(), msg.logger_name.end()));
    std::string s(msg.payload.begin(), msg.payload.end());

    switch (msg.level) {
      case spdlog::level::trace:
      case spdlog::level::debug:
        RCLCPP_DEBUG_STREAM(ros_logger, s);
        break;
      case spdlog::level::info:
        RCLCPP_INFO_STREAM(ros_logger, s);
        break;
      case spdlog::level::warn:
        RCLCPP_WARN_STREAM(ros_logger, s);
        break;
      case spdlog::level::err:
        RCLCPP_ERROR_STREAM(ros_logger, s);
        break;
      case spdlog::level::critical:
        RCLCPP_FATAL_STREAM(ros_logger, s);
        break;
      case spdlog::level::off:
        // do nothing
        break;
      default:
        RCLCPP_INFO_STREAM(ros_logger, s);
        break;
    }
  }

  void flush() override
  {
  }

  void set_pattern([[maybe_unused]] const std::string& pattern) override
  {
  }

  void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) override
  {
    formatter_ = std::move(sink_formatter);
  }

private:
  std::unique_ptr<spdlog::formatter> formatter_;
};

}  // namespace duatic::message_logger
