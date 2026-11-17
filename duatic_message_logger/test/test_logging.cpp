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
#include <gtest/gtest.h>

#include <sstream>
#include <stdexcept>

#include "duatic_message_logger/logging.hpp"

#include "spdlog/sinks/ostream_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

TEST(Logging, DefaultOutput)
{
  // Test stream like usage
  duatic::message_logger::debug() << "Test debug"
                                  << "test" << std::endl;
  duatic::message_logger::info() << "Test info";
  duatic::message_logger::warning() << "Test warning";
  duatic::message_logger::error() << "Test error";
  duatic::message_logger::fatal() << "Test fatal";

  // Test std::hex
  duatic::message_logger::info() << "Test hex: 0x" << std::hex << 834;
  // Test fmt style usage
  duatic::message_logger::debug("Debug {}", 1);
  duatic::message_logger::info("Info {}", 1);
  duatic::message_logger::warning("Warning {}", 1);
  duatic::message_logger::error("Error {}", 1);
  duatic::message_logger::fatal("Fatal {}", 1);
}

TEST(Logging, StandardOut)
{
  // Reconfigure to use a different output sink
  auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  // ROS2-like pattern:
  // %^   = start color
  // %l   = log level
  // %$   = reset color
  // %n   = logger name (node name)
  // %v   = message
  // %Y-%m-%d %H:%M:%S.%f = timestamp with microseconds
  sink->set_color_mode(spdlog::color_mode::always);
  sink->set_pattern("[%^%l%$] [%Y-%m-%d %H:%M:%S.%e] [%n]: %v");
  const duatic::message_logger::Logger logger{ std::make_shared<spdlog::logger>("test", sink) };
  duatic::message_logger::set_default_logger(logger);

  duatic::message_logger::debug() << "Test debug"
                                  << "test" << std::endl;
  duatic::message_logger::info() << "Test info";
  duatic::message_logger::warning() << "Test warning";
  duatic::message_logger::error() << "Test error";
  duatic::message_logger::fatal() << "Test fatal";

  // Restore the default logger for the other tests
  duatic::message_logger::set_default_logger(duatic::message_logger::get_logger("global_logger"));
}

TEST(Logging, ExplicitLogger)
{
  // Reconfigure to use a different output sink
  auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  // ROS2-like pattern:
  // %^   = start color
  // %l   = log level
  // %$   = reset color
  // %n   = logger name (node name)
  // %v   = message
  // %Y-%m-%d %H:%M:%S.%f = timestamp with microseconds
  sink->set_color_mode(spdlog::color_mode::always);
  sink->set_pattern("[%^%l%$] [%Y-%m-%d %H:%M:%S.%e] [%n]: %v");
  const duatic::message_logger::Logger logger{ std::make_shared<spdlog::logger>("custom", sink) };

  // ftm style
  duatic::message_logger::debug(logger, "Debug {}", 1);
  duatic::message_logger::info(logger, "Info {}", 1);
  duatic::message_logger::warning(logger, "Warning {}", 1);
  duatic::message_logger::error(logger, "Error {}", 1);
  duatic::message_logger::fatal(logger, "Fatal {}", 1);

  // stream style
  duatic::message_logger::debug(logger) << "Test debug"
                                        << "test" << std::endl;
  duatic::message_logger::info(logger) << "Test info (stream)";
  duatic::message_logger::warning(logger) << "Test warning";
  duatic::message_logger::error(logger) << "Test error";
  duatic::message_logger::fatal(logger) << "Test fatal";
}
TEST(Logging, LoggerWithName)
{
  auto logger = duatic::message_logger::get_logger("test_with_name");

  duatic::message_logger::debug(logger) << "Test debug" << logger.name() << std::endl;
  duatic::message_logger::info(logger) << "Test info " << logger.name();
  duatic::message_logger::warning(logger) << "Test warning";
  duatic::message_logger::error(logger) << "Test error";
  duatic::message_logger::fatal(logger) << "Test fatal";
}

// Test the convenience wrapper
#include "duatic_message_logger/log.hpp"

TEST(Logging, Simple)
{
  logging::info() << "Test" << std::endl;
}

namespace
{
// Records everything written to the default sink as "<logger> <level>: <message>" lines, while it lives
class Capture
{
public:
  Capture() : sink_(std::make_shared<spdlog::sinks::ostream_sink_mt>(out_))
  {
    sink_->set_pattern("%n %l: %v");
    logging::attach_sink(sink_);
  }
  ~Capture()
  {
    logging::detach_sink(sink_);
  }
  Capture(const Capture&) = delete;
  Capture& operator=(const Capture&) = delete;

