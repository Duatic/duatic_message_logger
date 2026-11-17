# Copyright 2026 Duatic AG
# SPDX-License-Identifier: BSD-3-Clause
"""Sends the records of Python's logging module to the duatic loggers of the same name."""

import logging
from typing import Optional

from ._bindings import LogLevel, Logger, get_default_logger, get_logger


def _to_level(levelno: int) -> LogLevel:
    if levelno >= logging.CRITICAL:
        return LogLevel.Fatal
    if levelno >= logging.ERROR:
        return LogLevel.Error
    if levelno >= logging.WARNING:
        return LogLevel.Warning
    if levelno >= logging.INFO:
        return LogLevel.Info
    return LogLevel.Debug


def _to_logger(name: str) -> Logger:
    # The root logger of Python's logging module is the default logger
    return get_default_logger() if name == "root" else get_logger(name)


class LoggingHandler(logging.Handler):
    """
    Logs each record of Python's logging module through the duatic logger of the same name.

    The duatic logger's level decides as well: a record below it is dropped before it is formatted.
    The message is the handler's formatted record, by default the message with any traceback.
    """

    def emit(self, record: logging.LogRecord) -> None:
        try:
            logger = _to_logger(record.name)
            level = _to_level(record.levelno)
            if logger.should_log(level):
                logger.log(level, self.format(record))
        except Exception:  # noqa: BLE001 - reported by the logging module, as for every handler
            self.handleError(record)


def bridge_python_logging(logger: Optional[logging.Logger] = None) -> LoggingHandler:
    """
    Adds a LoggingHandler to a logger of Python's logging module, the root logger by default.

    The logger is set to DEBUG, so the duatic levels decide what is logged. Calling it again for the
    same logger returns the handler already added.
    """
    target = logger if logger is not None else logging.getLogger()
    target.setLevel(logging.DEBUG)
    for handler in target.handlers:
        if isinstance(handler, LoggingHandler):
            return handler
    handler = LoggingHandler()
    target.addHandler(handler)
    return handler
