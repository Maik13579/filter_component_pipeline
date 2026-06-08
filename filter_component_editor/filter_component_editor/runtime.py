# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from dataclasses import dataclass
import os
import signal
import subprocess
import time

from composition_interfaces.srv import ListNodes
from composition_interfaces.srv import LoadNode, UnloadNode
from lifecycle_msgs.msg import State, Transition
from lifecycle_msgs.srv import ChangeState, GetState
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
    parameters: dict[str, object]
    configured: bool = False


class LivePipelineRuntime:
    def __init__(self, container_name: str = "filter_component_editor_container") -> None:
        self.container_name = container_name
        self._process: subprocess.Popen | None = None
        self._owns_process = False
        self._attached_existing_process = False
        self._node = None
        self.loaded: dict[str, LoadedComponent] = {}

    def start(self) -> None:
        if not rclpy.ok():
            rclpy.init(args=None)
        if self._node is None:
            self._node = rclpy.create_node(f"{self.container_name}_client")
        if self._process is None or self._process.poll() is not None:
            if self._service_available(f"/{self.container_name}/_container/load_node", LoadNode):
                self._process = None
                self._owns_process = False
                self._attached_existing_process = True
                if not self.loaded:
                    self._adopt_loaded_components()
                return
            self.loaded.clear()
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
                start_new_session=True,
            )
            self._owns_process = True
            self._attached_existing_process = False
        self._wait_for_service(f"/{self.container_name}/_container/load_node", LoadNode)

    @property
    def node(self):
        self.start()
        return self._node

    def stop(self) -> None:
        if self.loaded and self._node is not None and rclpy.ok():
            try:
                self.unload_all(ensure_started=False)
            except RuntimeError:
                self.loaded.clear()
        else:
            self.loaded.clear()
        if self._node is not None:
            self._node.destroy_node()
            self._node = None
        if self._process is not None and self._owns_process:
            self._terminate_process_group()
            self._process = None
            self._owns_process = False
        elif self._attached_existing_process:
            self._terminate_attached_container()
            self._attached_existing_process = False

    def _terminate_process_group(self) -> None:
        if self._process is None:
            return
        try:
            os.killpg(self._process.pid, signal.SIGTERM)
        except ProcessLookupError:
            return
        try:
            self._process.wait(timeout=2.0)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(self._process.pid, signal.SIGKILL)
            except ProcessLookupError:
                return
            self._process.wait(timeout=2.0)

    def _terminate_attached_container(self) -> None:
        pattern = f"component_container_mt.*__node:={self.container_name}"
        result = subprocess.run(
            ["pgrep", "-f", pattern],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
        )
        pids = [int(line) for line in result.stdout.splitlines() if line.strip().isdigit()]
        for pid in pids:
            try:
                os.kill(pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
        deadline = time.monotonic() + 2.0
        while time.monotonic() < deadline:
            remaining = [
                pid for pid in pids
                if self._process_exists(pid)
            ]
            if not remaining:
                return
            time.sleep(0.05)
        for pid in pids:
            try:
                os.kill(pid, signal.SIGKILL)
            except ProcessLookupError:
                pass

    def _process_exists(self, pid: int) -> bool:
        try:
            os.kill(pid, 0)
        except ProcessLookupError:
            return False
        return True

    def sync(self, desired: dict[str, dict[str, object]]) -> None:
        if not desired:
            self.start()
            self._reconcile_loaded_components()
            self.unload_all()
            return
        self.start()
        self._reconcile_loaded_components()
        for node_id in list(self.loaded):
            loaded = self.loaded[node_id]
            spec = desired.get(node_id)
            if spec is None:
                self.unload(node_id)
            elif loaded.package != spec["package"] or loaded.component_class != spec["component_class"]:
                self.unload(node_id)

        for node_id, spec in desired.items():
            configure = bool(spec.get("configure", True))
            if node_id not in self.loaded:
                self.load(node_id, spec, configure=configure)
            elif self._topic_parameters_changed(self.loaded[node_id].parameters, spec["parameters"]):
                self.unload(node_id)
                self.load(node_id, spec, configure=configure)
            elif spec.get("reload_on_parameter_change") and loaded.parameters != spec["parameters"]:
                self.unload(node_id)
                self.load(node_id, spec, configure=configure)
            elif configure:
                self.reconfigure(node_id, spec["parameters"])
            else:
                self.update_unconfigured(node_id, spec["parameters"])
        self._reconcile_loaded_components()

    def load(self, node_id: str, spec: dict[str, object], configure: bool = True) -> None:
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
            parameters=dict(spec["parameters"]),
            configured=False,
        )
        if configure:
            self._configure_and_activate(response.full_node_name)
            self.loaded[node_id].configured = True

    def unload(self, node_id: str, ensure_started: bool = True) -> None:
        loaded = self.loaded.pop(node_id, None)
        if loaded is None:
            return
        if ensure_started:
            self.start()
        if self._node is None:
            return
        try:
            self._make_unconfigured(loaded.full_node_name)
        except RuntimeError:
            pass
        client = self._client(f"/{self.container_name}/_container/unload_node", UnloadNode)
        request = UnloadNode.Request()
        request.unique_id = loaded.unique_id
        response = self._call(client, request, f"unload {node_id}")
        if not response.success:
            if self._is_missing_component_error(response.error_message):
                return
            raise RuntimeError(response.error_message or f"Failed to unload {node_id}")

    def unload_all(self, ensure_started: bool = True) -> None:
        for node_id in list(self.loaded):
            self.unload(node_id, ensure_started=ensure_started)

    def reconfigure(self, node_id: str, parameters: dict[str, object]) -> None:
        loaded = self.loaded[node_id]
        self._make_unconfigured(loaded.full_node_name)
        self._set_parameters(loaded.full_node_name, parameters)
        self._configure_and_activate(loaded.full_node_name)
        loaded.parameters = dict(parameters)
        loaded.configured = True

    def update_unconfigured(self, node_id: str, parameters: dict[str, object]) -> None:
        loaded = self.loaded[node_id]
        self._make_unconfigured(loaded.full_node_name)
        loaded.configured = False
        self._set_parameters(loaded.full_node_name, parameters)
        loaded.parameters = dict(parameters)

    def _topic_parameters_changed(self, current: dict[str, object], desired: dict[str, object]) -> bool:
        keys = {
            key for key in set(current).union(desired)
            if key.startswith(("inputs.", "outputs.")) and key.endswith(".topic")
        }
        return any(current.get(key) != desired.get(key) for key in keys)

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
            state = self._state_label(full_node_name)
            raise RuntimeError(f"Lifecycle transition {transition_id} failed for {full_node_name} from {state}")

    def _make_unconfigured(self, full_node_name: str) -> None:
        state_id = self._state_id(full_node_name)
        if state_id == State.PRIMARY_STATE_ACTIVE:
            self._transition(full_node_name, Transition.TRANSITION_DEACTIVATE)
            state_id = self._state_id(full_node_name)
        if state_id == State.PRIMARY_STATE_INACTIVE:
            self._transition(full_node_name, Transition.TRANSITION_CLEANUP)

    def _configure_and_activate(self, full_node_name: str) -> None:
        state_id = self._state_id(full_node_name)
        if state_id == State.PRIMARY_STATE_ACTIVE:
            return
        if state_id == State.PRIMARY_STATE_UNCONFIGURED:
            self._transition(full_node_name, Transition.TRANSITION_CONFIGURE)
            state_id = self._state_id(full_node_name)
        if state_id == State.PRIMARY_STATE_INACTIVE:
            self._transition(full_node_name, Transition.TRANSITION_ACTIVATE)
            return
        if state_id != State.PRIMARY_STATE_ACTIVE:
            raise RuntimeError(f"Cannot activate {full_node_name} from {self._state_label(full_node_name)}")

    def _state_id(self, full_node_name: str) -> int:
        client = self._client(f"{full_node_name}/get_state", GetState)
        response = self._call(client, GetState.Request(), f"get lifecycle state for {full_node_name}")
        return response.current_state.id

    def _state_label(self, full_node_name: str) -> str:
        try:
            client = self._client(f"{full_node_name}/get_state", GetState)
            response = self._call(client, GetState.Request(), f"get lifecycle state for {full_node_name}")
            return f"{response.current_state.label} [{response.current_state.id}]"
        except RuntimeError:
            return "unknown"

    def _parameter_messages(self, parameters: dict[str, object]) -> list[ParameterMsg]:
        return [Parameter(name, value=value).to_parameter_msg() for name, value in parameters.items()]

    def loaded_components(self) -> dict[str, int]:
        self.start()
        client = self._client(f"/{self.container_name}/_container/list_nodes", ListNodes)
        response = self._call(client, ListNodes.Request(), "list loaded nodes")
        return dict(zip(response.full_node_names, response.unique_ids, strict=False))

    def _adopt_loaded_components(self) -> None:
        client = self._node.create_client(ListNodes, f"/{self.container_name}/_container/list_nodes")
        if not client.wait_for_service(timeout_sec=0.5):
            return
        response = self._call(client, ListNodes.Request(), "list loaded nodes")
        self._merge_loaded_component_list(response.full_node_names, response.unique_ids)

    def _reconcile_loaded_components(self) -> None:
        if self._node is None:
            return
        client = self._node.create_client(ListNodes, f"/{self.container_name}/_container/list_nodes")
        if not client.wait_for_service(timeout_sec=0.5):
            return
        response = self._call(client, ListNodes.Request(), "list loaded nodes")
        actual_ids = {
            full_node_name.strip("/").rsplit("/", 1)[-1]
            for full_node_name in response.full_node_names
        }
        for node_id in list(self.loaded):
            if node_id not in actual_ids:
                self.loaded.pop(node_id, None)
        self._merge_loaded_component_list(response.full_node_names, response.unique_ids)

    def _merge_loaded_component_list(self, full_node_names: list[str], unique_ids: list[int]) -> None:
        for full_node_name, unique_id in zip(full_node_names, unique_ids, strict=False):
            node_id = full_node_name.strip("/").rsplit("/", 1)[-1]
            loaded = self.loaded.get(node_id)
            if loaded is not None:
                loaded.unique_id = unique_id
                loaded.full_node_name = full_node_name
                continue
            self.loaded[node_id] = LoadedComponent(
                unique_id=unique_id,
                full_node_name=full_node_name,
                package="",
                component_class="",
                parameters={},
                configured=False,
            )

    def _is_missing_component_error(self, message: str) -> bool:
        lowered = message.lower()
        return "no node" in lowered and "unique_id" in lowered

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

    def _service_available(self, service_name: str, service_type) -> bool:
        client = self._node.create_client(service_type, service_name)
        return client.wait_for_service(timeout_sec=0.1)

    def _call(self, client, request, description: str):
        future = client.call_async(request)
        rclpy.spin_until_future_complete(self._node, future, timeout_sec=8.0)
        if future.result() is None:
            raise RuntimeError(f"Timed out during {description}")
        return future.result()
