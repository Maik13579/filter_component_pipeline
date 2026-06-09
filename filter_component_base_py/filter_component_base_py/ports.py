# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from dataclasses import dataclass
from typing import Any


@dataclass(frozen=True)
class PortDescriptor:
    name: str
    adapter_type: type
    description: str = ""
    default_reliability: str = "best_effort"
    default_history: str = "keep_last"
    default_depth: int = 5
    default_durability: str = "volatile"
    direction: str = ""


def input_port(name: str, adapter_type: type, description: str = "", **kwargs: Any) -> PortDescriptor:
    return PortDescriptor(name=name, adapter_type=adapter_type, description=description, direction="input", **kwargs)


def output_port(name: str, adapter_type: type, description: str = "", **kwargs: Any) -> PortDescriptor:
    return PortDescriptor(name=name, adapter_type=adapter_type, description=description, direction="output", **kwargs)
