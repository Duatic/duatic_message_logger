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

// Python bindings of duatic_message_logger. No Python code runs on a logging thread: log calls release the GIL before
// they reach the sinks, and the QueueSink for GUIs is plain C++, so a C++ thread never waits for the GIL.

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "duatic_message_logger/logging.hpp"
#include "spdlog/sinks/base_sink.h"

namespace py = pybind11;

namespace duatic::message_logger::python
{
namespace
{

LogLevel from_spdlog(spdlog::level::level_enum level)
{
  switch (level) {
    case spdlog::level::trace:
    case spdlog::level::debug:
      return LogLevel::Debug;
    case spdlog::level::warn:
      return LogLevel::Warning;
    case spdlog::level::err:
      return LogLevel::Error;
    case spdlog::level::critical:
      return LogLevel::Fatal;
    default:
      return LogLevel::Info;
  }
}

/// A record as a QueueSink hands it to Python.
struct LogRecord
{
  LogLevel level;
  std::string logger_name;
  std::string message;
  std::int64_t timestamp_ns;  ///< Since the Unix epoch
};

/**
 * Keeps the records for a GUI, which drains them from its own timer. When full, it drops the oldest record.
 *
 * It holds no Python objects, so it stays safe to call from any thread, also after the interpreter is gone.
 */
class QueueSink : public spdlog::sinks::base_sink<std::mutex>
{
public:
  explicit QueueSink(std::size_t capacity) : capacity_(capacity)
  {
    if (capacity_ == 0) {
      throw std::invalid_argument("duatic_message_logger: QueueSink capacity must not be 0");
    }
  }

  /// Returns the records collected since the last call, oldest first.
  std::vector<LogRecord> drain()
  {
    std::deque<LogRecord> records;
    {
      const std::lock_guard lock(mutex_);
      records.swap(records_);
    }
    return { std::make_move_iterator(records.begin()), std::make_move_iterator(records.end()) };
  }

  /// Returns how many records were dropped since the sink was created, because it was full.
  std::uint64_t dropped()
  {
    const std::lock_guard lock(mutex_);
    return dropped_;
  }

protected:
  void sink_it_(const spdlog::details::log_msg& msg) override
  {
    if (msg.level == spdlog::level::off) {
      return;
    }
    if (records_.size() == capacity_) {
      records_.pop_front();
      ++dropped_;
    }
    records_.push_back({ from_spdlog(msg.level), std::string(msg.logger_name.begin(), msg.logger_name.end()),
                         std::string(msg.payload.begin(), msg.payload.end()),
                         std::chrono::duration_cast<std::chrono::nanoseconds>(msg.time.time_since_epoch()).count() });
  }
  void flush_() override
  {
  }

private:
  std::size_t capacity_;
  std::deque<LogRecord> records_;
  std::uint64_t dropped_{ 0 };
};

/// Formats @p message with str.format, only when there are arguments, and logs it without the GIL.
void log_message(const Logger& logger, LogLevel level, const py::object& message, const py::args& args,
                 const py::kwargs& kwargs)
{
  if (!logger.should_log(level)) {
    return;
  }
  py::str text(message);
  if (!args.empty() || !kwargs.empty()) {
    text = py::str(text.attr("format")(*args, **kwargs));
  }
  const auto utf8 = text.cast<std::string>();
  const py::gil_scoped_release release;
  logger.log(level, "{}", utf8);
}

template <LogLevel Level>
void log_default(const py::object& message, const py::args& args, const py::kwargs& kwargs)
{
  log_message(get_default_logger(), Level, message, args, kwargs);
}

template <LogLevel Level>
void log_with(const Logger& logger, const py::object& message, const py::args& args, const py::kwargs& kwargs)
{
  log_message(logger, Level, message, args, kwargs);
}

}  // namespace
}  // namespace duatic::message_logger::python

