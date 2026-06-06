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

    def to_dict(self) -> dict[str, Any]:
        data: dict[str, Any] = {"from": self.source.to_dict(), "to": self.target.to_dict()}
        if self.topic:
            data["topic"] = self.topic
        if self.qos:
            data["qos"] = self.qos
        if self.position:
            data["position"] = self.position
        return data


@dataclass
class Node:
    id: str
    type: str
    package: str = ""
    filter: str = ""
    component_class: str = ""
    input_type: str = ""
    output_type: str = ""
    optional_output_type: str = ""
    topic: str = ""
    parameters: dict[str, Any] = field(default_factory=dict)
    qos: dict[str, Any] = field(default_factory=dict)
    sync: dict[str, Any] = field(default_factory=dict)
    position: dict[str, float] = field(default_factory=lambda: {"x": 0.0, "y": 0.0})

    def to_dict(self) -> dict[str, Any]:
        data: dict[str, Any] = {
            "id": self.id,
            "type": self.type,
            "position": self.position,
        }
        for key in (
            "package",
            "filter",
            "component_class",
            "input_type",
            "output_type",
            "optional_output_type",
            "topic",
        ):
            value = getattr(self, key)
            if value:
                data[key] = value
        for key in ("parameters", "qos", "sync"):
            value = getattr(self, key)
            if value:
                data[key] = value
        return data


@dataclass
class Graph:
    version: int = 1
    editor: dict[str, Any] = field(default_factory=dict)
    nodes: list[Node] = field(default_factory=list)
    edges: list[Edge] = field(default_factory=list)

    def validate(self, type_by_node: dict[str, str] | None = None) -> None:
        ids: set[str] = set()
        for node in self.nodes:
            if not node.id:
                raise ValueError("node id must not be empty")
            if node.id in ids:
                raise ValueError(f"duplicate node id {node.id}")
            ids.add(node.id)
            if node.type not in {"input", "filter", "topic", "output"}:
                raise ValueError(f"unsupported node type {node.type}")
            if node.type == "filter" and not (node.component_class or (node.package and node.filter)):
                raise ValueError(f"filter node {node.id} has no component identity")
            if node.type in {"input", "topic", "output"} and not node.topic:
                raise ValueError(f"{node.type} node {node.id} has no topic")
            if node.type == "topic" and not (node.input_type or node.output_type):
                raise ValueError(f"topic node {node.id} has no type")

        for edge in self.edges:
            if edge.source.node not in ids:
                raise ValueError(f"edge source {edge.source.node} does not exist")
            if edge.target.node not in ids:
                raise ValueError(f"edge target {edge.target.node} does not exist")
            if edge.source.node == edge.target.node:
                raise ValueError(f"self edge on {edge.source.node} is invalid")
            if type_by_node:
                source_type = type_by_node.get(edge.source.node)
                target_type = type_by_node.get(edge.target.node)
                if source_type and target_type and source_type != target_type:
                    raise ValueError(
                        f"type mismatch: {edge.source.node} produces {source_type}, "
                        f"{edge.target.node} expects {target_type}"
                    )
        topic_names = [node.topic for node in self.nodes if node.type == "topic" and node.topic]
        if len(topic_names) != len(set(topic_names)):
            raise ValueError("topic node names must be unique")

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
        graph.nodes.append(
            Node(
                id=item["id"],
                type=item["type"],
                package=item.get("package", ""),
                filter=item.get("filter", ""),
                component_class=item.get("component_class", ""),
                input_type=item.get("input_type", ""),
                output_type=item.get("output_type", ""),
                optional_output_type=item.get("optional_output_type", ""),
                topic=item.get("topic", ""),
                parameters=item.get("parameters", {}) or {},
                qos=item.get("qos", {}) or {},
                sync=item.get("sync", {}) or {},
                position=item.get("position", {"x": 0.0, "y": 0.0}) or {"x": 0.0, "y": 0.0},
            )
        )
    for item in data.get("edges", []):
        graph.edges.append(
            Edge(
                PortRef(item["from"]["node"], item["from"].get("port", "")),
                PortRef(item["to"]["node"], item["to"].get("port", "")),
                item.get("topic", ""),
                item.get("qos", {}) or {},
                item.get("position", {}) or {},
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
