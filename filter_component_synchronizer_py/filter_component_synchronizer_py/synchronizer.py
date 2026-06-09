# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from collections import deque
from dataclasses import dataclass
from typing import Any, Callable


def sync_mode_from_string(value: str) -> str:
    if value in {"receipt_time", "latest"}:
        return value
    raise ValueError(f"Unsupported sync mode '{value}'")


@dataclass
class SynchronizerOptions:
    mode: str = "receipt_time"
    queue_size: int = 10
    max_interval: float = 0.05

    def __post_init__(self) -> None:
        self.mode = sync_mode_from_string(self.mode)
        self.queue_size = max(1, int(self.queue_size))
        self.max_interval = max(0.0, float(self.max_interval))


class _InputState:
    def __init__(self, adapter_type: type) -> None:
        self.adapter_type = adapter_type
        self.queued: deque[tuple[int, Any]] = deque()
        self.latest: Any | None = None


class FilterComponentSynchronizer:
    def __init__(
        self,
        options: SynchronizerOptions | None = None,
        ready_callback: Callable[[], None] | None = None,
    ) -> None:
        self.options = options or SynchronizerOptions()
        self._ready_callback = ready_callback
        self._input_order: list[str] = []
        self._inputs: dict[str, _InputState] = {}

    def add_input(self, port_name: str, adapter_type: type) -> None:
        if port_name in self._inputs:
            raise ValueError(f"Duplicate input port '{port_name}'")
        self._input_order.append(port_name)
        self._inputs[port_name] = _InputState(adapter_type)

    def clear(self) -> None:
        self._input_order.clear()
        self._inputs.clear()

    def store_message(
        self,
        port_name: str,
        adapter_type: type,
        message: Any,
        receipt_stamp_ns: int,
    ) -> None:
        state = self._typed_state(port_name, adapter_type)
        if self.options.mode == "latest":
            state.latest = message
        else:
            state.queued.append((receipt_stamp_ns, message))
            while len(state.queued) > self.options.queue_size:
                state.queued.popleft()
        if self._try_synchronize() and self._ready_callback is not None:
            self._ready_callback()

    def take_input(self, port_name: str, adapter_type: type | None = None) -> Any | None:
        state = self._state(port_name)
        if adapter_type is not None and state.adapter_type is not adapter_type:
            raise TypeError(f"Input port '{port_name}' has a different adapter type")
        message = state.latest
        if self.options.mode == "receipt_time":
            state.latest = None
        return message

    def peek_input(self, port_name: str, adapter_type: type | None = None) -> Any | None:
        state = self._state(port_name)
        if adapter_type is not None and state.adapter_type is not adapter_type:
            raise TypeError(f"Input port '{port_name}' has a different adapter type")
        return state.latest

    def _state(self, port_name: str) -> _InputState:
        try:
            return self._inputs[port_name]
        except KeyError as exc:
            raise KeyError(f"Unknown input port '{port_name}'") from exc

    def _typed_state(self, port_name: str, adapter_type: type) -> _InputState:
        state = self._state(port_name)
        if state.adapter_type is not adapter_type:
            raise TypeError(f"Input port '{port_name}' has a different adapter type")
        return state

    def _try_synchronize(self) -> bool:
        if not self._input_order:
            return False
        if self.options.mode == "latest":
            return all(self._inputs[name].latest is not None for name in self._input_order)
        if not all(self._inputs[name].queued for name in self._input_order):
            return False
        selected = self._select_receipt_time()
        if selected is None:
            return False
        for port_name, index in selected.items():
            state = self._inputs[port_name]
            state.latest = state.queued[index][1]
            for _ in range(index + 1):
                state.queued.popleft()
        return True

    def _select_receipt_time(self) -> dict[str, int] | None:
        if len(self._input_order) == 1:
            return {self._input_order[0]: 0}
        max_interval_ns = round(self.options.max_interval * 1_000_000_000)
        best_span: int | None = None
        best_selected: dict[str, int] | None = None
        for port_name in self._input_order:
            state = self._inputs[port_name]
            for index, (minimum, _) in enumerate(state.queued):
                selected = {port_name: index}
                maximum = minimum
                complete = True
                for other_name in self._input_order:
                    if other_name == port_name:
                        continue
                    other_state = self._inputs[other_name]
                    other_index = next(
                        (
                            candidate
                            for candidate, (stamp, _) in enumerate(other_state.queued)
                            if stamp >= minimum
                        ),
                        None,
                    )
                    if other_index is None:
                        complete = False
                        break
                    selected[other_name] = other_index
                    maximum = max(maximum, other_state.queued[other_index][0])
                if not complete:
                    continue
                span = maximum - minimum
                if span <= max_interval_ns and (best_span is None or span < best_span):
                    best_span = span
                    best_selected = selected
        if best_selected is not None:
            return best_selected
        self._drop_oldest_receipt_time_message()
        return None

    def _drop_oldest_receipt_time_message(self) -> None:
        oldest_port = min(
            self._input_order,
            key=lambda name: self._inputs[name].queued[0][0],
        )
        self._inputs[oldest_port].queued.popleft()
