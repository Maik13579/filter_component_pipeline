# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from filter_component_base_py import declare_parameter_if_not_declared
from filter_component_base_py import make_floating_point_range_parameter_descriptor
from filter_component_base_py import make_integer_range_parameter_descriptor
from filter_component_base_py import make_parameter_descriptor


class FakeNode:
    def __init__(self) -> None:
        self.parameters = {}
        self.declarations = []

    def has_parameter(self, name: str) -> bool:
        return name in self.parameters

    def declare_parameter(self, name: str, default_value, descriptor=None) -> None:
        self.parameters[name] = default_value
        self.declarations.append((name, default_value, descriptor))


def test_declare_parameter_if_not_declared_skips_existing_parameter() -> None:
    node = FakeNode()
    node.parameters["filter.enabled"] = False

    declare_parameter_if_not_declared(node, "filter.enabled", True)

    assert node.parameters["filter.enabled"] is False
    assert node.declarations == []


def test_declare_parameter_if_not_declared_uses_descriptor() -> None:
    node = FakeNode()
    descriptor = make_parameter_descriptor("Enable filter.")

    declare_parameter_if_not_declared(node, "filter.enabled", True, descriptor)

    assert node.parameters["filter.enabled"] is True
    assert node.declarations == [("filter.enabled", True, descriptor)]


def test_range_parameter_descriptors_match_ros_descriptor_shape() -> None:
    integer_descriptor = make_integer_range_parameter_descriptor("Queue size.", 1, 100)
    floating_descriptor = make_floating_point_range_parameter_descriptor("Leaf size.", 1.0e-6, 1000.0)

    assert integer_descriptor.integer_range[0].from_value == 1
    assert integer_descriptor.integer_range[0].to_value == 100
    assert floating_descriptor.floating_point_range[0].from_value == 1.0e-6
    assert floating_descriptor.floating_point_range[0].to_value == 1000.0
