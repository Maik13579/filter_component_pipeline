# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from dataclasses import dataclass
import subprocess
import time

from composition_interfaces.srv import LoadNode, UnloadNode
from lifecycle_msgs.msg import Transition
from lifecycle_msgs.srv import ChangeState
import rclpy
from rcl_interfaces.msg import Parameter as ParameterMsg
from rcl_interfaces.srv import SetParameters
from rclpy.parameter import Parameter


@dataclass
class LoadedComponent:
    unique_id: int
    full_node_name: str
    package: str
    component_class: str


class LivePipelineRuntime:
    def __init__(self, container_name: str = "pcl_filter_editor_container") -> None:
        self.container_name = container_name
        self._process: subprocess.Popen | None = None
        self._node = None
        self.loaded: dict[str, LoadedComponent] = {}

    def start(self) -> None:
        if not rclpy.ok():
            rclpy.init(args=None)
        if self._node is None:
            self._node = rclpy.create_node("pcl_filter_editor_runtime_client")
        if self._process is None or self._process.poll() is not None:
            self._process = subprocess.Popen(
                [
                    "ros2",
                    "run",
                    "rclcpp_components",
                    "component_container_mt",
                    "--ros-args",
                    "-r",
                    f"__node:={self.container_name}",
                ],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
        self._wait_for_service(f"/{self.container_name}/_container/load_node", LoadNode)

    @property
    def node(self):
        self.start()
        return self._node

    def stop(self) -> None:
        self.unload_all()
        if self._node is not None:
            self._node.destroy_node()
            self._node = None
        if self._process is not None:
            self._process.terminate()
            try:
                self._process.wait(timeout=2.0)
            except subprocess.TimeoutExpired:
                self._process.kill()
                self._process.wait(timeout=2.0)
            self._process = None

    def sync(self, desired: dict[str, dict[str, object]]) -> None:
        if not desired:
            self.unload_all()
            return
        self.start()
        for node_id in list(self.loaded):
            loaded = self.loaded[node_id]
            spec = desired.get(node_id)
            if spec is None:
                self.unload(node_id)
            elif loaded.package != spec["package"] or loaded.component_class != spec["component_class"]:
                self.unload(node_id)

        for node_id, spec in desired.items():
            if node_id not in self.loaded:
                self.load(node_id, spec)
            else:
                self.reconfigure(node_id, spec["parameters"])

    def load(self, node_id: str, spec: dict[str, object]) -> None:
        self.start()
        client = self._client(f"/{self.container_name}/_container/load_node", LoadNode)
        request = LoadNode.Request()
        request.package_name = str(spec["package"])
        request.plugin_name = str(spec["component_class"])
        request.node_name = node_id
        request.parameters = self._parameter_messages(spec["parameters"])
        request.extra_arguments = [Parameter("use_intra_process_comms", value=True).to_parameter_msg()]
        response = self._call(client, request, f"load {node_id}")
        if not response.success:
            raise RuntimeError(response.error_message or f"Failed to load {node_id}")
        self.loaded[node_id] = LoadedComponent(
            unique_id=response.unique_id,
            full_node_name=response.full_node_name,
            package=str(spec["package"]),
            component_class=str(spec["component_class"]),
        )
        self._transition(response.full_node_name, Transition.TRANSITION_CONFIGURE)
        self._transition(response.full_node_name, Transition.TRANSITION_ACTIVATE)

    def unload(self, node_id: str) -> None:
        loaded = self.loaded.pop(node_id, None)
        if loaded is None:
            return
        self.start()
        try:
            self._transition(loaded.full_node_name, Transition.TRANSITION_DEACTIVATE)
            self._transition(loaded.full_node_name, Transition.TRANSITION_CLEANUP)
        except RuntimeError:
            pass
        client = self._client(f"/{self.container_name}/_container/unload_node", UnloadNode)
        request = UnloadNode.Request()
        request.unique_id = loaded.unique_id
        response = self._call(client, request, f"unload {node_id}")
        if not response.success:
            raise RuntimeError(response.error_message or f"Failed to unload {node_id}")

    def unload_all(self) -> None:
        for node_id in list(self.loaded):
            self.unload(node_id)

    def reconfigure(self, node_id: str, parameters: dict[str, object]) -> None:
        loaded = self.loaded[node_id]
        self._transition(loaded.full_node_name, Transition.TRANSITION_DEACTIVATE)
        self._transition(loaded.full_node_name, Transition.TRANSITION_CLEANUP)
        self._set_parameters(loaded.full_node_name, parameters)
        self._transition(loaded.full_node_name, Transition.TRANSITION_CONFIGURE)
        self._transition(loaded.full_node_name, Transition.TRANSITION_ACTIVATE)

    def _set_parameters(self, full_node_name: str, parameters: dict[str, object]) -> None:
        client = self._client(f"{full_node_name}/set_parameters", SetParameters)
        request = SetParameters.Request()
        request.parameters = self._parameter_messages(parameters)
        response = self._call(client, request, f"set parameters on {full_node_name}")
        failed = [result.reason for result in response.results if not result.successful]
        if failed:
            raise RuntimeError("; ".join(failed))

    def _transition(self, full_node_name: str, transition_id: int) -> None:
        client = self._client(f"{full_node_name}/change_state", ChangeState)
        request = ChangeState.Request()
        request.transition.id = transition_id
        response = self._call(client, request, f"transition {full_node_name}")
        if not response.success:
            raise RuntimeError(f"Lifecycle transition {transition_id} failed for {full_node_name}")

    def _parameter_messages(self, parameters: dict[str, object]) -> list[ParameterMsg]:
        return [Parameter(name, value=value).to_parameter_msg() for name, value in parameters.items()]

    def _client(self, service_name: str, service_type):
        client = self._node.create_client(service_type, service_name)
        if not client.wait_for_service(timeout_sec=5.0):
            raise RuntimeError(f"Service {service_name} did not become available")
        return client

    def _wait_for_service(self, service_name: str, service_type) -> None:
        deadline = time.monotonic() + 8.0
        client = self._node.create_client(service_type, service_name)
        while time.monotonic() < deadline:
            if client.wait_for_service(timeout_sec=0.1):
                return
        raise RuntimeError(f"Service {service_name} did not become available")

    def _call(self, client, request, description: str):
        future = client.call_async(request)
        rclpy.spin_until_future_complete(self._node, future, timeout_sec=8.0)
        if future.result() is None:
            raise RuntimeError(f"Timed out during {description}")
        return future.result()
