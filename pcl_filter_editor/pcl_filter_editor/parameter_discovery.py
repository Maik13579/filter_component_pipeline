# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from dataclasses import dataclass, field
import re

import rclpy
from rcl_interfaces.srv import DescribeParameters, GetParameters, ListParameters
from rclpy.parameter import parameter_value_to_python

from pcl_filter_editor.runtime import LivePipelineRuntime


@dataclass
class ParameterMetadata:
    defaults: dict[str, object] = field(default_factory=dict)
    descriptions: dict[str, str] = field(default_factory=dict)


class ComponentParameterDiscovery:
    def __init__(self, runtime: LivePipelineRuntime | None = None) -> None:
        self._owns_runtime = runtime is None
        self._runtime = runtime or LivePipelineRuntime()

    def parameters_for_component(self, package: str, component_class: str) -> ParameterMetadata:
        return self._load_parameters(package, component_class)

    def _load_parameters(self, package: str, component_class: str) -> ParameterMetadata:
        node_name = self._probe_node_name(component_class)
        self._runtime.load(
            node_name,
            {
                "package": package,
                "component_class": component_class,
                "parameters": {},
            },
            configure=False,
        )
        runtime_node = self._runtime.node
        full_node_name = self._runtime.loaded[node_name].full_node_name
        try:
            return self.parameters_for_loaded_node(runtime_node, full_node_name)
        finally:
            try:
                self._runtime.unload(node_name)
            finally:
                if self._owns_runtime:
                    self._runtime.stop()

    def parameters_for_loaded_node(self, runtime_node, full_node_name: str) -> ParameterMetadata:
        names = self._list_parameter_names(runtime_node, full_node_name)
        names = [name for name in names if self._is_discoverable_parameter(name)]
        if not names:
            return ParameterMetadata()
        filter_names = [name for name in names if self._is_filter_parameter(name)]
        defaults = self._get_parameter_defaults(runtime_node, full_node_name, filter_names) if filter_names else {}
        descriptions = self._describe_parameters(runtime_node, full_node_name, names)
        return ParameterMetadata(defaults=defaults, descriptions=descriptions)

    def _list_parameter_names(self, node, node_name: str) -> list[str]:
        client = node.create_client(ListParameters, self._service_name(node_name, "list_parameters"))
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
        client = node.create_client(GetParameters, self._service_name(node_name, "get_parameters"))
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
        client = node.create_client(DescribeParameters, self._service_name(node_name, "describe_parameters"))
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

    def _is_discoverable_parameter(self, name: str) -> bool:
        if name in {"start_type_description_service", "use_sim_time"}:
            return False
        return True

    def _is_filter_parameter(self, name: str) -> bool:
        return not (
            name.startswith("inputs.")
            or name.startswith("outputs.")
            or name.startswith("sync.")
        )

    def _service_name(self, node_name: str, service: str) -> str:
        return f"/{node_name.strip('/')}/{service}"

    def _probe_node_name(self, component_class: str) -> str:
        name = component_class.rsplit("::", 1)[-1]
        name = name.removesuffix("Component")
        name = re.sub(r"(.)([A-Z][a-z]+)", r"\1_\2", name)
        name = re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", name)
        chars = [char.lower() if char.isalnum() else "_" for char in name]
        readable = re.sub(r"_+", "_", "".join(chars)).strip("_") or "component"
        return f"parameter_probe_{readable}"
