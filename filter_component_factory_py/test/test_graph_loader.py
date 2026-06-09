# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

from filter_component_factory_py.component_container import LoadedPythonNode
from filter_component_factory_py.component_container import PythonComponentContainer
from filter_component_factory_py.graph_loader import load_pipeline_graph


def test_loads_only_python_filter_nodes(tmp_path: Path) -> None:
    graph_file = tmp_path / "pipeline.yaml"
    graph_file.write_text(
        """
version: 2
nodes:
  - type: filter
    name: CppFilter
    component_class: test_pkg::CppFilterComponent
    input_type: PointXYZI
    output_type: PointXYZI
  - type: filter
    name: PyFilter
    python_module: test_pkg.filters
    python_class: PyFilter
    input_type: PointCloud2
    output_type: PointCloud2
edges: []
""",
        encoding="utf-8",
    )

    graph = load_pipeline_graph(graph_file)

    assert [node.id for node in graph.python_nodes] == ["PyFilter"]


def test_container_destroy_releases_lifecycle_state_machine() -> None:
    class FakeExecutor:
        def __init__(self) -> None:
            self.removed = []

        def remove_node(self, node) -> None:
            self.removed.append(node)

    class FakeStateMachine:
        def __init__(self) -> None:
            self.destroyed = False

        def destroy_when_not_in_use(self) -> None:
            self.destroyed = True

    class FakeNode:
        def __init__(self) -> None:
            self.destroyed = False
            self._state_machine = FakeStateMachine()

        def destroy_node(self) -> None:
            self.destroyed = True

    container = object.__new__(PythonComponentContainer)
    executor = FakeExecutor()
    node = FakeNode()
    state_machine = node._state_machine
    container._executor = executor
    container._loaded = {
        7: LoadedPythonNode(
            unique_id=7,
            node_id="PyFilter",
            full_node_name="/PyFilter",
            instance=node,
        )
    }

    container._destroy_loaded_node(7)

    assert executor.removed == [node]
    assert node.destroyed is True
    assert state_machine.destroyed is True
    assert node._state_machine is None
    assert container._loaded == {}
