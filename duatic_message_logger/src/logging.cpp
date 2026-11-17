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

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "spdlog/sinks/dist_sink.h"
#include "spdlog/sinks/sink.h"

#include "duatic_message_logger/detail/rcutils_sink.hpp"

namespace duatic::message_logger
{

[[nodiscard]] std::optional<LogLevel> parse_level(std::string_view text) noexcept
{
  const auto lower = [](char c) noexcept { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c; };
  const auto matches = [&](std::string_view name) noexcept {
    return std::ranges::equal(text, name, [&](char a, char b) noexcept { return lower(a) == lower(b); });
  };

  if (const auto it = std::ranges::find_if(log_levels, [&](LogLevel level) { return matches(to_string(level)); });
      it != log_levels.end()) {
    return *it;
  }
  // The ROS 2 spelling
  if (matches("warn")) {
    return LogLevel::Warning;
  }
  return std::nullopt;
}

std::ostream& operator<<(std::ostream& os, LogLevel level)
{
  return os << to_string(level);
}

namespace
{

// The sinks and the logger registry are created on first use and are deliberately never destroyed, so logging stays
// safe during static destruction.

// The output and the attached sinks, behind the one forwarding sink all registered loggers write to. It is rebuilt in
// one step on every change, so there is never a moment without an output.
struct Sinks
{
  std::mutex mutex;
  spdlog::sink_ptr output{ std::make_shared<detail::RcutilsSink>() };
  std::vector<spdlog::sink_ptr> attached;
  std::shared_ptr<spdlog::sinks::dist_sink_mt> forwarding{ std::make_shared<spdlog::sinks::dist_sink_mt>(
      std::vector<spdlog::sink_ptr>{ output }) };

  // Requires mutex
  void rebuild()
  {
    std::vector<spdlog::sink_ptr> all{ output };
    all.insert(all.end(), attached.begin(), attached.end());
    forwarding->set_sinks(std::move(all));
  }
};

Sinks& sinks()
{
  static Sinks& state{ *new Sinks{} };
  return state;
}

// The Default level until set_level(Default, ...) is called: DUATIC_LOG_LEVEL, read once at first use, else Info
LogLevel built_in_default_level()
{
  static const LogLevel level = [] {
    const char* value = std::getenv("DUATIC_LOG_LEVEL");
    if (value == nullptr || value[0] == '\0') {
      return LogLevel::Info;
    }
    if (const auto parsed = parse_level(value)) {
      return *parsed;
    }
    std::fprintf(stderr, "duatic_message_logger: ignoring DUATIC_LOG_LEVEL=%s\n", value);
    return LogLevel::Info;
  }();
  return level;
}

void check_name(const std::string& name)
{
  if (name.empty()) {
    throw std::invalid_argument("duatic_message_logger: logger name must not be empty");
  }
}

void check_sink(const spdlog::sink_ptr& sink)
{
  if (!sink) {
    throw std::invalid_argument("duatic_message_logger: sink must not be null");
  }
}

// The registered loggers and the level settings; every change of a setting is written into the registered loggers
class Registry
{
public:
  Registry() : default_logger_(get_logger("global_logger"))
  {
  }

  Logger get_logger(const std::string& name)
  {
    const std::lock_guard lock(mutex_);
    auto [it, inserted] = loggers_.try_emplace(name);
    if (inserted) {
      it->second = std::make_shared<spdlog::logger>(name, sinks().forwarding);
      it->second->set_level(impl::to_spdlog(resolve(name)));
    }
    return Logger(it->second);
  }

  // Not locked: like logging through it, replacing it is not synchronized
  Logger& default_logger() noexcept
  {
    return default_logger_;
  }

  void set_level(const std::string& name, LogLevel level)
  {
    const std::lock_guard lock(mutex_);
    levels_[name] = level;
    apply_levels();
  }

  void set_default_level(LogLevel level)
  {
    const std::lock_guard lock(mutex_);
    default_level_ = level;
    apply_levels();
  }

  void reset_level(const std::string& name)
  {
    const std::lock_guard lock(mutex_);
    levels_.erase(name);
    apply_levels();
  }

