# Copyright 2026 Duatic AG
# SPDX-License-Identifier: BSD-3-Clause
import logging
import threading

import pytest

import duatic_message_logger as dml

Debug = dml.LogLevel.Debug
Info = dml.LogLevel.Info
Warning = dml.LogLevel.Warning
Error = dml.LogLevel.Error


@pytest.fixture
def sink():
    queue = dml.QueueSink()
    dml.attach_sink(queue)
    yield queue
    dml.detach_sink(queue)


def records_of(queue, name):
    return [record for record in queue.drain() if record.logger_name == name]


def test_level_names_round_trip():
    for level in dml.log_levels:
        assert dml.parse_level(dml.to_string(level)) == level
    assert dml.parse_level("WARN") == Warning
    assert dml.parse_level("verbose") is None


def test_loggers_are_shared_and_follow_level_settings():
    first = dml.get_logger("py.shared")
    second = dml.get_logger("py.shared")
    assert first.name == "py.shared"
    assert not first.should_log(Debug)
    dml.set_level("py", Debug)
    try:
        assert second.should_log(Debug)
        assert dml.get_level("py") == Debug
        assert dml.get_level("py.shared") is None
        assert dml.get_effective_level("py.shared") == Debug
    finally:
        dml.reset_level("py")
    assert not first.should_log(Debug)


def test_default_level():
    dml.set_level(dml.Default, Warning)
    try:
        assert dml.get_effective_level(dml.Default) == Warning
        assert not dml.get_logger("py.default_level").should_log(Info)
    finally:
        dml.reset_level(dml.Default)
    assert dml.get_level(dml.Default) is None


def test_invalid_arguments_raise():
    with pytest.raises(ValueError):
        dml.get_logger("")
    with pytest.raises(ValueError):
        dml.QueueSink(capacity=0)


def test_records_reach_the_sink(sink):
    log = dml.get_logger("py.records")
    log.info("value {} {name}", 1, name="x")
    log.warning("braces {kept}")  # without arguments the message is not formatted
    log.debug("dropped")
    records = records_of(sink, "py.records")
    assert [(record.level, record.message) for record in records] == [
        (Info, "value 1 x"),
        (Warning, "braces {kept}"),
    ]
    assert records[0].timestamp_ns > 0


def test_format_errors_are_logged_instead_of_raised(sink):
    log = dml.get_logger("py.format_error")
    log.info("{} {}", 1)
    log.info("{:d}", "x")
    messages = [record.message for record in records_of(sink, "py.format_error")]
    assert messages[0].startswith("{} {} (format error: IndexError: ")
    assert "args: (1,)" in messages[0]
    assert messages[1].startswith("{:d} (format error: ValueError: ")


def test_unprintable_message_is_logged_instead_of_raised(sink):
    class Unprintable:
        def __str__(self):
            raise RuntimeError("no text")

    dml.get_logger("py.unprintable").info(Unprintable())
    messages = [record.message for record in records_of(sink, "py.unprintable")]
    assert messages == ["(unprintable message: RuntimeError: no text)"]


def test_keywords_may_be_named_like_the_parameters(sink):
    dml.get_logger("py.keywords").info("{message}", message="hi")
    dml.log(Info, "{level}", level=3)
    records = [(record.logger_name, record.message) for record in sink.drain()]
    assert ("py.keywords", "hi") in records
    assert (dml.get_default_logger().name, "3") in records


def test_surrogates_are_escaped(sink):
    dml.get_logger("py.surrogates").info("bad \udc80 name")
    assert [record.message for record in records_of(sink, "py.surrogates")] == ["bad \\udc80 name"]


def test_levels_are_ordered():
    assert Debug < Info < Warning < Error < dml.LogLevel.Fatal
    assert sorted(reversed(dml.log_levels)) == list(dml.log_levels)


def test_handles_of_one_name_are_equal():
    first = dml.get_logger("py.equal")
    second = dml.get_logger("py.equal")
    assert first == second
    assert hash(first) == hash(second)
    assert first != dml.get_logger("py.other")
    assert {first: "value"}[second] == "value"
    assert repr(dml.Default) == "Default"


def test_disabled_record_is_not_formatted():
    class Counted:
        calls = 0

        def __format__(self, spec):
            Counted.calls += 1
            return "counted"

    log = dml.get_logger("py.lazy")
    log.debug("{}", Counted())
    assert Counted.calls == 0
    log.info("{}", Counted())
    assert Counted.calls == 1


def test_free_functions_use_the_default_logger(sink):
    dml.info("default {}", 1)
    dml.log(Error, "default {}", 2)
    records = records_of(sink, dml.get_default_logger().name)
    assert [(record.level, record.message) for record in records] == [
        (Info, "default 1"),
        (Error, "default 2"),
    ]


def test_detached_sink_receives_nothing():
    queue = dml.QueueSink()
    dml.attach_sink(queue)
    dml.detach_sink(queue)
    dml.get_logger("py.detached").info("after detach")
    assert queue.drain() == []


def test_full_sink_drops_the_oldest_records():
    queue = dml.QueueSink(capacity=2)
    dml.attach_sink(queue)
    try:
        log = dml.get_logger("py.full")
        for message in ("a", "b", "c"):
            log.info(message)
    finally:
        dml.detach_sink(queue)
    assert [record.message for record in queue.drain()] == ["b", "c"]
    assert queue.dropped == 1


def test_threads_log_while_the_main_thread_drains(sink):
    log = dml.get_logger("py.threads")
    threads = [
        threading.Thread(
            target=lambda index=index: [
                log.info("thread {} record {}", index, i) for i in range(500)
            ]
        )
        for index in range(8)
    ]
    for thread in threads:
        thread.start()
    received = []
    while any(thread.is_alive() for thread in threads):
        received += records_of(sink, "py.threads")
    for thread in threads:
        thread.join(timeout=30)
        assert not thread.is_alive()
    received += records_of(sink, "py.threads")
    assert len(received) == 8 * 500


def test_bridge_forwards_python_logging(sink):
    root = logging.getLogger()
    root_level = root.level
    handler = dml.bridge_python_logging()
    try:
        assert dml.bridge_python_logging() is handler
        python_log = logging.getLogger("py.bridge")
        python_log.info("value %s", 1)
        python_log.debug("below the duatic level")
        try:
            raise RuntimeError("boom")
        except RuntimeError:
            python_log.exception("failed")
        records = records_of(sink, "py.bridge")
        assert len(records) == 2
        assert (records[0].level, records[0].message) == (Info, "value 1")
        assert records[1].level == Error
        assert records[1].message.startswith("failed")
        assert "RuntimeError: boom" in records[1].message
    finally:
        root.removeHandler(handler)
        root.setLevel(root_level)
