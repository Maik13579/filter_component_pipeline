# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from filter_component_synchronizer_py import FilterComponentSynchronizer
from filter_component_synchronizer_py import SynchronizerOptions


class Adapter:
    pass


def test_single_input_receipt_time_triggers_and_takes_message() -> None:
    ready = 0

    def on_ready() -> None:
        nonlocal ready
        ready += 1

    sync = FilterComponentSynchronizer(ready_callback=on_ready)
    sync.add_input("cloud", Adapter)

    message = object()
    sync.store_message("cloud", Adapter, message, 10)

    assert ready == 1
    assert sync.take_input("cloud", Adapter) is message
    assert sync.take_input("cloud", Adapter) is None


def test_latest_waits_for_all_inputs_and_keeps_latest_messages() -> None:
    ready = 0
    sync = FilterComponentSynchronizer(
        SynchronizerOptions(mode="latest"),
        lambda: globals(),
    )
    sync._ready_callback = lambda: None
    sync.add_input("a", Adapter)
    sync.add_input("b", Adapter)

    sync.store_message("a", Adapter, "a1", 10)
    assert sync.peek_input("a") == "a1"
    assert sync.peek_input("b") is None

    sync._ready_callback = lambda: None
    sync.store_message("b", Adapter, "b1", 20)
    assert sync.take_input("a") == "a1"
    assert sync.take_input("b") == "b1"


def test_receipt_time_rejects_sets_outside_max_interval() -> None:
    ready = 0

    def on_ready() -> None:
        nonlocal ready
        ready += 1

    sync = FilterComponentSynchronizer(
        SynchronizerOptions(mode="receipt_time", max_interval=0.01),
        on_ready,
    )
    sync.add_input("a", Adapter)
    sync.add_input("b", Adapter)

    sync.store_message("a", Adapter, "a1", 0)
    sync.store_message("b", Adapter, "b1", 20_000_000)
    assert ready == 0

    sync.store_message("a", Adapter, "a2", 19_000_000)
    assert ready == 1
    assert sync.take_input("a") == "a2"
    assert sync.take_input("b") == "b1"
