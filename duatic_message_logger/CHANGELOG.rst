^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package duatic_message_logger
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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