  void reset_default_level()
  {
    const std::lock_guard lock(mutex_);
    default_level_.reset();
    apply_levels();
  }

  [[nodiscard]] std::optional<LogLevel> level(const std::string& name) const
  {
    const std::lock_guard lock(mutex_);
    if (const auto it = levels_.find(name); it != levels_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<LogLevel> default_level() const
  {
    const std::lock_guard lock(mutex_);
    return default_level_;
  }

  [[nodiscard]] LogLevel effective_level(const std::string& name) const
  {
    const std::lock_guard lock(mutex_);
    return resolve(name);
  }

private:
  // Requires mutex_. The most specific setting of the name or one of its ancestors, else the Default level
  [[nodiscard]] LogLevel resolve(std::string_view name) const
  {
    while (true) {
      if (const auto it = levels_.find(std::string(name)); it != levels_.end()) {
        return it->second;
      }
      const auto dot = name.rfind('.');
      if (dot == std::string_view::npos) {
        return default_level_.value_or(built_in_default_level());
      }
      name = name.substr(0, dot);
    }
  }

  // Requires mutex_. Writes the resolved levels into all registered loggers, after a level setting changed
  void apply_levels()
  {
    for (const auto& [name, logger] : loggers_) {
      logger->set_level(impl::to_spdlog(resolve(name)));
    }
  }

  mutable std::mutex mutex_;
  std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> loggers_;
  std::unordered_map<std::string, LogLevel> levels_;
  std::optional<LogLevel> default_level_;
  Logger default_logger_;
};

Registry& registry()
{
  static Registry& registry{ *new Registry{} };
  return registry;
}

}  // namespace

Logger::Logger(std::shared_ptr<spdlog::logger> logger) : logger_(std::move(logger))
{
  if (!logger_) {
    throw std::invalid_argument("duatic_message_logger: logger must not be null");
  }
}

namespace impl
{

LogStream::~LogStream()
{
  if (record_) {
    record_->logger->log(to_spdlog(level_), record_->stream.str());
  }
}
}  // namespace impl

Logger get_logger(const std::string& name)
{
  check_name(name);
  return registry().get_logger(name);
}
const Logger& get_default_logger()
{
  return registry().default_logger();
}
void set_default_logger(const Logger& logger)
{
  registry().default_logger() = logger;
}

void set_level(const std::string& name, LogLevel level)
{
  check_name(name);
  registry().set_level(name, level);
}
void set_level(DefaultLevelTag, LogLevel level)
{
  registry().set_default_level(level);
}
void reset_level(const std::string& name)
{
  check_name(name);
  registry().reset_level(name);
}
void reset_level(DefaultLevelTag)
{
  registry().reset_default_level();
}
std::optional<LogLevel> get_level(const std::string& name)
{
  check_name(name);
  return registry().level(name);
}
std::optional<LogLevel> get_level(DefaultLevelTag)
{
  return registry().default_level();
}
LogLevel get_effective_level(const std::string& name)
{
  check_name(name);
  return registry().effective_level(name);
}
LogLevel get_effective_level(DefaultLevelTag)
{
  return registry().default_level().value_or(built_in_default_level());
}

void set_output_sink(spdlog::sink_ptr sink)
{
  check_sink(sink);
  auto& state = sinks();
  const std::lock_guard lock(state.mutex);
  state.output = std::move(sink);
  state.rebuild();
}
void attach_sink(spdlog::sink_ptr sink)
{
  check_sink(sink);
  auto& state = sinks();
  const std::lock_guard lock(state.mutex);
  if (std::ranges::find(state.attached, sink) != state.attached.end()) {
    return;
  }
  state.attached.push_back(std::move(sink));
  state.rebuild();
}
void detach_sink(const spdlog::sink_ptr& sink)
{
  auto& state = sinks();
  const std::lock_guard lock(state.mutex);
  const auto it = std::ranges::find(state.attached, sink);
  if (it == state.attached.end()) {
    return;
  }
  state.attached.erase(it);
  state.rebuild();
}

}  // namespace duatic::message_logger
