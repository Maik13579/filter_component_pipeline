# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from dataclasses import dataclass, field
from typing import Any

import yaml


@dataclass
class PortRef:
    node: str
    port: str = ""

    def to_dict(self) -> dict[str, str]:
        data = {"node": self.node}
        if self.port:
            data["port"] = self.port
        return data


@dataclass
class Edge:
    source: PortRef
    target: PortRef
    topic: str = ""
    qos: dict[str, Any] = field(default_factory=dict)
    position: dict[str, float] = field(default_factory=dict)
    compatibility: str = ""

    def to_dict(self) -> dict[str, Any]:
        data: dict[str, Any] = {"from": self.source.to_dict(), "to": self.target.to_dict()}
        if self.topic:
            data["topic"] = self.topic
        if self.position:
            data["position"] = self.position
        if self.compatibility == "ros_message":
            data["compatibility"] = self.compatibility
        return data


@dataclass
class Node:
    id: str
    type: str
    name: str = ""
    package: str = ""
    filter: str = ""
    component_class: str = ""
    input_type: str = ""
    output_type: str = ""
    input_ports: str = ""
    output_ports: str = ""
    topic: str = ""
    parameters: dict[str, Any] = field(default_factory=dict)
    qos: dict[str, Any] = field(default_factory=dict)
    inputs: dict[str, Any] = field(default_factory=dict)
    outputs: dict[str, Any] = field(default_factory=dict)
    sync: dict[str, Any] = field(default_factory=dict)
    position: dict[str, float] = field(default_factory=lambda: {"x": 0.0, "y": 0.0})

    def to_dict(self) -> dict[str, Any]:
        data: dict[str, Any] = {
            "type": self.type,
            "position": self.position,
        }
        if self.type == "filter":
            data["name"] = self.name or self.id
        for key in (
            "name",
            "package",
            "filter",
            "component_class",
            "input_type",
            "output_type",
            "input_ports",
            "output_ports",
            "topic",
        ):
            value = getattr(self, key)
            if value:
                data[key] = value
        for key in ("parameters", "inputs", "outputs", "sync"):
            value = getattr(self, key)
            if value and (self.type == "filter" or key not in {"inputs", "outputs"}):
                data[key] = value
        return data


@dataclass
class Graph:
    version: int = 1
    editor: dict[str, Any] = field(default_factory=dict)
    nodes: list[Node] = field(default_factory=list)
    edges: list[Edge] = field(default_factory=list)

    def validate(
        self,
        type_by_node: dict[str, str] | None = None,
        message_type_by_logical: dict[str, str] | None = None,
    ) -> None:
        ids: set[str] = set()
        nodes_by_id: dict[str, Node] = {}
        for node in self.nodes:
            if not node.id:
                raise ValueError("node id must not be empty")
            if node.id in ids:
                raise ValueError(f"duplicate node id {node.id}")
            ids.add(node.id)
            nodes_by_id[node.id] = node
            if node.type not in {"filter", "topic"}:
                raise ValueError(f"unsupported node type {node.type}")
            if node.type == "filter" and not (node.component_class or (node.package and node.filter)):
                raise ValueError(f"filter node {node.id} has no component identity")
            if node.type == "topic" and not node.topic:
                raise ValueError(f"topic node {node.id} has no topic")
            if node.type == "topic" and not (node.input_type or node.output_type):
                raise ValueError(f"topic node {node.id} has no type")

        for edge in self.edges:
            if edge.compatibility and edge.compatibility != "ros_message":
                raise ValueError(f"unsupported edge compatibility {edge.compatibility}")
            if edge.source.node not in ids:
                raise ValueError(f"edge source {edge.source.node} does not exist")
            if edge.target.node not in ids:
                raise ValueError(f"edge target {edge.target.node} does not exist")
            if edge.source.node == edge.target.node:
                raise ValueError(f"self edge on {edge.source.node} is invalid")
            source_type = self._node_type(nodes_by_id[edge.source.node], outgoing=True, port=edge.source.port)
            target_type = self._node_type(nodes_by_id[edge.target.node], outgoing=False, port=edge.target.port)
            if type_by_node:
                source_type = type_by_node.get(edge.source.node, source_type)
                target_type = type_by_node.get(edge.target.node, target_type)
            if source_type and target_type and source_type != target_type:
                if edge.compatibility == "ros_message":
                    if message_type_by_logical is not None:
                        source_message = message_type_by_logical.get(source_type, "")
                        target_message = message_type_by_logical.get(target_type, "")
                        if not source_message or source_message != target_message:
                            raise ValueError(
                                f"type mismatch: {edge.source.node} produces {source_type}, "
                                f"{edge.target.node} expects {target_type}"
                            )
                    continue
                raise ValueError(
                    f"type mismatch: {edge.source.node} produces {source_type}, "
                    f"{edge.target.node} expects {target_type}"
                )
        used_inputs: set[tuple[str, str]] = set()
        used_outputs: set[tuple[str, str]] = set()
        for edge in self.edges:
            source_node = nodes_by_id[edge.source.node]
            target_node = nodes_by_id[edge.target.node]
            if source_node.type == "filter":
                key = (
                    source_node.id,
                    _canonical_port(source_node.output_ports or source_node.output_type, edge.source.port, True),
                )
                if key in used_outputs:
                    raise ValueError(f"filter output {source_node.id}:{key[1]} is already connected")
                used_outputs.add(key)
            if target_node.type == "filter":
                key = (
                    target_node.id,
                    _canonical_port(target_node.input_ports or target_node.input_type, edge.target.port, False),
                )
                if key in used_inputs:
                    raise ValueError(f"filter input {target_node.id}:{key[1]} is already connected")
                used_inputs.add(key)
        topic_names = [node.topic for node in self.nodes if node.type == "topic" and node.topic]
        if len(topic_names) != len(set(topic_names)):
            raise ValueError("topic names must be unique")

    @staticmethod
    def _node_type(node: Node, outgoing: bool, port: str = "") -> str:
        if node.type == "topic":
            return _type_for_port(node.output_type or node.input_type, port, outgoing)
        return _type_for_port(
            (node.output_ports or node.output_type) if outgoing else (node.input_ports or node.input_type),
            port,
            outgoing,
        )

    def to_dict(self) -> dict[str, Any]:
        return {
            "version": self.version,
            "editor": self.editor,
            "nodes": [node.to_dict() for node in self.nodes],
            "edges": [edge.to_dict() for edge in self.edges],
        }


