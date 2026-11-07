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

namespace duatic::message_logger
{

/**
 * @brief Lets a repeated log message through at most once per period, on the time the caller passes in.
 *
 * Keep one per message, and one per object when several objects log the same message.
 *
 * @note Not thread safe.
 */
class LogThrottle
{
public:
  using Clock = std::chrono::steady_clock;  ///< Clock of the times passed to ready().

  /**
   * @brief Creates a throttle that lets the first message through.
   * @param period Minimum time between two messages.
   */
  explicit LogThrottle(std::chrono::nanoseconds period) noexcept : period_(period)
  {
  }

  /**
   * @brief Reports whether the message may be logged now, and if so starts a new period.
   * @param now Current time.
   * @return true on the first call and once the period has passed since the last message let through.
   */
  [[nodiscard]] bool ready(Clock::time_point now) noexcept
  {
    if (now < next_) {
      return false;
    }
    next_ = now + period_;
    return true;
  }

private:
  std::chrono::nanoseconds period_;
  Clock::time_point next_{ Clock::time_point::min() };  ///< Earliest time the next message passes
};

}  // namespace duatic::message_logger
