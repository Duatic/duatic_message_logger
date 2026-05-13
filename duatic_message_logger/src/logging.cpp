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

#include "spdlog/sinks/stdout_color_sinks.h"

#ifdef ENABLE_ROS2_LOGGING
#include "duatic_message_logger/ros2_logger.hpp"
#endif

namespace duatic::message_logger
{
// As soon as rclcpp is found we default to the ros2 logging infrastructure
// otherwise we use std::cout logging via spdlog
#ifdef ENABLE_ROS2_LOGGING
static auto sink_{ std::make_shared<ROS2Sink>() };
#else
static auto sink_{ std::make_shared<spdlog::sinks::stdout_color_sink_mt>() };
#endif

static spdlog::logger logger_{ "global_logger", sink_ };

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
  logger_ = logger;
}
void configure_logger_with_default_sink(Logger& logger)
{
  logger_ = logger;
  logger_.sinks().push_back(sink_);
}
Logger get_logger_with_default_sink(const std::string& name)
{
  return spdlog::logger(name, sink_);
}
Logger& get_default_logger()
{
  return logger_;
}

}  // namespace duatic::message_logger
