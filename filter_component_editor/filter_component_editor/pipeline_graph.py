# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from dataclasses import dataclass, field
from typing import Any

import yaml


@dataclass
class PortRef:
    node: str
    port: str = ""
    direction: str = ""

    def to_dict(self) -> dict[str, str]:
        data = {"node": self.node, "direction": self.direction}
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
    implementation: str = "cpp"
    package: str = ""
    filter: str = ""
    component_class: str = ""
    python_module: str = ""
    python_class: str = ""
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
            "python_module",
            "python_class",
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
    version: int = 2
    editor: dict[str, Any] = field(default_factory=dict)
    nodes: list[Node] = field(default_factory=list)
    edges: list[Edge] = field(default_factory=list)

    def validate(
        self,
        type_by_node: dict[str, str] | None = None,
        message_type_by_logical: dict[str, str] | None = None,
    ) -> None:
        if self.version != 2:
            raise ValueError(f"unsupported graph version {self.version}")
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
            if (
                node.type == "filter"
                and node.implementation == "cpp"
                and (node.python_module or node.python_class)
            ):
                node.implementation = "python"
            if node.type == "filter" and node.implementation == "cpp" and not node.component_class:
                raise ValueError(f"filter node {node.id} has no component class")
            if (
                node.type == "filter"
                and node.implementation == "python"
                and not (node.python_module and node.python_class)
            ):
                raise ValueError(f"python filter node {node.id} has no python module or class")
            if node.type == "topic" and not node.topic:
                raise ValueError(f"topic node {node.id} has no topic")
            if node.type == "topic" and not (node.input_type or node.output_type):
                raise ValueError(f"topic node {node.id} has no type")
            if node.type == "filter":
                _validate_sync_config(node)

        for edge in self.edges:
            if edge.compatibility and edge.compatibility != "ros_message":
                raise ValueError(f"unsupported edge compatibility {edge.compatibility}")
            if edge.source.direction != "output":
                raise ValueError(f"edge from.direction must be output for {edge.source.node}")
            if edge.target.direction != "input":
                raise ValueError(f"edge to.direction must be input for {edge.target.node}")
            if edge.source.node not in ids:
                raise ValueError(f"edge source {edge.source.node} does not exist")
            if edge.target.node not in ids:
                raise ValueError(f"edge target {edge.target.node} does not exist")
            if edge.source.node == edge.target.node:
                raise ValueError(f"self edge on {edge.source.node} is invalid")
            source_node = nodes_by_id[edge.source.node]
            target_node = nodes_by_id[edge.target.node]
            if source_node.type == "filter" and not _filter_port_is_valid(source_node, edge.source.port, True):
                raise ValueError(f"filter output port {source_node.id}:{edge.source.port} is not declared")
            if target_node.type == "filter" and not _filter_port_is_valid(target_node, edge.target.port, False):
                raise ValueError(f"filter input port {target_node.id}:{edge.target.port} is not declared")
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
                    _canonical_filter_port(source_node, edge.source.port, True),
                )
                if key in used_outputs:
                    raise ValueError(f"filter output {source_node.id}:{key[1]} is already connected")
                used_outputs.add(key)
            if target_node.type == "filter":
                key = (
                    target_node.id,
                    _canonical_filter_port(target_node, edge.target.port, False),
                )
                if key in used_inputs:
                    raise ValueError(f"filter input {target_node.id}:{key[1]} is already connected")
                used_inputs.add(key)
        published_topics_by_filter: dict[str, set[str]] = {}
        subscribed_topics_by_filter: dict[str, set[str]] = {}
        for edge in self.edges:
            source_node = nodes_by_id[edge.source.node]
            target_node = nodes_by_id[edge.target.node]
            if source_node.type == "filter" and target_node.type == "topic":
                topic = target_node.topic or target_node.id
                published_topics_by_filter.setdefault(source_node.id, set()).add(topic)
            elif source_node.type == "topic" and target_node.type == "filter":
                topic = source_node.topic or source_node.id
                subscribed_topics_by_filter.setdefault(target_node.id, set()).add(topic)
        for filter_id, published_topics in published_topics_by_filter.items():
            repeated_topics = published_topics & subscribed_topics_by_filter.get(filter_id, set())
            if repeated_topics:
                topic = sorted(repeated_topics)[0]
                raise ValueError(f"filter {filter_id} cannot both publish and subscribe to topic {topic}")
        topic_names = [node.topic for node in self.nodes if node.type == "topic" and node.topic]
        if len(topic_names) != len(set(topic_names)):
            raise ValueError("topic names must be unique")

    @staticmethod
    def _node_type(node: Node, outgoing: bool, port: str = "") -> str:
        if node.type == "topic":
            return _type_for_port(node.output_type or node.input_type, port, outgoing)
        return _type_for_filter_port(
            (node.output_ports or node.output_type) if outgoing else (node.input_ports or node.input_type),
            port,
            outgoing,
        )

    def to_dict(self) -> dict[str, Any]:
        self._canonicalize_edge_ports()
        return {
            "version": self.version,
            "editor": self.editor,
            "nodes": [node.to_dict() for node in self.nodes],
            "edges": [edge.to_dict() for edge in self.edges],
        }

    def _canonicalize_edge_ports(self) -> None:
        nodes_by_id = {node.id: node for node in self.nodes}
        for edge in self.edges:
            source = nodes_by_id.get(edge.source.node)
            target = nodes_by_id.get(edge.target.node)
            if source is not None and source.type == "filter":
                edge.source.port = _canonical_filter_port(source, edge.source.port, True)
            if target is not None and target.type == "filter":
                edge.target.port = _canonical_filter_port(target, edge.target.port, False)


