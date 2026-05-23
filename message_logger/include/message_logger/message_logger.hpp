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

#include <iomanip>

#include "duatic_message_logger/logging.hpp"

// Wrapper to provide the old MELO_LOG style api
// See https://github.com/leggedrobotics/message_logger

// NOTE we only implement a subset of the API but the most commonly used one
// The _throttle_ variants are just redirecting to the other versions

#define MELO_DEBUG(...) duatic::message_logger::debug(__VA_ARGS__);
#define MELO_DEBUG_STREAM(message) (duatic::message_logger::debug() << message);
#define MELO_DEBUG_THROTTLE_STREAM(rate, message)                                                                      \
  do {                                                                                                                 \
    static auto last_log_time = std::chrono::steady_clock::time_point::min();                                          \
    const auto now = std::chrono::steady_clock::now();                                                                 \
    if (now - last_log_time >= std::chrono::duration<double>(rate)) {                                                  \
      last_log_time = now;                                                                                             \
      MELO_DEBUG_STREAM(message);                                                                                      \
    }                                                                                                                  \
  } while (0);

#define MELO_INFO(...) duatic::message_logger::info(__VA_ARGS__);
#define MELO_INFO_STREAM(message) (duatic::message_logger::info() << message);
#define MELO_INFO_THROTTLE_STREAM(rate, message)                                                                       \
  do {                                                                                                                 \
    static auto last_log_time = std::chrono::steady_clock::time_point::min();                                          \
    const auto now = std::chrono::steady_clock::now();                                                                 \
    if (now - last_log_time >= std::chrono::duration<double>(rate)) {                                                  \
      last_log_time = now;                                                                                             \
      MELO_INFO_STREAM(message);                                                                                       \
    }                                                                                                                  \
  } while (0);

#define MELO_WARN(...) duatic::message_logger::warning(__VA_ARGS__);
#define MELO_WARN_STREAM(message) (duatic::message_logger::warning() << message);
#define MELO_WARN_THROTTLE_STREAM(rate, message)                                                                       \
  do {                                                                                                                 \
    static auto last_log_time = std::chrono::steady_clock::time_point::min();                                          \
    const auto now = std::chrono::steady_clock::now();                                                                 \
    if (now - last_log_time >= std::chrono::duration<double>(rate)) {                                                  \
      last_log_time = now;                                                                                             \
      MELO_WARN_STREAM(message);                                                                                       \
    }                                                                                                                  \
  } while (0);

#define MELO_ERROR(...) duatic::message_logger::error(__VA_ARGS__);
#define MELO_ERROR_STREAM(message) (duatic::message_logger::error() << message);
#define MELO_ERROR_THROTTLE_STREAM(rate, message)                                                                      \
  do {                                                                                                                 \
    static auto last_log_time = std::chrono::steady_clock::time_point::min();                                          \
    const auto now = std::chrono::steady_clock::now();                                                                 \
    if (now - last_log_time >= std::chrono::duration<double>(rate)) {                                                  \
      last_log_time = now;                                                                                             \
      MELO_ERROR_STREAM(message);                                                                                      \
    }                                                                                                                  \
  } while (0);

#define MELO_FATAL(...) duatic::message_logger::error(__VA_ARGS__);
#define MELO_FATAL_STREAM(message) (duatic::message_logger::error() << message);
#define MELO_FATAL_THROTTLE_STREAM(rate, message)                                                                      \
  do {                                                                                                                 \
    static auto last_log_time = std::chrono::steady_clock::time_point::min();                                          \
    const auto now = std::chrono::steady_clock::now();                                                                 \
    if (now - last_log_time >= std::chrono::duration<double>(rate)) {                                                  \
      last_log_time = now;                                                                                             \
      MELO_FATAL_STREAM(message);                                                                                      \
    }                                                                                                                  \
  } while (0);

namespace message_logger::log::levels
{
using Level = duatic::message_logger::LogLevel;
}
