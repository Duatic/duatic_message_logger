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

#include <cinttypes>

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
      return rcutils_severity_debug;
    case spdlog::level::warn:
      return rcutils_severity_warn;
    case spdlog::level::err:
      return rcutils_severity_error;
    case spdlog::level::critical:
      return rcutils_severity_fatal;
    default:
      return rcutils_severity_info;
  }
}

const char* to_ros_severity_name(spdlog::level::level_enum level)
{
  switch (level) {
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
  return dlsym(RTLD_DEFAULT, name);
#else
  return nullptr;
#endif
}
}  // namespace

RcutilsSink::RcutilsSink() : stdout_(std::make_shared<spdlog::sinks::stdout_color_sink_st>())
{
}

void RcutilsSink::resolve()
{
  auto* log = reinterpret_cast<RcutilsLog>(find_symbol("rcutils_log"));
  auto* enabled = reinterpret_cast<RcutilsIsEnabledFor>(find_symbol("rcutils_logging_logger_is_enabled_for"));
  auto* initialize = reinterpret_cast<RcutilsInitialize>(find_symbol("rcutils_logging_initialize"));
  if (log == nullptr || enabled == nullptr || initialize == nullptr) {
    return;
  }
  // Returns early when rcutils logging is already initialized
  initialize();
  log_ = log;
  enabled_ = enabled;
}

void RcutilsSink::sink_it_(const spdlog::details::log_msg& msg)
{
  if (msg.level == spdlog::level::off) {
    return;
  }
  if (log_ == nullptr) {
    const auto now = std::chrono::steady_clock::now();
    if (now >= next_lookup_) {
      next_lookup_ = now + lookup_period;
      resolve();
    }
  }
  if (log_ == nullptr) {
    stdout_->log(msg);
    return;
  }
  const std::string name(msg.logger_name.begin(), msg.logger_name.end());
  const int severity = to_ros_severity(msg.level);
  if (!enabled_(name.c_str(), severity)) {
    // The logger's own level let it through, so it is shown even where ROS filters it
    write_fallback(msg, name);
    return;
  }
  const RcutilsLocation location{ msg.source.funcname != nullptr ? msg.source.funcname : "",
                                  msg.source.filename != nullptr ? msg.source.filename : "",
                                  static_cast<std::size_t>(msg.source.line > 0 ? msg.source.line : 0) };
  log_(&location, severity, name.c_str(), "%.*s", static_cast<int>(msg.payload.size()), msg.payload.data());
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

// Mirrors the rcutils default console layout: [{severity}] [{seconds}.{nanoseconds}] [{name}]: {message}
void RcutilsSink::write_fallback(const spdlog::details::log_msg& msg, const std::string& name)
{
  const auto since_epoch = msg.time.time_since_epoch();
  const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
  const auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(since_epoch - seconds);
  std::FILE* stream = output_stream();
  std::fprintf(stream, "[%s] [%" PRId64 ".%09" PRId64 "] [%s]: %.*s\n", to_ros_severity_name(msg.level),
               static_cast<std::int64_t>(seconds.count()), static_cast<std::int64_t>(nanos.count()), name.c_str(),
               static_cast<int>(msg.payload.size()), msg.payload.data());
  std::fflush(stream);
}

}  // namespace duatic::message_logger::detail