PYBIND11_MODULE(_bindings, m)
{
  using namespace duatic::message_logger;          // NOLINT(build/namespaces)
  using namespace duatic::message_logger::python;  // NOLINT(build/namespaces)

  m.doc() = "Python bindings of duatic_message_logger";

  py::enum_<LogLevel>(m, "LogLevel", "The log levels, the same as in ROS 2.")
      .value("Debug", LogLevel::Debug)
      .value("Info", LogLevel::Info)
      .value("Warning", LogLevel::Warning)
      .value("Error", LogLevel::Error)
      .value("Fatal", LogLevel::Fatal);

  py::tuple levels(log_levels.size());
  for (std::size_t i = 0; i < log_levels.size(); ++i) {
    levels[i] = py::cast(log_levels[i]);
  }
  m.attr("log_levels") = levels;

  m.def(
      "to_string", [](LogLevel level) { return std::string(to_string(level)); }, py::arg("level"),
      "Returns the name of the level as parse_level() accepts it: debug, info, warning, error or fatal.");
  m.def("parse_level", &parse_level, py::arg("text"),
        "Returns the level named text (case insensitive, also 'warn'), or None if it names none.");

  py::class_<DefaultLevelTag>(m, "DefaultLevelTag", "The type of Default.");
  m.attr("Default") = py::cast(Default);

  py::class_<Logger>(m, "Logger", "Handle to a registered logger; copies refer to the same logger.")
      .def_property_readonly("name", &Logger::name)
      .def("should_log", &Logger::should_log, py::arg("level"),
           "Returns whether a record of the level passes the logger's level.")
      .def("log", &log_message, py::arg("level"), py::arg("message"),
           "Logs message.format(*args, **kwargs) at the level; formats only if the level passes.")
      .def("debug", &log_with<LogLevel::Debug>, py::arg("message"))
      .def("info", &log_with<LogLevel::Info>, py::arg("message"))
      .def("warning", &log_with<LogLevel::Warning>, py::arg("message"))
      .def("error", &log_with<LogLevel::Error>, py::arg("message"))
      .def("fatal", &log_with<LogLevel::Fatal>, py::arg("message"))
      .def("__repr__", [](const Logger& logger) { return "<Logger '" + logger.name() + "'>"; });

  m.def(
      "log",
      [](LogLevel level, const py::object& message, const py::args& args, const py::kwargs& kwargs) {
        log_message(get_default_logger(), level, message, args, kwargs);
      },
      py::arg("level"), py::arg("message"), "Logs through the default logger, like Logger.log().");
  m.def("debug", &log_default<LogLevel::Debug>, py::arg("message"));
  m.def("info", &log_default<LogLevel::Info>, py::arg("message"));
  m.def("warning", &log_default<LogLevel::Warning>, py::arg("message"));
  m.def("error", &log_default<LogLevel::Error>, py::arg("message"));
  m.def("fatal", &log_default<LogLevel::Fatal>, py::arg("message"));

  m.def("get_logger", &get_logger, py::arg("name"),
        "Returns the registered logger of the name, creating it on first use; the C++ code shares it.");
  m.def(
      "get_default_logger", []() { return get_default_logger(); },
      "Returns the default logger, which the calls without a logger use.");
  m.def("set_default_logger", &set_default_logger, py::arg("logger"),
        "Sets the default logger; call it before other threads log through the default logger.");

  m.def("set_level", py::overload_cast<const std::string&, LogLevel>(&set_level), py::arg("name"), py::arg("level"),
        "Sets the level of the name and of every name below it.");
  m.def("set_level", py::overload_cast<DefaultLevelTag, LogLevel>(&set_level), py::arg("tag"), py::arg("level"),
        "Sets the Default level, used by the registered loggers that no name setting covers.");
  m.def("reset_level", py::overload_cast<const std::string&>(&reset_level), py::arg("name"),
        "Removes the setting of the name, so it inherits again.");
  m.def("reset_level", py::overload_cast<DefaultLevelTag>(&reset_level), py::arg("tag"),
        "Removes the Default level setting, which brings back Info.");
  m.def("get_level", py::overload_cast<const std::string&>(&get_level), py::arg("name"),
        "Returns the level set for exactly the name, or None.");
  m.def("get_level", py::overload_cast<DefaultLevelTag>(&get_level), py::arg("tag"),
        "Returns the Default level, or None if none was set.");
  m.def("get_effective_level", py::overload_cast<const std::string&>(&get_effective_level), py::arg("name"),
        "Returns the level a registered logger of the name uses.");
  m.def("get_effective_level", py::overload_cast<DefaultLevelTag>(&get_effective_level), py::arg("tag"),
        "Returns the Default level in force: the one set, or Info.");

  py::class_<LogRecord>(m, "LogRecord", "A record as QueueSink.drain() returns it.")
      .def_readonly("level", &LogRecord::level)
      .def_readonly("logger_name", &LogRecord::logger_name)
      .def_readonly("message", &LogRecord::message)
      .def_readonly("timestamp_ns", &LogRecord::timestamp_ns, "Time of the record, in ns since the Unix epoch.")
      .def("__repr__", [](const LogRecord& record) {
        return "<LogRecord " + std::string(to_string(record.level)) + " '" + record.logger_name + "': " +
               record.message + ">";
      });

  py::class_<QueueSink, std::shared_ptr<QueueSink>>(
      m, "QueueSink", "Keeps the records for a GUI, which drains them from its own timer; drops the oldest when full.")
      .def(py::init<std::size_t>(), py::arg("capacity") = 10000)
      .def("drain", &QueueSink::drain, py::call_guard<py::gil_scoped_release>(),
           "Returns the records collected since the last call, oldest first.")
      .def_property_readonly("dropped", &QueueSink::dropped,
                             "How many records were dropped since the sink was created, because it was full.");

  m.def(
      "attach_sink", [](const std::shared_ptr<QueueSink>& sink) { attach_sink(sink); }, py::arg("sink"),
      "Also forwards every record of the registered loggers to the sink; attaching it twice has no effect.");
  m.def(
      "detach_sink", [](const std::shared_ptr<QueueSink>& sink) { detach_sink(sink); }, py::arg("sink"),
      "Stops forwarding to the sink; once this returns, it receives no further records.");
}
