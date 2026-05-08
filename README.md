# duatic_message_logger

A modern c++20 logging solution which can either log to a ROS2 backend or be used standalone
[![Humble Build Main](https://github.com/Duatic/duatic_message_logger/actions/workflows/build-humble.yml/badge.svg?branch=main)](https://github.com/Duatic/duatic_message_logger/actions/workflows/build-humble.yml) [![Jazzy Build Main](https://github.com/Duatic/duatic_message_logger/actions/workflows/build-jazzy.yml/badge.svg?branch=main)](https://github.com/Duatic/duatic_message_logger/actions/workflows/build-jazzy.yml)  [![Kilted Build Main](https://github.com/Duatic/duatic_message_logger/actions/workflows/build-kilted.yml/badge.svg?branch=main)](https://github.com/Duatic/duatic_message_logger/actions/workflows/build-kilted.yml)  [![Rolling Build Main](https://github.com/Duatic/duatic_message_logger/actions/workflows/build-rolling.yml/badge.svg?branch=main)](https://github.com/Duatic/duatic_message_logger/actions/workflows/build-rolling.yml)


It provide a compat header with the old `MELO` style logging macros.

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

logging::debug() << "Test" << std::endl;
logging::info() << "Test" << std::endl;
logging::warning() << "Test" << std::endl;
logging::error() << "Test" << std::endl;
logging::fatal() << "Test" << std::endl;

```

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
```

## 3. custom logger streaming api

```c++
#include "duatic_message_logger/log.hpp"

...
  // Create a custom sink + logger (spdlog style)
  auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  sink->set_color_mode(spdlog::color_mode::always);
  sink->set_pattern("[%^%l%$] [%Y-%m-%d %H:%M:%S.%e] [%n]: %v");
  spdlog::logger logger{ "custom", sink };

  // and use it in the streaming style
  logging::debug(logger) << "Test debug"
                                        << "test" << std::endl;
  logging::info(logger) << "Test info";
  logging::warning(logger) << "Test warning";
  logging::error(logger) << "Test error";
  logging::fatal(logger) << "Test fatal";
```

## 4. custom logger ftm style api

See [fmt](https://github.com/fmtlib/fmt) for all formatting options

```c++
#include "duatic_message_logger/log.hpp"

...
  // Create a custom sink + logger (spdlog style)
  auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  sink->set_color_mode(spdlog::color_mode::always);
  sink->set_pattern("[%^%l%$] [%Y-%m-%d %H:%M:%S.%e] [%n]: %v");
  spdlog::logger logger{ "custom", sink };

  // and use it in the streaming style
  logging::debug(logger, "Debug {}", 1);
  logging::info(logger, "Info {}", 1);
  logging::warning(logger, "Warning {}", 1);
  logging::error(logger, "Error {}", 1);
  logging::fatal(logger, "Fatal {}", 1);
```

## 5. Old MELO style api

Do not use this - this is for compatibility only.
The `message_logger` packet is implemented as drop in replacement.

```c++
#include "message_logger/message_logger.hpp"

...

MELO_INFO("{}", 1);
MELO_INFO_STREAM("ABC" << 1);
```

## 6. Explicit namespace

Use this in case the `logging` namespace is too general for you

```c++
// we include a different header
#include "duatic_message_logger/logging.hpp"

...
// call with the full namespace
duatic::message_logger::debug("Debug {}", 1);

```

# Contributing

Please see the [Contributing guide](./CONTRIBUTING.md)
