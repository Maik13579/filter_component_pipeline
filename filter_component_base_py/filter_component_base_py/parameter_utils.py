# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from typing import Any

from rcl_interfaces.msg import FloatingPointRange
from rcl_interfaces.msg import IntegerRange
from rcl_interfaces.msg import ParameterDescriptor


def declare_parameter_if_not_declared(
    node: Any,
    name: str,
    default_value: Any,
    descriptor: ParameterDescriptor | None = None,
) -> None:
    if node.has_parameter(name):
        return
    if descriptor is None:
        node.declare_parameter(name, default_value)
    else:
        node.declare_parameter(name, default_value, descriptor)


def make_parameter_descriptor(
    description: str,
    additional_constraints: str = "",
) -> ParameterDescriptor:
    descriptor = ParameterDescriptor()
    descriptor.description = description
    descriptor.additional_constraints = additional_constraints
    return descriptor


def make_integer_range_parameter_descriptor(
    description: str,
    from_value: int,
    to_value: int,
    step: int = 1,
    additional_constraints: str = "",
) -> ParameterDescriptor:
    descriptor = make_parameter_descriptor(description, additional_constraints)
    integer_range = IntegerRange()
    integer_range.from_value = from_value
    integer_range.to_value = to_value
    integer_range.step = step
    descriptor.integer_range.append(integer_range)
    return descriptor


def make_floating_point_range_parameter_descriptor(
    description: str,
    from_value: float,
    to_value: float,
    step: float = 0.0,
    additional_constraints: str = "",
) -> ParameterDescriptor:
    descriptor = make_parameter_descriptor(description, additional_constraints)
    floating_point_range = FloatingPointRange()
    floating_point_range.from_value = from_value
    floating_point_range.to_value = to_value
    floating_point_range.step = step
    descriptor.floating_point_range.append(floating_point_range)
    return descriptor


def get_parameter(node: Any, name: str) -> Any:
    return node.get_parameter(name).value
