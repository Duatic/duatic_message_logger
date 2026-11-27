^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package message_logger
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Upcoming changes
----------------
* fix: MELO_FATAL, MELO_FATAL_STREAM and MELO_FATAL_THROTTLE_STREAM log at fatal instead of error

1.0.0 (2026-11-05)
------------------
* feat: drop-in replacement for the MELO message_logger API, logging through duatic_message_logger
* feat: rate limit the MELO_*_THROTTLE_STREAM macros, which logged every call before
