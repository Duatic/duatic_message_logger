2.0.0 (2026-11-04)
------------------
* feat!: drop the rclcpp dependency, detect ROS 2 logging at runtime (#15)
* Handle prints to log levels where the logger loglevel and the ros2 backend loglevel do not agree
  (#10)
* added missing <optional> include (#8)
* Added ostream operator for LogLevel, and added parse_level_from_string function
* Use a function local static instead of a global static to avoid the construction order fiasco (#7)
* added 'configure_level' function to chnage the maximum log level of the default sink
* added fmt as dependency in order to fix version mismatched (#4)
* bump cmake minimum version from 3.8 to 3.22
* add several warning flags + -fPIC as this library is used in plugin contexts
* Fix issue with fixed ros2 logger name
* added function which allows to configure logger and use the default sink
* made some names of the logging api clearer
