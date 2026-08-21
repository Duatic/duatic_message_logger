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
#include "duatic_message_logger/logging.hpp"

#include "spdlog/sinks/sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#ifdef ENABLE_ROS2_LOGGING
#include "duatic_message_logger/ros2_logger.hpp"
#endif

namespace duatic::message_logger
{

[[nodiscard]] std::optional<LogLevel> parse_level_from_string(std::string_view text) noexcept
{
  const auto lower = [](char c) noexcept { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c; };
  const auto matches = [&](std::string_view name) noexcept {
    return std::ranges::equal(text, name, [&](char a, char b) noexcept { return lower(a) == lower(b); });
  };

  if (matches("debug"))
    return LogLevel::Debug;
  if (matches("info"))
    return LogLevel::Info;
  if (matches("warning"))
    return LogLevel::Warning;
  if (matches("error"))
    return LogLevel::Error;
  if (matches("fatal"))
    return LogLevel::Fatal;

  return std::nullopt;
}

std::ostream& operator<<(std::ostream& os, const LogLevel level)
{
  switch (level) {
    case LogLevel::Debug:
      return os << "Debug";
    case LogLevel::Info:
      return os << "Info";
    case LogLevel::Warning:
      return os << "Warning";
    case LogLevel::Error:
      return os << "Error";
    case LogLevel::Fatal:
      return os << "Fatal";
  }
  return os << "Invalid LogLevel";
}

namespace
{

// The default sink and the default logger are created on first use and are deliberately never destroyed.
// if rclcpp is found we default to the ros2 logging infrastructure, otherwise we use std::cout logging via
// spdlog.
spdlog::sink_ptr& default_sink()
{
#ifdef ENABLE_ROS2_LOGGING
  static spdlog::sink_ptr& sink{ *new spdlog::sink_ptr{ std::make_shared<ROS2Sink>() } };
#else
  static spdlog::sink_ptr& sink{ *new spdlog::sink_ptr{ std::make_shared<spdlog::sinks::stdout_color_sink_mt>() } };
#endif
  return sink;
}

Logger& default_logger()
{
  static Logger& logger{ *new Logger{ "global_logger", default_sink() } };
  return logger;
}

}  // namespace

// Helper functions to convert our own log level into the library used log level
static constexpr spdlog::level::level_enum convert_level(const LogLevel level)
{
  switch (level) {
    case LogLevel::Debug:
      return spdlog::level::debug;
    case LogLevel::Info:
      return spdlog::level::info;
    case LogLevel::Warning:
      return spdlog::level::warn;
    case LogLevel::Error:
      return spdlog::level::err;
    case LogLevel::Fatal:
      return spdlog::level::critical;
  }
  return spdlog::level::info;
}

namespace impl
{

LogStream::~LogStream()
{
  logger_->log(convert_level(level_), oss_.str());
}
}  // namespace impl

void configure_logger(Logger& logger)
{
  default_logger() = logger;
}
void configure_logger_with_default_sink(Logger& logger)
{
  Logger& configured = default_logger();
  configured = logger;
  configured.sinks().push_back(default_sink());
}
Logger get_logger_with_default_sink(const std::string& name)
{
  return spdlog::logger(name, default_sink());
}
Logger& get_default_logger()
{
  return default_logger();
}
void configure_level(const LogLevel maximum_log_level)
{
  default_sink()->set_level(convert_level(maximum_log_level));
}

}  // namespace duatic::message_logger
