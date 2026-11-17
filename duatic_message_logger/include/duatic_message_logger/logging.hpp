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

// This header is also compiled as C++17 by the users of the MELO compatibility package (message_logger), so it must
// stay C++17 compatible: no concepts, ranges or other C++20 features here; the library sources may use C++20.

#include <array>
#include <chrono>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <spdlog/spdlog.h>  // NOLINT(build/include_order)

/**
 * Loggers, levels and sinks
 *
 * - A registered logger is obtained with get_logger(name), or is the default logger. Its level comes from the level
 *   settings (set_level), never from ROS.
 * - Log through a logger with its members, logger.info("Value {}", 1) or logger.info() << "Value " << 1, or with the
 *   free functions, logging::info(logger, "Value {}", 1); the free functions without a logger use the default logger.
 * - A custom logger is a Logger built from your own spdlog::logger. It owns its level, which you set on that
 *   spdlog::logger; level settings never touch it.
 * - A level setting for a name applies to that name and to every name below it: "a" covers "a.b" and "a.b.c", but not
 *   "ab", as in ROS 2. Where no setting matches, the Default level applies (until set: the environment variable
 *   DUATIC_LOG_LEVEL, read once at first use, else Info). The Default level is not the level of the default logger,
 *   which is set by its name like any other logger.
 * - Registered loggers write every record to one output (set_output_sink; by default ROS 2 logging when the process has
 *   it loaded, stdout otherwise) and to any number of attached sinks (attach_sink).
 * - Loggers decide, sinks are destinations: a record is filtered once, by its logger's level, and every sink receives
 *   all records that passed. Keep sinks at their default level (trace) and filter only for display.
 * - Sinks run on the logging thread while the sinks are locked: a sink must not log through a registered logger (that
 *   deadlocks), and should hand records to another thread instead of doing slow work.
 */
namespace duatic::message_logger
{

/// @brief The log levels, the same as in ROS 2.
enum class LogLevel
{
  Debug,
  Info,
  Warning,
  Error,
  Fatal
};

/// @brief All levels, from the most to the least verbose.
inline constexpr std::array<LogLevel, 5> log_levels{ LogLevel::Debug, LogLevel::Info, LogLevel::Warning,
                                                     LogLevel::Error, LogLevel::Fatal };

/**
 * @brief Returns the name of @p level as parse_level() accepts it: debug, info, warning, error or fatal.
 */
[[nodiscard]] constexpr std::string_view to_string(LogLevel level) noexcept
{
  switch (level) {
    case LogLevel::Debug:
      return "debug";
    case LogLevel::Info:
      return "info";
    case LogLevel::Warning:
      return "warning";
    case LogLevel::Error:
      return "error";
    case LogLevel::Fatal:
      return "fatal";
  }
  return "invalid";
}

/**
 * @brief Returns the level named @p text, or nothing if it names none.
 * @note Accepts the names to_string() returns, case insensitive, and "warn".
 */
[[nodiscard]] std::optional<LogLevel> parse_level(std::string_view text) noexcept;

/// @brief Writes the name to_string() returns.
std::ostream& operator<<(std::ostream& os, LogLevel level);

class Logger;

namespace impl
{

constexpr spdlog::level::level_enum to_spdlog(LogLevel level) noexcept
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

const std::shared_ptr<spdlog::logger>& spdlog_logger(const Logger& logger) noexcept;

class LogStream;

}  // namespace impl

/**
 * @brief Handle to a logger; copies refer to the same logger.
 *
 * Obtain registered loggers with get_logger(), or wrap your own spdlog::logger to get a custom logger.
 * Log through it with logger.info("Value {}", 1) or logger.info() << "Value " << 1, or with the free functions such
 * as logging::info(logger, "Value {}", 1).
 */
class Logger
{
public:
  /**
   * @brief Creates a custom logger, which owns its level and its sinks.
   * @throws std::invalid_argument if @p logger is null.
   */
  explicit Logger(std::shared_ptr<spdlog::logger> logger);

