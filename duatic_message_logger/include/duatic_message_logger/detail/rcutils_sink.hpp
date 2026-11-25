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
#include <string_view>
#include <utility>

#include <rcutils/logging.h>  // NOLINT(build/include_order)

#include "spdlog/details/log_msg.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace duatic::message_logger::detail
{
/**
 * @brief Writes through the ROS 2 logging output when the process has it loaded, to stdout otherwise
 *
 * Records go straight to the current rcutils output handler (rcl's console and file outputs in a ROS 2 process),
 * without a ROS level check: the levels of our loggers are decided by duatic_message_logger alone.
 *
 * Built against the rcutils headers, but the functions are looked up by name at runtime, so the library neither links
 * rcutils nor needs it installed.
 * The lookup happens once, at the first record, so the logging path never touches the dynamic loader afterwards; a
 * process that loads ROS only after its first record keeps writing to stdout.
 */
class RcutilsSink : public spdlog::sinks::base_sink<std::mutex>
{
public:
  RcutilsSink();

  /// @brief Writes the records under the ROS logger @p ros_logger, each with its own logger name in front of the
  /// message; an empty name writes them under their own names
  void set_ros_logger(std::string ros_logger);

protected:
  void sink_it_(const spdlog::details::log_msg& msg) override;
  void flush_() override;
  void set_pattern_(const std::string& pattern) override;
  void set_formatter_(std::unique_ptr<spdlog::formatter> sink_formatter) override;

private:
  using RcutilsGetOutputHandler = decltype(&rcutils_logging_get_output_handler);
  using RcutilsInitialize = decltype(&rcutils_logging_initialize);

  /// @brief Looks up the rcutils functions and initializes rcutils logging if they are found
  void resolve();

  std::shared_ptr<spdlog::sinks::stdout_color_sink_st> stdout_;
  // Looked up per record, as rcl installs its output handler at initialization, possibly after the lookup
  RcutilsGetOutputHandler get_output_handler_{ nullptr };
  bool resolved_{ false };  ///< Whether the lookup happened
  std::string ros_logger_;  ///< Guarded by mutex_
};

}  // namespace duatic::message_logger::detail