def graph_from_dict(data: dict[str, Any]) -> Graph:
    graph = Graph(version=int(data.get("version", 0)))
    graph.editor = data.get("editor", {}) or {}
    for item in data.get("nodes", []):
        node_type = item["type"]
        node_id = item.get("id", item.get("name", item.get("topic", "")))
        parameters = item.get("parameters", {}) or {}
        sync = item.get("sync", {}) or {}
        if node_type == "filter":
            parameters = dict(parameters)
            sync = dict(sync)
            for key in ("policy", "queue_size", "slop", "max_interval"):
                if key in parameters:
                    sync.setdefault(key, parameters.pop(key))
        python_module = item.get("python_module", "")
        python_class = item.get("python_class", "")
        implementation = item.get("implementation", "")
        if not implementation:
            implementation = "python" if python_module or python_class else "cpp"
        graph.nodes.append(
            Node(
                id=node_id,
                type=node_type,
                name=item.get("name", ""),
                implementation=implementation,
                package=item.get("package", ""),
                filter=item.get("filter", ""),
                component_class=item.get("component_class", ""),
                python_module=python_module,
                python_class=python_class,
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
                PortRef(item["from"]["node"], item["from"].get("port", ""), item["from"].get("direction", "")),
                PortRef(item["to"]["node"], item["to"].get("port", ""), item["to"].get("direction", "")),
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


def _type_for_filter_port(value: str, port: str, outgoing: bool) -> str:
    ports = _split_ports(value)
    if not ports or not port or port in {"in", "out"}:
        return ""
    for index, (port_name, stream_type) in enumerate(ports):
        inferred_port = _port_name_for_type(stream_type, index, len(ports), outgoing)
        if port == stream_type or port == port_name or port == inferred_port:
            return stream_type
    return ""


def _filter_port_is_valid(node: Node, port: str, outgoing: bool) -> bool:
    spec = (node.output_ports or node.output_type) if outgoing else (node.input_ports or node.input_type)
    return bool(_type_for_filter_port(spec, port, outgoing))


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


def _canonical_filter_port(node: Node, port: str, outgoing: bool) -> str:
    spec = (node.output_ports or node.output_type) if outgoing else (node.input_ports or node.input_type)
    canonical = _canonical_port(spec, port, outgoing)
    default_port = "out" if outgoing else "in"
    configs = node.outputs if outgoing else node.inputs
    if canonical == default_port and (not port or port in {"in", "out"}) and len(configs) == 1:
        return next(iter(configs))
    return canonical


def _validate_sync_config(node: Node) -> None:
    for key, value in node.sync.items():
        if key == "policy":
            raise ValueError(f"filter node {node.id} uses removed sync.policy; use sync.mode instead")
        if key == "slop":
            raise ValueError(f"filter node {node.id} uses removed sync.slop; use sync.max_interval instead")
        if key not in {"mode", "queue_size", "max_interval"}:
            raise ValueError(f"unsupported sync option {key} on filter node {node.id}")
        if key == "mode" and value not in {"receipt_time", "latest"}:
            raise ValueError(f"unsupported sync.mode {value} on filter node {node.id}")