  // No move operations: moving copies, so a handle never becomes empty
  Logger(const Logger&) = default;
  Logger& operator=(const Logger&) = default;
  ~Logger() = default;

  [[nodiscard]] const std::string& name() const noexcept
  {
    return logger_->name();
  }

  /// @brief Returns whether a record of @p level passes the logger's level.
  [[nodiscard]] bool should_log(LogLevel level) const noexcept
  {
    return logger_->should_log(impl::to_spdlog(level));
  }

  // fmt style api; formats only when the record passes the logger's level
  template <typename... Args>
  void log(LogLevel level, fmt::format_string<Args...> fmt, Args&&... args) const;
  template <typename... Args>
  void debug(fmt::format_string<Args...> fmt, Args&&... args) const;
  template <typename... Args>
  void info(fmt::format_string<Args...> fmt, Args&&... args) const;
  template <typename... Args>
  void warning(fmt::format_string<Args...> fmt, Args&&... args) const;
  template <typename... Args>
  void error(fmt::format_string<Args...> fmt, Args&&... args) const;
  template <typename... Args>
  void fatal(fmt::format_string<Args...> fmt, Args&&... args) const;

  // stream style api; does nothing below the logger's level
  impl::LogStream log(LogLevel level) const;
  impl::LogStream debug() const;
  impl::LogStream info() const;
  impl::LogStream warning() const;
  impl::LogStream error() const;
  impl::LogStream fatal() const;

private:
  friend const std::shared_ptr<spdlog::logger>& impl::spdlog_logger(const Logger& logger) noexcept;

  std::shared_ptr<spdlog::logger> logger_;
};

namespace impl
{

inline const std::shared_ptr<spdlog::logger>& spdlog_logger(const Logger& logger) noexcept
{
  return logger.logger_;
}

}  // namespace impl

/**
 * @brief Lets a repeated message through at most once per period, on the time passed to the throttled log calls.
 *
 * Keep one per message, and one per object when several objects log the same message. A message below the logger's
 * level does not start a period.
 *
 * @note Not thread safe.
 */
class LogThrottle
{
public:
  using Clock = std::chrono::steady_clock;  ///< Clock of the times passed to the throttled log calls.

  /**
   * @brief Creates a throttle that lets the first message through.
   * @param period Minimum time between two messages.
   */
  explicit LogThrottle(std::chrono::nanoseconds period) noexcept : period_(period)
  {
  }

private:
  template <typename... Args>
  friend void log(LogThrottle& throttle, Clock::time_point now, const Logger& logger, LogLevel level,
                  fmt::format_string<Args...> fmt, Args&&... args);

  /// @brief Returns true on the first call and once the period has passed, and then starts a new period.
  [[nodiscard]] bool ready(Clock::time_point now) noexcept
  {
    if (now < next_) {
      return false;
    }
    next_ = now + period_;
    return true;
  }

  std::chrono::nanoseconds period_;
  Clock::time_point next_{ Clock::time_point::min() };  ///< Earliest time the next message passes
};

/**
 * @brief Lets a message through only the first time it is logged.
 *
 * Keep one per message, and one per object when several objects log the same message. A message below the logger's
 * level does not count as logged.
 *
 * @note Not thread safe.
 */
class LogOnce
{
private:
  template <typename... Args>
  friend void log(LogOnce& once, const Logger& logger, LogLevel level, fmt::format_string<Args...> fmt, Args&&... args);

  bool logged_{ false };
};

/**
 * @brief Returns the default logger, which the calls without a logger use.
 */
[[nodiscard]] const Logger& get_default_logger();

namespace impl
{

// Wrapper around spdlog logger which provide a streaming style api; does nothing below the logger's level
class LogStream
{
public:
  LogStream(const Logger& logger, LogLevel level) : level_(level)
  {
    if (logger.should_log(level)) {
      record_.emplace(spdlog_logger(logger));
    }
  }

