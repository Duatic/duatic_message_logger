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

#include <spdlog/spdlog.h>  // NOLINT(build/include_order)

namespace duatic::message_logger
{

using Logger = spdlog::logger;

enum class LogLevel
{
  Debug,
  Info,
  Warning,
  Error,
  Fatal
};

namespace impl
{
Logger& get_logger();

class LogStream
{
public:
  LogStream(Logger& logger, LogLevel level) : logger_(&logger), level_(level)
  {
  }

  template <typename T>
  requires(!std::is_function_v<std::remove_pointer_t<T>>) LogStream& operator<<(const T& value)
  {
    oss_ << value;
    return *this;
  }

  LogStream& operator<<(std::ostream& (*manip)(std::ostream&))
  {
    manip(oss_);
    return *this;
  }

  ~LogStream();

private:
  Logger* logger_;
  LogLevel level_;
  std::ostringstream oss_;
};

}  // namespace impl

template <typename... Args>
inline void log(LogLevel level, fmt::format_string<Args...> fmt, Args&&... args)
{
  switch (level) {
    case LogLevel::Debug:
      impl::get_logger().debug(fmt, std::forward<Args>(args)...);
      break;
    case LogLevel::Info:
      impl::get_logger().info(fmt, std::forward<Args>(args)...);
      break;
    case LogLevel::Warning:
      impl::get_logger().warn(fmt, std::forward<Args>(args)...);
      break;
    case LogLevel::Error:
      impl::get_logger().error(fmt, std::forward<Args>(args)...);
      break;
    case LogLevel::Fatal:
      impl::get_logger().critical(fmt, std::forward<Args>(args)...);
      break;
  }
}

template <typename... Args>
inline void debug(fmt::format_string<Args...> fmt, Args&&... args)
{
  log(LogLevel::Debug, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void info(fmt::format_string<Args...> fmt, Args&&... args)
{
  log(LogLevel::Info, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void warning(fmt::format_string<Args...> fmt, Args&&... args)
{
  log(LogLevel::Warning, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void error(fmt::format_string<Args...> fmt, Args&&... args)
{
  log(LogLevel::Error, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void fatal(fmt::format_string<Args...> fmt, Args&&... args)
{
  log(LogLevel::Fatal, fmt, std::forward<Args>(args)...);
}

inline impl::LogStream log(LogLevel level)
{
  return impl::LogStream(impl::get_logger(), level);
}
inline impl::LogStream debug()
{
  return log(LogLevel::Debug);
}

inline impl::LogStream info()
{
  return log(LogLevel::Info);
}

inline impl::LogStream warning()
{
  return log(LogLevel::Warning);
}
inline impl::LogStream error()
{
  return log(LogLevel::Error);
}
inline impl::LogStream fatal()
{
  return log(LogLevel::Fatal);
}

/**
 * @brief configure the default logger which is used if no logger is pased to the functions
 * @note this also changes the per default used sink !
 */
void configure_logger(Logger& logger);
/**
 * @brief obtain a named logger
 */
Logger get_logger(const std::string& name);

}  // namespace duatic::message_logger
