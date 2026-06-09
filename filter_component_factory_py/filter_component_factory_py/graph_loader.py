# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import yaml


@dataclass
class PipelineNode:
    id: str
    type: str
    implementation: str = "cpp"
    name: str = ""
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
    inputs: dict[str, dict[str, Any]] = field(default_factory=dict)
    outputs: dict[str, dict[str, Any]] = field(default_factory=dict)
    sync: dict[str, Any] = field(default_factory=dict)


@dataclass
class PipelineGraph:
    version: int
    nodes: list[PipelineNode]
    edges: list[dict[str, Any]]

    @property
    def python_nodes(self) -> list[PipelineNode]:
        return [
            node
            for node in self.nodes
            if node.type == "filter" and node.implementation == "python"
        ]


def load_pipeline_graph(path: str | Path) -> PipelineGraph:
    data = yaml.safe_load(Path(path).read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise ValueError("Pipeline YAML must contain a top-level mapping")
    version = int(data.get("version", 0))
    if version != 2:
        raise ValueError(f"Unsupported pipeline graph version {version}")
    nodes = [_parse_node(item) for item in data.get("nodes", [])]
    return PipelineGraph(version=version, nodes=nodes, edges=list(data.get("edges", [])))


def _parse_node(item: dict[str, Any]) -> PipelineNode:
    node_type = str(item.get("type", ""))
    name = str(item.get("name", ""))
    topic = str(item.get("topic", ""))
    node_id = str(item.get("id", name or topic))
    python_module = str(item.get("python_module", ""))
    python_class = str(item.get("python_class", ""))
    implementation = str(item.get("implementation", ""))
    if not implementation:
        implementation = "python" if python_module or python_class else "cpp"
    if implementation not in {"cpp", "python"}:
        raise ValueError(f"Unsupported node implementation '{implementation}' for '{node_id}'")
    node = PipelineNode(
        id=node_id,
        type=node_type,
        implementation=implementation,
        name=name,
        package=str(item.get("package", "")),
        filter=str(item.get("filter", "")),
        component_class=str(item.get("component_class", "")),
        python_module=python_module,
        python_class=python_class,
        input_type=str(item.get("input_type", "")),
        output_type=str(item.get("output_type", "")),
        input_ports=str(item.get("input_ports", "")),
        output_ports=str(item.get("output_ports", "")),
        topic=topic,
        parameters=dict(item.get("parameters", {}) or {}),
        inputs=dict(item.get("inputs", {}) or {}),
        outputs=dict(item.get("outputs", {}) or {}),
        sync=dict(item.get("sync", {}) or {}),
    )
    if node.type == "filter" and node.implementation == "python":
        if not node.python_module or not node.python_class:
            raise ValueError(f"Python filter node '{node.id}' requires python_module and python_class")
    return node
