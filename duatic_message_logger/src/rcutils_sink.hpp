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

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "spdlog/details/log_msg.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace duatic::message_logger::detail
{
/**
 * @brief Writes through the ROS 2 logging system when the process has it loaded, to stdout otherwise
 *
 * The rcutils C functions are looked up by name at runtime, so the logger has no build or link dependency on ROS.
 * Until they are found, each message checks again, so a process that loads ROS later switches over.
 */
class RcutilsSink : public spdlog::sinks::base_sink<std::mutex>
{
public:
  RcutilsSink();

protected:
  void sink_it_(const spdlog::details::log_msg& msg) override;
  void flush_() override;
  void set_pattern_(const std::string& pattern) override;
  void set_formatter_(std::unique_ptr<spdlog::formatter> sink_formatter) override;

private:
  /// @brief Layout of rcutils_log_location_t; the rosout handler reads it, so it is always passed
  struct Location
  {
    const char* function_name;
    const char* file_name;
    std::size_t line_number;
  };

  using LogFunction = void (*)(const Location* location, int severity, const char* name, const char* format, ...);
  using EnabledFunction = bool (*)(const char* name, int severity);
  using InitializeFunction = int (*)();

  /// @brief Looks up the rcutils functions and initializes rcutils logging once they are found
  void resolve();

  /// @brief Writes a message ROS 2 has filtered out, in the rcutils console layout
  static void write_fallback(const spdlog::details::log_msg& msg, const std::string& name);

  std::shared_ptr<spdlog::sinks::stdout_color_sink_st> stdout_;
  LogFunction log_{ nullptr };
  EnabledFunction enabled_{ nullptr };
};

}  // namespace duatic::message_logger::detail
