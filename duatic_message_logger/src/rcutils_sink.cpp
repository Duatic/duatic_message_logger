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
#include "duatic_message_logger/detail/rcutils_sink.hpp"

#include <cstdarg>

#if defined(__unix__) || defined(__APPLE__)
#include <dlfcn.h>
#endif

namespace duatic::message_logger::detail
{
namespace
{
int to_ros_severity(spdlog::level::level_enum level)
{
  switch (level) {
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

// Builds the va_list the output handler takes; the format attribute lets -Wformat check the calls
[[gnu::format(printf, 6, 7)]] void call_output_handler(rcutils_logging_output_handler_t handler,
                                                       const rcutils_log_location_t* location, int severity,
                                                       const char* name, rcutils_time_point_value_t timestamp,
                                                       const char* format, ...)
{
  va_list args;
  va_start(args, format);
  handler(location, severity, name, timestamp, format, &args);
  va_end(args);
}

// Same stream as rcutils
std::FILE* output_stream()
{
  static std::FILE* const stream = [] {
    const char* value = std::getenv("RCUTILS_LOGGING_USE_STDOUT");
    return (value != nullptr && value[0] == '1' && value[1] == '\0') ? stdout : stderr;
  }();
  return stream;
}

void* find_symbol([[maybe_unused]] const char* name)
{
#if defined(__unix__) || defined(__APPLE__)
  // A library loaded privately, as Python loads extension modules and their dependencies, is not in the global scope;
  // RTLD_NOLOAD returns it only if it is already loaded, so this never loads ROS
  if (void* rcutils = dlopen("librcutils.so", RTLD_LAZY | RTLD_NOLOAD); rcutils != nullptr) {
    if (void* symbol = dlsym(rcutils, name); symbol != nullptr) {
      return symbol;
    }
  }
  return dlsym(RTLD_DEFAULT, name);
#else
  return nullptr;
#endif
}
}  // namespace

RcutilsSink::RcutilsSink() : stdout_(std::make_shared<spdlog::sinks::stdout_color_sink_st>())
{
}

void RcutilsSink::set_ros_logger(std::string ros_logger)
{
  const std::lock_guard lock(mutex_);
  ros_logger_ = std::move(ros_logger);
}

void RcutilsSink::resolve()
{
  auto* get_handler = reinterpret_cast<RcutilsGetOutputHandler>(find_symbol("rcutils_logging_get_output_handler"));
  auto* initialize = reinterpret_cast<RcutilsInitialize>(find_symbol("rcutils_logging_initialize"));
  if (get_handler == nullptr || initialize == nullptr) {
    return;
  }
  // Returns early when rcutils logging is already initialized
  initialize();
  get_output_handler_ = get_handler;
}

void RcutilsSink::sink_it_(const spdlog::details::log_msg& msg)
{
  if (msg.level == spdlog::level::off) {
    return;
  }
  if (!resolved_) {
    resolved_ = true;
    resolve();
  }
  const rcutils_logging_output_handler_t handler = get_output_handler_ != nullptr ? get_output_handler_() : nullptr;
  if (handler == nullptr) {
    stdout_->log(msg);
    return;
  }
  // The logger's level already decided, so this skips the level check rcutils_log would do
  std::string name(msg.logger_name.begin(), msg.logger_name.end());
  std::string_view text(msg.payload.data(), msg.payload.size());
  std::string prefixed;
  if (!ros_logger_.empty()) {
    prefixed = "[" + name + "] ";
    prefixed.append(text);
    text = prefixed;
    name = ros_logger_;
  }
  const rcutils_log_location_t location{ msg.source.funcname != nullptr ? msg.source.funcname : "",
                                         msg.source.filename != nullptr ? msg.source.filename : "",
                                         static_cast<std::size_t>(msg.source.line > 0 ? msg.source.line : 0) };
  const auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(msg.time.time_since_epoch()).count();
  call_output_handler(handler, &location, to_ros_severity(msg.level), name.c_str(),
                      static_cast<rcutils_time_point_value_t>(timestamp), "%.*s", static_cast<int>(text.size()),
                      text.data());
}

void RcutilsSink::flush_()
{
  stdout_->flush();
  std::fflush(output_stream());
}

void RcutilsSink::set_pattern_(const std::string& pattern)
{
  stdout_->set_pattern(pattern);
}

void RcutilsSink::set_formatter_(std::unique_ptr<spdlog::formatter> sink_formatter)
{
  stdout_->set_formatter(std::move(sink_formatter));
}

}  // namespace duatic::message_logger::detail
