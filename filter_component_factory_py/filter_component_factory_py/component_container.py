# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from dataclasses import dataclass
import gc
from importlib import import_module
from typing import Any

from composition_interfaces.srv import ListNodes
from composition_interfaces.srv import LoadNode
from composition_interfaces.srv import UnloadNode
from rclpy.executors import MultiThreadedExecutor
from rclpy.lifecycle import TransitionCallbackReturn
from rclpy.node import Node
from rclpy.parameter import Parameter
from rclpy.parameter import parameter_value_to_python


@dataclass
class LoadedPythonNode:
    unique_id: int
    node_id: str
    full_node_name: str
    instance: Any


def instantiate_python_filter(
    python_module: str,
    python_class: str,
    node_id: str,
    namespace: str = "",
) -> Any:
    module = import_module(python_module)
    cls = getattr(module, python_class)
    kwargs: dict[str, Any] = {
        "node_name": node_id,
        "use_global_arguments": False,
    }
    if namespace:
        kwargs["namespace"] = namespace
    try:
        return cls(**kwargs)
    except TypeError:
        return cls(node_name=node_id)


def transition_lifecycle_node(node: Any, transition_name: str, node_id: str) -> None:
    result = getattr(node, transition_name)()
    if result != TransitionCallbackReturn.SUCCESS:
        raise RuntimeError(f"Failed to run {transition_name} for Python filter '{node_id}'")


class PythonComponentContainer(Node):
    def __init__(self, node_name: str = "filter_component_python_container") -> None:
        super().__init__(node_name)
        self._executor: MultiThreadedExecutor | None = None
        self._next_unique_id = 1
        self._loaded: dict[int, LoadedPythonNode] = {}
        prefix = self.get_fully_qualified_name()
        self.create_service(LoadNode, f"{prefix}/_container/load_node", self._load_node)
        self.create_service(UnloadNode, f"{prefix}/_container/unload_node", self._unload_node)
        self.create_service(ListNodes, f"{prefix}/_container/list_nodes", self._list_nodes)

    def attach_executor(self, executor: MultiThreadedExecutor) -> None:
        self._executor = executor

    def unload_all(self) -> None:
        for unique_id in list(self._loaded):
            self._destroy_loaded_node(unique_id)

    def _load_node(self, request: LoadNode.Request, response: LoadNode.Response) -> LoadNode.Response:
        if self._executor is None:
            response.success = False
            response.error_message = "Python component container has no executor"
            return response
        try:
            module_name, class_name = self._parse_plugin_name(request.plugin_name)
            node_name = request.node_name or class_name
            instance = instantiate_python_filter(
                module_name,
                class_name,
                node_name,
                request.node_namespace,
            )
            parameters = {
                item.name: parameter_value_to_python(item.value)
                for item in request.parameters
            }
            if parameters:
                instance.set_parameters([Parameter(name, value=value) for name, value in parameters.items()])
            unique_id = self._next_unique_id
            self._next_unique_id += 1
            self._executor.add_node(instance)
            full_node_name = instance.get_fully_qualified_name()
            self._loaded[unique_id] = LoadedPythonNode(
                unique_id=unique_id,
                node_id=node_name,
                full_node_name=full_node_name,
                instance=instance,
            )
            response.success = True
            response.full_node_name = full_node_name
            response.unique_id = unique_id
        except Exception as error:
            response.success = False
            response.error_message = str(error)
        return response

    def _unload_node(self, request: UnloadNode.Request, response: UnloadNode.Response) -> UnloadNode.Response:
        if request.unique_id not in self._loaded:
            response.success = False
            response.error_message = f"No node with unique_id {request.unique_id}"
            return response
        try:
            self._destroy_loaded_node(request.unique_id)
            response.success = True
        except Exception as error:
            response.success = False
            response.error_message = str(error)
        return response

    def _list_nodes(self, _request: ListNodes.Request, response: ListNodes.Response) -> ListNodes.Response:
        loaded = sorted(self._loaded.values(), key=lambda item: item.unique_id)
        response.full_node_names = [item.full_node_name for item in loaded]
        response.unique_ids = [item.unique_id for item in loaded]
        return response

    def _destroy_loaded_node(self, unique_id: int) -> None:
        loaded = self._loaded.pop(unique_id)
        if self._executor is not None:
            self._executor.remove_node(loaded.instance)
        loaded.instance.destroy_node()
        state_machine = getattr(loaded.instance, "_state_machine", None)
        if state_machine is not None:
            try:
                state_machine.destroy_when_not_in_use()
            except Exception:
                pass
            loaded.instance._state_machine = None
        gc.collect()

    @staticmethod
    def _parse_plugin_name(plugin_name: str) -> tuple[str, str]:
        for separator in (":", "::"):
            if separator in plugin_name:
                module_name, class_name = plugin_name.rsplit(separator, 1)
                if module_name and class_name:
                    return module_name, class_name
        raise ValueError("Python plugin_name must be '<module>:<class>'")