  [[nodiscard]] std::string str() const
  {
    return out_.str();
  }
  [[nodiscard]] bool contains(const std::string& text) const
  {
    return out_.str().find(text) != std::string::npos;
  }

private:
  std::ostringstream out_;
  std::shared_ptr<spdlog::sinks::ostream_sink_mt> sink_;
};

// Counts how often it is streamed
struct Counted
{
  int* calls;
};
std::ostream& operator<<(std::ostream& os, const Counted& counted)
{
  ++*counted.calls;
  return os;
}
}  // namespace

TEST(Logging, SetOutputSinkRedirectsExistingLoggers)
{
  auto logger = logging::get_logger("before_redirect");
  std::ostringstream out;
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(out);
  sink->set_pattern("%n: %v");
  logging::set_output_sink(sink);

  logging::info(logger, "redirected {}", 1);
  EXPECT_NE(out.str().find("before_redirect: redirected 1"), std::string::npos) << out.str();

  logging::set_output_sink(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
}

TEST(Logging, SetOutputSinkKeepsAttachedSinks)
{
  auto logger = logging::get_logger("kept_attached");
  const Capture capture;
  std::ostringstream out;
  logging::set_output_sink(std::make_shared<spdlog::sinks::ostream_sink_mt>(out));

  logging::info(logger, "still attached");
  EXPECT_TRUE(capture.contains("kept_attached info: still attached")) << capture.str();
  EXPECT_NE(out.str().find("still attached"), std::string::npos) << out.str();

  logging::set_output_sink(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
}

TEST(Logging, AttachSinkReceivesRecordsOfRegisteredLoggers)
{
  auto logger = logging::get_logger("before_attach");
  const Capture capture;
  auto later_logger = logging::get_logger("after_attach");

  logging::info(logger, "named {}", 1);
  logging::info(later_logger, "named {}", 2);
  EXPECT_TRUE(capture.contains("before_attach info: named 1")) << capture.str();
  EXPECT_TRUE(capture.contains("after_attach info: named 2")) << capture.str();
}

TEST(Logging, AttachingTwiceDeliversOnce)
{
  auto logger = logging::get_logger("attached_twice");
  std::ostringstream out;
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(out);
  sink->set_pattern("%v");
  logging::attach_sink(sink);
  logging::attach_sink(sink);

  logging::info(logger, "once");
  EXPECT_EQ(out.str(), "once\n");

  logging::detach_sink(sink);
}

TEST(Logging, DetachSinkStopsDelivery)
{
  auto logger = logging::get_logger("detached");
  std::ostringstream out;
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(out);
  logging::attach_sink(sink);
  logging::detach_sink(sink);
  EXPECT_NO_THROW(logging::detach_sink(sink));

  logging::info(logger, "not delivered");
  EXPECT_TRUE(out.str().empty()) << out.str();
}

TEST(Logging, DefaultLevelAppliesToExistingLoggers)
{
  auto logger = logging::get_logger("default_level");
  EXPECT_FALSE(logging::get_level(logging::Default).has_value());
  EXPECT_EQ(logging::get_effective_level(logging::Default), logging::LogLevel::Info);
  EXPECT_FALSE(logger.should_log(logging::LogLevel::Debug));

  logging::set_level(logging::Default, logging::LogLevel::Debug);
  EXPECT_EQ(logging::get_level(logging::Default), logging::LogLevel::Debug);
  EXPECT_TRUE(logger.should_log(logging::LogLevel::Debug));
  {
    const Capture capture;
    logging::debug(logger, "now visible");
    EXPECT_TRUE(capture.contains("default_level debug: now visible")) << capture.str();
  }

  logging::reset_level(logging::Default);
  EXPECT_EQ(logging::get_effective_level(logging::Default), logging::LogLevel::Info);
  EXPECT_FALSE(logger.should_log(logging::LogLevel::Debug));
}

TEST(Logging, NameSettingCoversNamesBelowIt)
{
  auto parent = logging::get_logger("hier");
  auto child = logging::get_logger("hier.child");
  auto sibling = logging::get_logger("hierx");

  logging::set_level("hier", logging::LogLevel::Debug);
  EXPECT_TRUE(parent.should_log(logging::LogLevel::Debug));
  EXPECT_TRUE(child.should_log(logging::LogLevel::Debug));
  EXPECT_FALSE(sibling.should_log(logging::LogLevel::Debug));
  EXPECT_EQ(logging::get_effective_level("hier.child.grandchild"), logging::LogLevel::Debug);

  logging::set_level("hier.child", logging::LogLevel::Warning);
  EXPECT_FALSE(child.should_log(logging::LogLevel::Info));
  EXPECT_TRUE(parent.should_log(logging::LogLevel::Debug));
  EXPECT_EQ(logging::get_level("hier.child"), logging::LogLevel::Warning);

  logging::reset_level("hier.child");
  EXPECT_FALSE(logging::get_level("hier.child").has_value());
  EXPECT_TRUE(child.should_log(logging::LogLevel::Debug));

  logging::reset_level("hier");
  EXPECT_FALSE(child.should_log(logging::LogLevel::Debug));
}

TEST(Logging, NameSettingAppliesToLoggersCreatedLater)
{
  logging::set_level("created_later", logging::LogLevel::Error);
  auto logger = logging::get_logger("created_later");
  EXPECT_FALSE(logger.should_log(logging::LogLevel::Warning));
  EXPECT_TRUE(logger.should_log(logging::LogLevel::Error));
  logging::reset_level("created_later");
}

TEST(Logging, DefaultLevelIsNotTheDefaultLoggersLevel)
{
  logging::set_level("global_logger", logging::LogLevel::Warning);
  logging::set_level(logging::Default, logging::LogLevel::Debug);
  EXPECT_EQ(logging::get_effective_level("global_logger"), logging::LogLevel::Warning);
  logging::reset_level(logging::Default);
  logging::reset_level("global_logger");
}

TEST(Logging, HandlesOfOneNameShareTheirLevel)
{
  auto first = logging::get_logger("shared");
  auto second = logging::get_logger("shared");
  logging::set_level("shared", logging::LogLevel::Debug);
  EXPECT_TRUE(first.should_log(logging::LogLevel::Debug));
  EXPECT_TRUE(second.should_log(logging::LogLevel::Debug));
  logging::reset_level("shared");
}

TEST(Logging, CustomLoggerIgnoresLevelSettings)
{
  std::ostringstream out;
  const logging::Logger custom{ std::make_shared<spdlog::logger>(
      "custom_levels", std::make_shared<spdlog::sinks::ostream_sink_mt>(out)) };
  logging::set_level("custom_levels", logging::LogLevel::Debug);
  logging::set_level(logging::Default, logging::LogLevel::Debug);
  EXPECT_FALSE(custom.should_log(logging::LogLevel::Debug));
  logging::reset_level(logging::Default);
  logging::reset_level("custom_levels");
}

TEST(Logging, MovedFromLoggerStaysUsable)
{
  auto logger = logging::get_logger("moved");
  const auto moved = std::move(logger);
  EXPECT_EQ(moved.name(), "moved");
  EXPECT_EQ(logger.name(), "moved");  // NOLINT(bugprone-use-after-move): moving copies
}

TEST(Logging, InvalidArgumentsThrow)
{
  EXPECT_THROW((void)logging::get_logger(""), std::invalid_argument);
  EXPECT_THROW(logging::set_level("", logging::LogLevel::Info), std::invalid_argument);
  EXPECT_THROW(logging::set_level({}, logging::LogLevel::Info), std::invalid_argument);
  EXPECT_THROW(logging::reset_level(""), std::invalid_argument);
  EXPECT_THROW((void)logging::get_level(""), std::invalid_argument);
  EXPECT_THROW((void)logging::get_effective_level(""), std::invalid_argument);
  EXPECT_THROW(logging::Logger{ nullptr }, std::invalid_argument);
  EXPECT_THROW(logging::attach_sink(nullptr), std::invalid_argument);
  EXPECT_THROW(logging::set_output_sink(nullptr), std::invalid_argument);
}

TEST(Logging, DisabledStreamDoesNotFormat)
{
  auto logger = logging::get_logger("stream_disabled");
  int calls = 0;
  logging::debug(logger) << Counted{ &calls };
  EXPECT_EQ(calls, 0);
  logging::info(logger) << Counted{ &calls };
  EXPECT_EQ(calls, 1);
}

TEST(Logging, MemberCallsLogThroughTheLogger)
{
  std::ostringstream out;
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(out);
  sink->set_pattern("%v");
  const logging::Logger logger{ std::make_shared<spdlog::logger>("members", sink) };

  logger.info("a {}", 1);
  logger.warning() << "b " << 2;
  logger.log(logging::LogLevel::Error, "c");
  EXPECT_EQ(out.str(), "a 1\nb 2\nc\n");
}

TEST(Logging, DisabledMemberStreamDoesNotFormat)
{
  const auto logger = logging::get_logger("member_stream_disabled");
  int calls = 0;
  logger.debug() << Counted{ &calls };
  EXPECT_EQ(calls, 0);
  logger.info() << Counted{ &calls };
  EXPECT_EQ(calls, 1);
}

TEST(Logging, StreamKeepsItsLoggerAlive)
{
  std::ostringstream out;
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(out);
  sink->set_pattern("%v");
  {
    // The temporary handle is gone before the record is written
    auto stream = logging::Logger{ std::make_shared<spdlog::logger>("temporary", sink) }.info();
    stream << "after " << 1;
  }
  EXPECT_EQ(out.str(), "after 1\n");
}

TEST(Logging, MemberCallsFollowLevelSettings)
{
  const auto logger = logging::get_logger("member_levels");
  const Capture capture;
  logger.debug("hidden");
  EXPECT_FALSE(capture.contains("hidden")) << capture.str();

  logging::set_level("member_levels", logging::LogLevel::Debug);
  logger.debug("shown {}", 1);
  EXPECT_TRUE(capture.contains("member_levels debug: shown 1")) << capture.str();

  logging::reset_level("member_levels");
}

TEST(Logging, ParseLevelAcceptsPrintedNamesAndWarn)
{
  EXPECT_EQ(logging::parse_level("debug"), logging::LogLevel::Debug);
  EXPECT_EQ(logging::parse_level("Warning"), logging::LogLevel::Warning);
  EXPECT_EQ(logging::parse_level("warn"), logging::LogLevel::Warning);
  EXPECT_EQ(logging::parse_level("WARN"), logging::LogLevel::Warning);
  EXPECT_EQ(logging::parse_level("fatal"), logging::LogLevel::Fatal);
  EXPECT_FALSE(logging::parse_level("trace").has_value());
  EXPECT_FALSE(logging::parse_level("").has_value());
}

TEST(Logging, LevelNamesRoundTrip)
{
  for (const auto level : logging::log_levels) {
    EXPECT_EQ(logging::parse_level(logging::to_string(level)), level) << logging::to_string(level);
  }
  static_assert(logging::to_string(logging::LogLevel::Warning) == "warning");
}

TEST(Logging, StreamedLevelIsItsName)
{
  for (const auto level : logging::log_levels) {
    std::ostringstream out;
    out << level;
    EXPECT_EQ(out.str(), logging::to_string(level));
  }
}

TEST(Logging, ThrottledMessageGoesOutFirstThenOncePerPeriod)
{
  std::ostringstream out;
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(out);
  sink->set_pattern("%v");
  const logging::Logger logger{ std::make_shared<spdlog::logger>("throttled", sink) };

  const logging::LogThrottle::Clock::time_point start{};
  const std::chrono::milliseconds ms{ 1 };
  logging::LogThrottle throttle{ 1000 * ms };
  for (const auto t : { 0, 999, 1000, 1500, 2500 }) {
    logging::warning(throttle, start + t * ms, logger, "at {}", t);
  }
  EXPECT_EQ(out.str(), "at 0\nat 1000\nat 2500\n");
}

TEST(Logging, LogOnceMessageGoesOutOnlyOnce)
{
  std::ostringstream out;
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(out);
  sink->set_pattern("%v");
  const logging::Logger logger{ std::make_shared<spdlog::logger>("once", sink) };

  logging::LogOnce once;
  for (const auto i : { 0, 1, 2 }) {
    logging::warning(once, logger, "call {}", i);
  }
  EXPECT_EQ(out.str(), "call 0\n");
}

TEST(Logging, DisabledOnceAndThrottledMessagesAppearAfterRaisingTheLevel)
{
  auto logger = logging::get_logger("raised_later");
  const Capture capture;
  logging::LogOnce once;
  logging::LogThrottle throttle{ std::chrono::seconds(1) };
  const logging::LogThrottle::Clock::time_point start{};

  logging::debug(once, logger, "once");
  logging::debug(throttle, start, logger, "throttled");
  EXPECT_TRUE(capture.str().empty()) << capture.str();

  logging::set_level("raised_later", logging::LogLevel::Debug);
  logging::debug(once, logger, "once");
  logging::debug(throttle, start + std::chrono::milliseconds(1), logger, "throttled");
  EXPECT_TRUE(capture.contains("raised_later debug: once")) << capture.str();
  EXPECT_TRUE(capture.contains("raised_later debug: throttled")) << capture.str();

  logging::reset_level("raised_later");
}
