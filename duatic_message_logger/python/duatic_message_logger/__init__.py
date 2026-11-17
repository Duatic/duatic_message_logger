# Copyright 2026 Duatic AG
# SPDX-License-Identifier: BSD-3-Clause
"""
Python bindings of duatic_message_logger.

Python and the C++ code of a process share one logger registry: loggers of the same name are the
same logger, and level settings made here also apply to the C++ loggers (e.g. the DuaDrive SDK),
and the other way round.

    import duatic_message_logger as dml

    log = dml.get_logger("duatic.my_tool")
    dml.set_level(dml.Default, dml.LogLevel.Debug)
    log.debug("connected to {}", bus)  # formatted with str.format, only if the level passes

    sink = dml.QueueSink()  # for a GUI: drain it from a timer of the GUI thread
    dml.attach_sink(sink)
    for record in sink.drain():
        print(record.logger_name, record.message)

    dml.bridge_python_logging()  # records of Python's logging module go to the same loggers

Records go to ROS 2 logging when the process has it loaded, to stdout otherwise. The output is
chosen at the first record, so import rclpy before logging the first record.
"""

from ._bindings import (
    Default,
    DefaultLevelTag,
    LogLevel,
    LogRecord,
    Logger,
    QueueSink,
    attach_sink,
    debug,
    detach_sink,
    error,
    fatal,
    get_default_logger,
    get_effective_level,
    get_level,
    get_logger,
    info,
    log,
    log_levels,
    parse_level,
    reset_level,
    set_default_logger,
    set_level,
    to_string,
    warning,
)
from ._bridge import LoggingHandler, bridge_python_logging

__all__ = [
    "Default",
    "DefaultLevelTag",
    "LogLevel",
    "LogRecord",
    "Logger",
    "LoggingHandler",
    "QueueSink",
    "attach_sink",
    "bridge_python_logging",
    "debug",
    "detach_sink",
    "error",
    "fatal",
    "get_default_logger",
    "get_effective_level",
    "get_level",
    "get_logger",
    "info",
    "log",
    "log_levels",
    "parse_level",
    "reset_level",
    "set_default_logger",
    "set_level",
    "to_string",
    "warning",
]
