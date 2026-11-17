# duatic_message_logger

A modern c++20 logging solution on spdlog, with no runtime ROS dependency, that logs through ROS 2 when the process uses it.

[![Humble](https://github.com/Duatic/duatic_message_logger/raw/badges/humble.svg)](https://github.com/Duatic/duatic_message_logger/actions/workflows/ci.yml)
[![Jazzy](https://github.com/Duatic/duatic_message_logger/raw/badges/jazzy.svg)](https://github.com/Duatic/duatic_message_logger/actions/workflows/ci.yml)
[![Kilted](https://github.com/Duatic/duatic_message_logger/raw/badges/kilted.svg)](https://github.com/Duatic/duatic_message_logger/actions/workflows/ci.yml)
[![Lyrical](https://github.com/Duatic/duatic_message_logger/raw/badges/lyrical.svg)](https://github.com/Duatic/duatic_message_logger/actions/workflows/ci.yml)
[![Rolling](https://github.com/Duatic/duatic_message_logger/raw/badges/rolling.svg)](https://github.com/Duatic/duatic_message_logger/actions/workflows/ci.yml)


It provides a compat header with the old `MELO` style logging macros.

# License

The contents are licensed under the BSD-3-Clause  [license](LICENSE).\
Images in this repository are to be licensed separately if you want to use them for any other usecase than forking this repository. Please open an issue in order to get in touch with us.

# Dependencies

All dependencies with their corresponding version are listed in the [repos.list](./repos.list).

| Name | Description | License
| ---  | --- | --- |
| [spdglog](https://github.com/gabime/spdlog) | A very versatile c++ logging library | [MIT](https://github.com/gabime/spdlog/blob/v1.x/LICENSE) |

# Usage

## 1. global logger streaming api

```c++
#include "duatic_message_logger/log.hpp"

...

logging::debug() << "Debug " << 1;
logging::info() << "Info " << 1;
logging::warning() << "Warning " << 1;
logging::error() << "Error " << 1;
logging::fatal() << "Fatal " << 1;

// level chosen at runtime
logging::log(logging::LogLevel::Info) << "Info " << 1;
```

A record is one line; do not end it with `std::endl`, which adds an empty line.

## 2. global logger fmt style api

See [fmt](https://github.com/fmtlib/fmt) for all formatting options

```c++
#include "duatic_message_logger/log.hpp"

...

logging::debug("Debug {}", 1);
logging::info("Info {}", 1);
logging::warning("Warning {}", 1);
logging::error("Error {}", 1);
logging::fatal("Fatal {}", 1);

// level chosen at runtime
logging::log(logging::LogLevel::Info, "Info {}", 1);
```

## 3. named logger

`get_logger(name)` returns the registered logger of that name; the same name always gives the same logger. It writes
to the output and the attached sinks, and its level follows the level settings (see 8.).

```c++
#include "duatic_message_logger/log.hpp"

...
  // Copies share the logger; keep one as a member of your component
  logging::Logger logger = logging::get_logger("duatic.my_component");

  // fmt style, see https://github.com/fmtlib/fmt for all formatting options
  logger.debug("Debug {}", 1);
  logger.info("Info {}", 1);
  logger.warning("Warning {}", 1);
  logger.error("Error {}", 1);
  logger.fatal("Fatal {}", 1);
  logger.log(logging::LogLevel::Info, "Info {}", 1);  // level chosen at runtime

  // or streaming style, also debug() ... fatal() and log(level)
  logger.warning() << "Warning " << 1;

  // a runtime string is passed as an argument, never as the format
  const std::string message = read_message();
  logger.info("{}", message);

  // skip expensive work for records that would be dropped
  if (logger.should_log(logging::LogLevel::Debug)) {
    logger.debug("State: {}", dump_state());
  }
```

The free functions take the logger as first argument and do the same, e.g. `logging::info(logger, "Info {}", 1)` or
`logging::error(logger) << "Error " << 1`. Use dot separated names (`duatic.<component>.<instance>`), so levels can be
set per subtree (see 8.).

## 4. custom logger

A `Logger` built from your own spdlog logger keeps its own sinks and its own level, which you set on the spdlog
logger; the level settings never touch it, and it writes only to its own sinks, not to the output or the attached
sinks.

```c++
#include "duatic_message_logger/log.hpp"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

...
  // Create custom sinks + logger (spdlog style)
  auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console->set_color_mode(spdlog::color_mode::always);
  console->set_pattern("[%^%l%$] [%Y-%m-%d %H:%M:%S.%e] [%n]: %v");
  auto file = std::make_shared<spdlog::sinks::basic_file_sink_mt>("/tmp/custom.log");

  // Keep the spdlog logger to change its level later
  auto spdlog_logger = std::make_shared<spdlog::logger>("custom", spdlog::sinks_init_list{ console, file });
  spdlog_logger->set_level(spdlog::level::debug);
  logging::Logger logger{ spdlog_logger };

  logger.debug("Debug {}", 1);
  logger.fatal() << "Fatal " << 1;
```

### The default logger

The calls without a logger (`logging::info(...)`, `logging::info() << ...`) use the default logger, a registered
logger named `global_logger`. A program can make another logger the default, at startup before other threads log:

```c++
  // A registered logger: the default logger is now "duatic.my_tool"
  logging::set_default_logger(logging::get_logger("duatic.my_tool"));
  logging::info("Info {}", 1);  // logged by duatic.my_tool

  // Or the custom logger from above, with its own sinks and level
  logging::set_default_logger(logger);

  const logging::Logger& current = logging::get_default_logger();
```

## 5. Throttled and one time messages

Each fmt style call with a logger also takes a `LogThrottle` and the current time. The message is formatted and logged only on the first call and then at most once per period. Keep one throttle per message. A message below the logger's level does not count, so it still appears once the level is raised.

```c++
#include <chrono>

#include "duatic_message_logger/log.hpp"

...
logging::LogThrottle no_reply_log{ std::chrono::seconds(5) };  // e.g. a member, one per message

...
logging::warning(no_reply_log, logging::LogThrottle::Clock::now(), logger, "No reply from {}", name);

// in a cyclic loop, pass the cycle's time stamp instead of reading the clock per message
logging::log(no_reply_log, cycle_start, logger, logging::LogLevel::Warning, "No reply from {}", name);
```

A `LogOnce` lets a message through only the first time.

```c++
logging::LogOnce no_zero_log;

...
logging::warning(no_zero_log, logger, "{} has no stored zero", name);
```

## 6. Old MELO style api

Do not use this - this is for compatibility only.
The `message_logger` packet is implemented as drop in replacement.

```c++
#include "message_logger/message_logger.hpp"

...

MELO_INFO("{}", 1);
MELO_INFO_STREAM("ABC" << 1);
```

## 7. Explicit namespace

Use this in case the `logging` namespace is too general for you

```c++
// we include a different header
#include "duatic_message_logger/logging.hpp"

...
// call with the full namespace
duatic::message_logger::debug("Debug {}", 1);

```

## 8. Loggers, levels and sinks

```
 get_logger("duatic.duadrive.1234") ─┐  level from the level settings, stored on the logger
 get_logger("duatic.ecat.eth0")     ─┼─► sinks ────────┬─► output:   ROS 2 logging when loaded, stdout otherwise
 get_default_logger()               ─┘                 │             (or anything set_output_sink() chose)
                                                       └─► attached: e.g. a GUI log view (attach_sink / detach_sink)
```

**Loggers decide, sinks are destinations.** A record is filtered once, by its logger's level, before it is
formatted. The output and every attached sink receive all records that passed. Keep sinks at their default level and
filter only for display, e.g. in a GUI log view.

**Levels** are set by name and apply to that name and every name below it (`a` covers `a.b` and `a.b.c`, not `ab`).
Where no name setting matches, the `Default` level applies (Info until set). The most specific setting wins, and
changes also apply to loggers created before the call.

```c++
logging::set_level(logging::Default, logging::LogLevel::Warning);       // everything not set by name
logging::set_level("duatic.ecat", logging::LogLevel::Debug);            // duatic.ecat and below
logging::reset_level("duatic.ecat");                                    // inherit again

logging::get_level("duatic.ecat");                 // set for exactly this name? (std::optional)
logging::get_effective_level("duatic.ecat.eth0");  // the level a logger of this name uses
```

`logging::Default` is the level for every name without a setting; it is not the level of the default logger
(`get_default_logger()`, named `global_logger`), which is set by its name like any other logger.

Typical use in a command line tool:

```c++
const auto level = logging::parse_level(argument);  // debug, info, warning (or warn), error, fatal
if (!level) {
  // report the valid names: logging::to_string(l) for each l in logging::log_levels
}
logging::set_level(logging::Default, *level);
```

**Sinks**: `set_output_sink()` replaces the output, also for loggers created before the call; attached sinks stay.
`attach_sink()` additionally forwards every record to another spdlog sink, and once `detach_sink()` returns, that sink
receives nothing more. Both take any spdlog sink; use the thread safe `_mt` variants, as several threads log.

```c++
#include "duatic_message_logger/log.hpp"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

...
  // Additionally write every record of the registered loggers to a file
  auto file = std::make_shared<spdlog::sinks::basic_file_sink_mt>("/tmp/session.log");
  file->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n]: %v");
  logging::attach_sink(file);
  ...
  logging::detach_sink(file);

  // Write to a colored console instead of ROS 2 logging / stdout
  logging::set_output_sink(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
```

A custom sink, e.g. a GUI log view, derives from `spdlog::sinks::base_sink`, which serializes the calls with its
mutex. Sinks run on the logging thread while the sinks are locked, so a sink must not log through a registered
logger (that deadlocks) and should hand records to another thread instead of doing slow work there; in Qt, connect
its signal with `Qt::QueuedConnection`, also for records logged on the GUI thread:

```c++
#include <mutex>

#include "duatic_message_logger/log.hpp"
#include "spdlog/sinks/base_sink.h"

class LogView : public spdlog::sinks::base_sink<std::mutex>
{
protected:
  void sink_it_(const spdlog::details::log_msg& msg) override
  {
    const std::string_view logger(msg.logger_name.data(), msg.logger_name.size());
    const std::string_view text(msg.payload.data(), msg.payload.size());
    // msg.level, msg.time, ... : queue them for the view
  }
  void flush_() override
  {
  }
};

...
  auto view = std::make_shared<LogView>();
  logging::attach_sink(view);
  ...
  logging::detach_sink(view);  // before the view is destroyed
```

## 9. ROS 2

The logger has no runtime ROS dependency. In a process that has ROS 2 loaded, the output writes through the ROS 2
logging output (the rcutils output handler), so messages appear in the console and in the process's ROS log file
under `~/.ros/log` in the ROS format, with no setup in the program. Elsewhere it writes to stdout. It is built against
the rcutils headers, but the rcutils functions are looked up at runtime, so the library does not link rcutils and
installs do not need it.

ROS only handles its own loggers: ROS levels and `--ros-args --log-level` do not affect duatic loggers, and our level
settings do not change ROS's; use `set_level` (e.g. from a node parameter). The records do not go to `/rosout`, which
rcl only publishes for loggers that belong to a node.

## 10. Python

The package also installs the Python module `duatic_message_logger`. Python and the C++ code of a process share one
logger registry: a logger of the same name is the same logger, and level settings made in Python also apply to the C++
loggers (e.g. the DuaDrive SDK), and the other way round.

```python
import duatic_message_logger as dml

log = dml.get_logger("duatic.my_tool")
log.info("connected to {}", bus)    # formatted with str.format, only if the level passes
log.warning(f"plain message {x}")  # a message without arguments is logged as it is
dml.info("through the default logger")

dml.set_level(dml.Default, dml.LogLevel.Debug)
dml.set_level("duatic.ecat", dml.LogLevel.Warning)
level = dml.parse_level("warn")    # None for an unknown name
```

**GUI applications** attach a `QueueSink` and drain it from a timer of the GUI thread. The sink is plain C++: the
logging threads never wait for Python, and it holds up to `capacity` records, dropping the oldest when full.

```python
sink = dml.QueueSink(capacity=10000)
dml.attach_sink(sink)

def on_timer():  # e.g. a QTimer, Tk after() or a frame callback
    for record in sink.drain():
        view.append(record.level, record.logger_name, record.message, record.timestamp_ns)
    # sink.dropped: records lost because the sink was full

dml.detach_sink(sink)  # when the view closes
```

**Python's logging module**: `bridge_python_logging()` adds a `LoggingHandler` to the root logger (or the logger
passed), which logs every record through the duatic logger of the same name; the root logger maps to the default
logger. It sets that logger to `DEBUG`, so the duatic levels decide.

```python
import logging

dml.bridge_python_logging()
logging.getLogger("duatic.my_tool").info("value %s", 1)  # logged by duatic.my_tool
```

The output is chosen at the first record: import `rclpy` before the first record so the records go to ROS 2 logging.

# Contributing

Please see the [Contributing guide](./CONTRIBUTING.md)