  template <typename T>
  LogStream& operator<<(const T& value)
  {
    if (record_) {
      record_->stream << value;
    }
    return *this;
  }

  LogStream& operator<<(std::ostream& (*manip)(std::ostream&))
  {
    if (record_) {
      manip(record_->stream);
    }
    return *this;
  }
  LogStream& operator<<(std::ios_base& (*manip)(std::ios_base&))
  {
    if (record_) {
      manip(record_->stream);
    }
    return *this;
  }
  ~LogStream();

private:
  struct Record
  {
    explicit Record(std::shared_ptr<spdlog::logger> record_logger) : logger(std::move(record_logger))
    {
    }

    std::shared_ptr<spdlog::logger> logger;  ///< Keeps the logger alive until the record is written
    std::ostringstream stream;
  };

  LogLevel level_;
  std::optional<Record> record_;  ///< Only constructed when the record passes the logger's level
};

}  // namespace impl

template <typename... Args>
inline void Logger::log(LogLevel level, fmt::format_string<Args...> fmt, Args&&... args) const
{
  logger_->log(impl::to_spdlog(level), fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void Logger::debug(fmt::format_string<Args...> fmt, Args&&... args) const
{
  log(LogLevel::Debug, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void Logger::info(fmt::format_string<Args...> fmt, Args&&... args) const
{
  log(LogLevel::Info, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void Logger::warning(fmt::format_string<Args...> fmt, Args&&... args) const
{
  log(LogLevel::Warning, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void Logger::error(fmt::format_string<Args...> fmt, Args&&... args) const
{
  log(LogLevel::Error, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void Logger::fatal(fmt::format_string<Args...> fmt, Args&&... args) const
{
  log(LogLevel::Fatal, fmt, std::forward<Args>(args)...);
}

inline impl::LogStream Logger::log(LogLevel level) const
{
  return impl::LogStream(*this, level);
}
inline impl::LogStream Logger::debug() const
{
  return log(LogLevel::Debug);
}
inline impl::LogStream Logger::info() const
{
  return log(LogLevel::Info);
}
inline impl::LogStream Logger::warning() const
{
  return log(LogLevel::Warning);
}
inline impl::LogStream Logger::error() const
{
  return log(LogLevel::Error);
}
inline impl::LogStream Logger::fatal() const
{
  return log(LogLevel::Fatal);
}

// fmt style api
template <typename... Args>
inline void log(const Logger& logger, LogLevel level, fmt::format_string<Args...> fmt, Args&&... args)
{
  logger.log(level, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void log(LogLevel level, fmt::format_string<Args...> fmt, Args&&... args)
{
  log(get_default_logger(), level, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void debug(fmt::format_string<Args...> fmt, Args&&... args)
{
  log(LogLevel::Debug, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void debug(const Logger& logger, fmt::format_string<Args...> fmt, Args&&... args)
{
  log(logger, LogLevel::Debug, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void info(fmt::format_string<Args...> fmt, Args&&... args)
{
  log(LogLevel::Info, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void info(const Logger& logger, fmt::format_string<Args...> fmt, Args&&... args)
{
  log(logger, LogLevel::Info, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void warning(fmt::format_string<Args...> fmt, Args&&... args)
{
  log(LogLevel::Warning, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void warning(const Logger& logger, fmt::format_string<Args...> fmt, Args&&... args)
{
  log(logger, LogLevel::Warning, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void error(fmt::format_string<Args...> fmt, Args&&... args)
{
  log(LogLevel::Error, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void error(const Logger& logger, fmt::format_string<Args...> fmt, Args&&... args)
{
  log(logger, LogLevel::Error, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void fatal(fmt::format_string<Args...> fmt, Args&&... args)
{
  log(LogLevel::Fatal, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void fatal(const Logger& logger, fmt::format_string<Args...> fmt, Args&&... args)
{
  log(logger, LogLevel::Fatal, fmt, std::forward<Args>(args)...);
}

// throttled fmt style api: formats and logs only when the level passes and the throttle lets the message through
template <typename... Args>
inline void log(LogThrottle& throttle, LogThrottle::Clock::time_point now, const Logger& logger, LogLevel level,
                fmt::format_string<Args...> fmt, Args&&... args)
{
  if (logger.should_log(level) && throttle.ready(now)) {
    log(logger, level, fmt, std::forward<Args>(args)...);
  }
}
template <typename... Args>
inline void debug(LogThrottle& throttle, LogThrottle::Clock::time_point now, const Logger& logger,
                  fmt::format_string<Args...> fmt, Args&&... args)
{
  log(throttle, now, logger, LogLevel::Debug, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void info(LogThrottle& throttle, LogThrottle::Clock::time_point now, const Logger& logger,
                 fmt::format_string<Args...> fmt, Args&&... args)
{
  log(throttle, now, logger, LogLevel::Info, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void warning(LogThrottle& throttle, LogThrottle::Clock::time_point now, const Logger& logger,
                    fmt::format_string<Args...> fmt, Args&&... args)
{
  log(throttle, now, logger, LogLevel::Warning, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void error(LogThrottle& throttle, LogThrottle::Clock::time_point now, const Logger& logger,
                  fmt::format_string<Args...> fmt, Args&&... args)
{
  log(throttle, now, logger, LogLevel::Error, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void fatal(LogThrottle& throttle, LogThrottle::Clock::time_point now, const Logger& logger,
                  fmt::format_string<Args...> fmt, Args&&... args)
{
  log(throttle, now, logger, LogLevel::Fatal, fmt, std::forward<Args>(args)...);
}

// log once fmt style api: formats and logs only the first call whose level passes
template <typename... Args>
inline void log(LogOnce& once, const Logger& logger, LogLevel level, fmt::format_string<Args...> fmt, Args&&... args)
{
  if (logger.should_log(level) && !once.logged_) {
    once.logged_ = true;
    log(logger, level, fmt, std::forward<Args>(args)...);
  }
}
template <typename... Args>
inline void debug(LogOnce& once, const Logger& logger, fmt::format_string<Args...> fmt, Args&&... args)
{
  log(once, logger, LogLevel::Debug, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void info(LogOnce& once, const Logger& logger, fmt::format_string<Args...> fmt, Args&&... args)
{
  log(once, logger, LogLevel::Info, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void warning(LogOnce& once, const Logger& logger, fmt::format_string<Args...> fmt, Args&&... args)
{
  log(once, logger, LogLevel::Warning, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void error(LogOnce& once, const Logger& logger, fmt::format_string<Args...> fmt, Args&&... args)
{
  log(once, logger, LogLevel::Error, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void fatal(LogOnce& once, const Logger& logger, fmt::format_string<Args...> fmt, Args&&... args)
{
  log(once, logger, LogLevel::Fatal, fmt, std::forward<Args>(args)...);
}

// stream style api
inline impl::LogStream log(LogLevel level)
{
  return impl::LogStream(get_default_logger(), level);
}
inline impl::LogStream log(const Logger& logger, LogLevel level)
{
  return logger.log(level);
}
inline impl::LogStream debug()
{
  return log(LogLevel::Debug);
}
inline impl::LogStream debug(const Logger& logger)
{
  return log(logger, LogLevel::Debug);
}

inline impl::LogStream info()
{
  return log(LogLevel::Info);
}
inline impl::LogStream info(const Logger& logger)
{
  return log(logger, LogLevel::Info);
}

inline impl::LogStream warning()
{
  return log(LogLevel::Warning);
}
inline impl::LogStream warning(const Logger& logger)
{
  return log(logger, LogLevel::Warning);
}
inline impl::LogStream error()
{
  return log(LogLevel::Error);
}
inline impl::LogStream error(const Logger& logger)
{
  return log(logger, LogLevel::Error);
}
inline impl::LogStream fatal()
{
  return log(LogLevel::Fatal);
}
inline impl::LogStream fatal(const Logger& logger)
{
  return log(logger, LogLevel::Fatal);
}

/**
 * @brief Returns the registered logger of @p name, creating it on first use.
 * @note The same name always returns the same logger. It writes to the output and the attached sinks, and follows the
 * level settings.
 * @throws std::invalid_argument if @p name is empty.
 */
[[nodiscard]] Logger get_logger(const std::string& name);

/**
 * @brief Sets the default logger, which the calls without a logger use.
 * @note A registered logger keeps following the level settings, a custom logger keeps its own level and sinks.
 * @note Not synchronized with logging; call it before other threads log through the default logger.
 */
void set_default_logger(const Logger& logger);

/// @brief Selects the Default level in the level functions, e.g. set_level(Default, LogLevel::Debug).
struct DefaultLevelTag
{
  enum class Construct
  {
    token
  };
  // No default constructor, as for std::nullopt_t: `{}` never converts to the tag
  explicit constexpr DefaultLevelTag(Construct /*token*/) noexcept
  {
  }
};

/// @brief The level of every registered logger that no name setting covers; not the level of the default logger.
inline constexpr DefaultLevelTag Default{ DefaultLevelTag::Construct::token };

/**
 * @brief Sets the level of @p name and of every name below it.
 * @throws std::invalid_argument if @p name is empty.
 */
void set_level(const std::string& name, LogLevel level);
/**
 * @brief Sets the Default level, used by the registered loggers that no name setting covers.
 */
void set_level(DefaultLevelTag, LogLevel level);

/**
 * @brief Removes the setting of @p name, so it inherits again.
 * @throws std::invalid_argument if @p name is empty.
 */
void reset_level(const std::string& name);
/**
 * @brief Removes the Default level setting, which brings back DUATIC_LOG_LEVEL, else Info.
 */
void reset_level(DefaultLevelTag);

/**
 * @brief Returns the level set for exactly @p name, if any.
 * @throws std::invalid_argument if @p name is empty.
 */
[[nodiscard]] std::optional<LogLevel> get_level(const std::string& name);
/**
 * @brief Returns the Default level, if one was set; DUATIC_LOG_LEVEL is not a setting.
 */
[[nodiscard]] std::optional<LogLevel> get_level(DefaultLevelTag);

/**
 * @brief Returns the level a registered logger of @p name uses.
 * @throws std::invalid_argument if @p name is empty.
 */
[[nodiscard]] LogLevel get_effective_level(const std::string& name);
/**
 * @brief Returns the Default level in force: the one set, else DUATIC_LOG_LEVEL, else Info.
 */
[[nodiscard]] LogLevel get_effective_level(DefaultLevelTag);

/**
 * @brief Replaces the output of the registered loggers.
 * @note By default the output is ROS 2 logging when the process has rcutils loaded, stdout otherwise.
 * @note Applies to loggers created before the call as well; attached sinks stay attached.
 * @throws std::invalid_argument if @p sink is null.
 */
void set_output_sink(spdlog::sink_ptr sink);

/**
 * @brief Also forwards every record of the registered loggers to @p sink.
 * @note Applies to loggers created before the call as well. Attaching an attached sink again has no effect; attaching
 * the output delivers its records twice.
 * @note @p sink is called on the logging thread while the sinks are locked: it must not log through a registered
 * logger, and should queue records for another thread (in Qt, connect with Qt::QueuedConnection).
 * @throws std::invalid_argument if @p sink is null.
 */
void attach_sink(spdlog::sink_ptr sink);

/**
 * @brief Stops forwarding to a sink attached with attach_sink().
 * @note Once it returns, @p sink receives no further records. Detaching a sink that is not attached has no effect.
 */
void detach_sink(const spdlog::sink_ptr& sink);

}  // namespace duatic::message_logger
