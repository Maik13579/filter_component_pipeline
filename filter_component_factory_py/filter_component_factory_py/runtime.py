# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import argparse
from typing import Any

import rclpy
from rclpy.executors import MultiThreadedExecutor
from rclpy.node import Node
from rclpy.parameter import Parameter

from filter_component_factory_py.component_container import PythonComponentContainer
from filter_component_factory_py.component_container import instantiate_python_filter
from filter_component_factory_py.component_container import transition_lifecycle_node
from filter_component_factory_py.graph_loader import PipelineNode
from filter_component_factory_py.graph_loader import load_pipeline_graph


class PythonPipelineRuntime(Node):
    def __init__(self) -> None:
        super().__init__("filter_pipeline_factory_py")
        self.declare_parameter("pipeline_file", "")
        self.loaded_nodes: list[Any] = []

    def load_pipeline(self, executor: MultiThreadedExecutor) -> None:
        pipeline_file = self.get_parameter("pipeline_file").value
        if not pipeline_file:
            raise RuntimeError("pipeline_file parameter is empty")
        graph = load_pipeline_graph(pipeline_file)
        for node in graph.python_nodes:
            instance = self._instantiate_node(node)
            self._apply_parameters(instance, self._parameters_for_node(graph, node))
            executor.add_node(instance)
            transition_lifecycle_node(instance, "trigger_configure", node.id)
            transition_lifecycle_node(instance, "trigger_activate", node.id)
            self.loaded_nodes.append(instance)

    def unload_pipeline(self, executor: MultiThreadedExecutor) -> None:
        for node in reversed(self.loaded_nodes):
            for transition in ("trigger_deactivate", "trigger_cleanup"):
                try:
                    transition_lifecycle_node(node, transition, node.get_name())
                except Exception:
                    pass
            executor.remove_node(node)
            node.destroy_node()
        self.loaded_nodes.clear()

    def _instantiate_node(self, node: PipelineNode) -> Any:
        return instantiate_python_filter(node.python_module, node.python_class, node.id)

    def _apply_parameters(self, node: Any, parameters: dict[str, Any]) -> None:
        if not parameters:
            return
        node.set_parameters([Parameter(name, value=value) for name, value in parameters.items()])

    def _parameters_for_node(self, graph, node: PipelineNode) -> dict[str, Any]:
        parameters = dict(node.parameters)
        for port, topic in _input_topics_for_node(graph, node.id):
            parameters[f"inputs.{port}.topic"] = topic
        for port, topic in _output_topics_for_node(graph, node.id):
            parameters[f"outputs.{port}.topic"] = topic
        for direction, ports in (("inputs", node.inputs), ("outputs", node.outputs)):
            for port, config in ports.items():
                for name, value in (config.get("qos", {}) or {}).items():
                    parameters[f"{direction}.{port}.qos.{name}"] = value
        for name, value in node.sync.items():
            parameters[f"sync.{name}"] = value
        return parameters

def _find_node(graph, node_id: str) -> PipelineNode | None:
    return next((node for node in graph.nodes if node.id == node_id), None)


def _input_topics_for_node(graph, node_id: str) -> list[tuple[str, str]]:
    topics: list[tuple[str, str]] = []
    for edge in graph.edges:
        target = edge.get("to", {})
        if target.get("node") != node_id:
            continue
        source = _find_node(graph, edge.get("from", {}).get("node", ""))
        if source is None:
            continue
        topic = source.topic if source.type == "topic" else edge.get("topic", "")
        if topic:
            topics.append((target.get("port", ""), topic))
    return topics


def _output_topics_for_node(graph, node_id: str) -> list[tuple[str, str]]:
    topics: list[tuple[str, str]] = []
    for edge in graph.edges:
        source_ref = edge.get("from", {})
        if source_ref.get("node") != node_id:
            continue
        target = _find_node(graph, edge.get("to", {}).get("node", ""))
        if target is None:
            continue
        topic = target.topic if target.type == "topic" else edge.get("topic", "")
        if topic:
            topics.append((source_ref.get("port", ""), topic))
    return topics


def main(args: list[str] | None = None) -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--container-name", default="filter_component_python_container")
    parsed, ros_args = parser.parse_known_args(args)
    rclpy.init(args=ros_args)
    executor = MultiThreadedExecutor()
    runtime = PythonComponentContainer(parsed.container_name)
    runtime.attach_executor(executor)
    executor.add_node(runtime)
    try:
        executor.spin()
    finally:
        runtime.unload_all()
        executor.remove_node(runtime)
        runtime.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
