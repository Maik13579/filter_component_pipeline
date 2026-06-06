# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from dataclasses import dataclass, field
import uuid

import rclpy
from rcl_interfaces.srv import DescribeParameters, GetParameters, ListParameters
from rclpy.parameter import parameter_value_to_python

from pcl_filter_editor.runtime import LivePipelineRuntime


@dataclass
class ParameterMetadata:
    defaults: dict[str, object] = field(default_factory=dict)
    descriptions: dict[str, str] = field(default_factory=dict)


class ComponentParameterDiscovery:
    def __init__(self) -> None:
        self._cache: dict[tuple[str, str], ParameterMetadata] = {}
        self._runtime = LivePipelineRuntime("pcl_filter_editor_parameter_probe_container")

    def parameters_for_component(self, package: str, component_class: str) -> ParameterMetadata:
        key = (package, component_class)
        if key in self._cache:
            return self._cache[key]
        metadata = self._load_parameters(package, component_class)
        self._cache[key] = metadata
        return metadata

    def _load_parameters(self, package: str, component_class: str) -> ParameterMetadata:
        node_name = f"pcl_filter_editor_probe_{uuid.uuid4().hex[:12]}"
        self._runtime.load(
            node_name,
            {
                "package": package,
                "component_class": component_class,
                "parameters": {},
            },
        )
        runtime_node = self._runtime.node
        full_node_name = self._runtime.loaded[node_name].full_node_name
        try:
            names = self._list_parameter_names(runtime_node, full_node_name)
            names = [name for name in names if self._is_filter_parameter(name)]
            if not names:
                return ParameterMetadata()
            defaults = self._get_parameter_defaults(runtime_node, full_node_name, names)
            descriptions = self._describe_parameters(runtime_node, full_node_name, names)
            return ParameterMetadata(defaults=defaults, descriptions=descriptions)
        finally:
            self._runtime.unload(node_name)

    def _list_parameter_names(self, node, node_name: str) -> list[str]:
        client = node.create_client(ListParameters, f"/{node_name}/list_parameters")
        if not client.wait_for_service(timeout_sec=8.0):
            raise RuntimeError(f"Parameter service for {node_name} did not become available")
        request = ListParameters.Request()
        request.depth = 10
        future = client.call_async(request)
        rclpy.spin_until_future_complete(node, future, timeout_sec=8.0)
        if future.result() is None:
            raise RuntimeError(f"Failed to list parameters for {node_name}")
        return list(future.result().result.names)

    def _get_parameter_defaults(self, node, node_name: str, names: list[str]) -> dict[str, object]:
        client = node.create_client(GetParameters, f"/{node_name}/get_parameters")
        if not client.wait_for_service(timeout_sec=2.0):
            raise RuntimeError(f"Get-parameters service for {node_name} did not become available")
        request = GetParameters.Request()
        request.names = names
        future = client.call_async(request)
        rclpy.spin_until_future_complete(node, future, timeout_sec=8.0)
        if future.result() is None:
            raise RuntimeError(f"Failed to get parameters for {node_name}")
        return {
            name: parameter_value_to_python(value)
            for name, value in zip(names, future.result().values, strict=False)
        }

    def _describe_parameters(self, node, node_name: str, names: list[str]) -> dict[str, str]:
        client = node.create_client(DescribeParameters, f"/{node_name}/describe_parameters")
        if not client.wait_for_service(timeout_sec=2.0):
            raise RuntimeError(f"Describe-parameters service for {node_name} did not become available")
        request = DescribeParameters.Request()
        request.names = names
        future = client.call_async(request)
        rclpy.spin_until_future_complete(node, future, timeout_sec=8.0)
        if future.result() is None:
            raise RuntimeError(f"Failed to describe parameters for {node_name}")
        return {
            descriptor.name: descriptor.description
            for descriptor in future.result().descriptors
        }

    def _is_filter_parameter(self, name: str) -> bool:
        if name in {"use_sim_time"}:
            return False
        return not (
            name.startswith("inputs.")
            or name.startswith("outputs.")
            or name.startswith("sync.")
        )