def graph_from_dict(data: dict[str, Any]) -> Graph:
    graph = Graph(version=int(data.get("version", 1)))
    graph.editor = data.get("editor", {}) or {}
    for item in data.get("nodes", []):
        node_type = item["type"]
        node_id = item.get("id", item.get("name", item.get("topic", "")))
        parameters = item.get("parameters", {}) or {}
        sync = item.get("sync", {}) or {}
        if node_type == "filter":
            parameters = dict(parameters)
            sync = dict(sync)
            for key in ("policy", "queue_size", "slop"):
                if key in parameters:
                    sync.setdefault(key, parameters.pop(key))
        graph.nodes.append(
            Node(
                id=node_id,
                type=node_type,
                name=item.get("name", ""),
                package=item.get("package", ""),
                filter=item.get("filter", ""),
                component_class=item.get("component_class", ""),
                input_type=item.get("input_type", ""),
                output_type=item.get("output_type", ""),
                input_ports=item.get("input_ports", ""),
                output_ports=item.get("output_ports", ""),
                topic=item.get("topic", ""),
                parameters=parameters,
                qos={},
                inputs=item.get("inputs", {}) or {},
                outputs=item.get("outputs", {}) or {},
                sync=sync,
                position=item.get("position", {"x": 0.0, "y": 0.0}) or {"x": 0.0, "y": 0.0},
            )
        )
    for item in data.get("edges", []):
        graph.edges.append(
            Edge(
                PortRef(item["from"]["node"], item["from"].get("port", "")),
                PortRef(item["to"]["node"], item["to"].get("port", "")),
                item.get("topic", ""),
                {},
                item.get("position", {}) or {},
                item.get("compatibility", ""),
            )
        )
    graph.validate()
    return graph


def load_graph(path: str) -> Graph:
    with open(path, "r", encoding="utf-8") as stream:
        return graph_from_dict(yaml.safe_load(stream) or {})


def save_graph(graph: Graph, path: str) -> None:
    graph.validate()
    with open(path, "w", encoding="utf-8") as stream:
        yaml.safe_dump(graph.to_dict(), stream, sort_keys=False)


def _split_types(value: str) -> list[str]:
    return [item.strip() for item in value.replace(";", ",").split(",") if item.strip()]


def _split_ports(value: str) -> list[tuple[str, str]]:
    ports: list[tuple[str, str]] = []
    for item in _split_types(value):
        if ":" in item:
            name, stream_type = item.split(":", 1)
            ports.append((name.strip(), stream_type.strip()))
        else:
            ports.append(("", item))
    return [(name, stream_type) for name, stream_type in ports if stream_type]


def _port_name_for_type(stream_type: str, index: int, total: int, outgoing: bool) -> str:
    if total > 1 and not outgoing:
        return f"input_{index + 1}"
    if stream_type == "PointIndices":
        return "indices"
    if stream_type.startswith("Point"):
        return "cloud"
    return stream_type.replace("/", "_").replace(":", "_").lower() or "out"


def _type_for_port(value: str, port: str, outgoing: bool) -> str:
    ports = _split_ports(value)
    if not ports:
        return ""
    if not port or port in {"in", "out"}:
        return ports[0][1]
    for index, (port_name, stream_type) in enumerate(ports):
        inferred_port = _port_name_for_type(stream_type, index, len(ports), outgoing)
        if port == stream_type or port == port_name or port == inferred_port:
            return stream_type
    return ports[0][1] if outgoing else ""


def _canonical_port(value: str, port: str, outgoing: bool) -> str:
    ports = _split_ports(value)
    default_port = "out" if outgoing else "in"
    if not ports:
        return port or default_port
    if not port or port in {"in", "out"}:
        port_name, stream_type = ports[0]
        if port_name:
            return port_name
        return _port_name_for_type(stream_type, 0, len(ports), outgoing) if len(ports) > 1 else default_port
    valid_ports = {
        port_name or _port_name_for_type(stream_type, index, len(ports), outgoing)
        for index, (port_name, stream_type) in enumerate(ports)
    }
    return port if port in valid_ports else port
