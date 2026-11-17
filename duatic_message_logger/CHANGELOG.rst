^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package duatic_message_logger
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Forthcoming
-----------
* feat!: Logger is a handle onto a logger; get_logger(name) returns the registered logger of that name, and copies share it
* feat!: duatic_message_logger alone decides the levels of its loggers: set_level / reset_level / get_level /
  get_effective_level by name (hierarchical, as in ROS 2) or for logging::Default; ROS levels no longer filter them
* feat!: records go straight to the ROS 2 logging output, so records ROS used to filter by level now reach its
  console and log file too
* feat: Logger has member log calls, fmt and stream style (logger.info("..."), logger.warning() << ...)
* feat: attach_sink / detach_sink forward records to additional sinks; set_output_sink replaces only the output
* feat: log_levels lists all levels and to_string(LogLevel) gives the names parse_level accepts
* feat: parse_level accepts "warn"
* fix: set_level enables debug output; loggers no longer stay at info
* fix: once and throttled messages below the logger's level no longer use up their slot
* fix: stream style calls below the logger's level no longer format their arguments
* fix: a stream style record keeps its logger alive until it is written
* fix: without ROS 2, rcutils is looked up once at the first record instead of once per second on the logging path;
  a process that loads ROS 2 only after its first record keeps writing to stdout
* fix!: operator<<(LogLevel) writes the same lowercase name as to_string (debug instead of Debug)
* doc: sinks run on the logging thread while the sinks are locked; they must not log and should queue slow work
* feat!: rename configure_logger to set_default_logger, set_default_sink to set_output_sink and
  parse_level_from_string to parse_level
* feat!: remove get_logger_with_default_sink (use get_logger), configure_level (use set_level(Default, level)) and
  configure_logger_with_default_sink (use get_logger, or set_default_logger with a custom logger)

2.0.0 (2026-11-04)
------------------
* feat!: drop the rclcpp dependency, detect ROS 2 logging at runtime (#15)
* feat: add an ostream operator for LogLevel and a parse_level_from_string function
* feat: add configure_level to change the maximum log level of the default sink
* feat: add configure_logger_with_default_sink to configure a logger with the default sink
* fix: handle log levels on which the logger and the ROS 2 backend disagree (#10)
* fix: add the missing <optional> include (#8)
* fix: use a function local static instead of a global one, avoiding the static initialization
  order fiasco (#7)
* fix: depend on fmt directly, avoiding a version mismatch (#4)
* fix: build with -fPIC, as the library is loaded in plugins
* fix: log under each message's own logger name instead of a fixed ROS 2 logger
* refactor: clearer names in the logging API, with get_default_logger public
