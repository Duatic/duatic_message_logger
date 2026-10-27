
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

#include <chrono>   // NOLINT(build/include_order)
#include <cstdio>   // NOLINT(build/include_order)
#include <cstdlib>  // NOLINT(build/include_order)
#include <memory>   // NOLINT(build/include_order)
#include <mutex>    // NOLINT(build/include_order)
#include <string>   // NOLINT(build/include_order)
#include <utility>  // NOLINT(build/include_order)

#include <rcutils/logging.h>  // NOLINT(build/include_order)
#include <rclcpp/rclcpp.hpp>  // NOLINT(build/include_order)

#include <spdlog/common.h>           // NOLINT(build/include_order)
#include <spdlog/details/log_msg.h>  // NOLINT(build/include_order)
#include <spdlog/formatter.h>        // NOLINT(build/include_order)
#include <spdlog/sinks/base_sink.h>  // NOLINT(build/include_order)

namespace duatic::message_logger
{
class ROS2Sink : public spdlog::sinks::base_sink<std::mutex>
{
public:
  ROS2Sink() = default;

protected:
  void sink_it_(const spdlog::details::log_msg& msg) override
  {
    if (msg.level == spdlog::level::off) {
      return;
    }

    const std::string name(msg.logger_name.begin(), msg.logger_name.end());

    RCUTILS_LOGGING_AUTOINIT;
    if (!rcutils_logging_logger_is_enabled_for(name.c_str(), to_ros_severity(msg.level))) {
      write_fallback(msg, name);
    }

    auto ros_logger = rclcpp::get_logger(name);
    const std::string s(msg.payload.begin(), msg.payload.end());

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

  void flush_() override
  {
    std::fflush(output_stream());
  }

  void set_pattern_([[maybe_unused]] const std::string& pattern) override
  {
  }

  void set_formatter_(std::unique_ptr<spdlog::formatter> sink_formatter) override
  {
    formatter_ = std::move(sink_formatter);
  }

private:
  static int to_ros_severity(spdlog::level::level_enum log_level)
  {
    switch (log_level) {
      case spdlog::level::trace:
      case spdlog::level::debug:
        return RCUTILS_LOG_SEVERITY_DEBUG;
      case spdlog::level::warn:
        return RCUTILS_LOG_SEVERITY_WARN;
      case spdlog::level::err:
        return RCUTILS_LOG_SEVERITY_ERROR;
      case spdlog::level::critical:
        return RCUTILS_LOG_SEVERITY_FATAL;
      default:
        return RCUTILS_LOG_SEVERITY_INFO;
    }
  }

  static const char* to_ros_severity_name(spdlog::level::level_enum log_level)
  {
    switch (log_level) {
      case spdlog::level::trace:
      case spdlog::level::debug:
        return "DEBUG";
      case spdlog::level::warn:
        return "WARN";
      case spdlog::level::err:
        return "ERROR";
      case spdlog::level::critical:
        return "FATAL";
      default:
        return "INFO";
    }
  }

  // Match rcutils behavior
  static std::FILE* output_stream()
  {
    static std::FILE* const stream = [] {
      const char* value = std::getenv("RCUTILS_LOGGING_USE_STDOUT");
      return (value != nullptr && value[0] == '1' && value[1] == '\0') ? stdout : stderr;
    }();
    return stream;
  }

  // Mirrors the rcutils default console layout:
  //   [{severity}] [{seconds}.{nanoseconds}] [{name}]: {message}
  void write_fallback(const spdlog::details::log_msg& msg, const std::string& name)
  {
    const auto since_epoch = msg.time.time_since_epoch();
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
    const auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(since_epoch - seconds);

    std::FILE* stream = output_stream();

    std::fprintf(stream, "[%s] [%ld.%09ld] [%s]: %.*s\n", to_ros_severity_name(msg.level),
                 static_cast<int64_t>(seconds.count()), static_cast<int64_t>(nanos.count()), name.c_str(),
                 static_cast<int>(msg.payload.size()), msg.payload.data());
    std::fflush(stream);
  }
};

}  // namespace duatic::message_logger
