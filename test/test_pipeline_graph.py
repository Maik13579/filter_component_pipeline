# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

import pytest

from pcl_filter_components.filter_discovery import discover_filters
from pcl_filter_components.pipeline_graph import Edge, Graph, Node, PortRef, load_graph, save_graph


def test_graph_round_trip_preserves_editor_fields(tmp_path: Path) -> None:
    graph = Graph(
        editor={"orientation": "top_down"},
        nodes=[
            Node(
                id="/points",
                type="topic",
                topic="/points",
                input_type="PointXYZI",
                output_type="PointXYZI",
                position={"x": 10.0, "y": 20.0},
            ),
            Node(
                id="VoxelGridXYZI_1",
                name="VoxelGridXYZI_1",
                type="filter",
                package="pcl_filter_components",
                filter="VoxelGridXYZI",
                component_class="pcl_filter_components::VoxelGridXYZIComponent",
                input_type="PointXYZI",
                output_type="PointXYZI,PointIndices",
                parameters={"filter.leaf_size_x": 0.1},
                sync={"policy": "ExactTime"},
            ),
            Node(
                id="/pcl_pipeline/voxel_to_output",
                type="topic",
                topic="/pcl_pipeline/voxel_to_output",
                input_type="PointXYZI",
                output_type="PointXYZI",
                qos={"depth": 7, "reliability": "reliable"},
                position={"x": 120.0, "y": 80.0},
            ),
            Node(id="/filtered", type="topic", topic="/filtered", input_type="PointXYZI", output_type="PointXYZI"),
        ],
        edges=[
            Edge(PortRef("/points", "out"), PortRef("VoxelGridXYZI_1", "in")),
            Edge(PortRef("VoxelGridXYZI_1", "out"), PortRef("/pcl_pipeline/voxel_to_output", "in")),
            Edge(PortRef("/pcl_pipeline/voxel_to_output", "out"), PortRef("/filtered", "in")),
        ],
    )
    path = tmp_path / "pipeline.yaml"

    save_graph(graph, str(path))
    assert "id: /pcl_pipeline/voxel_to_output" not in path.read_text(encoding="utf-8")
    assert "id: VoxelGridXYZI_1" not in path.read_text(encoding="utf-8")
    assert "id: /points" not in path.read_text(encoding="utf-8")
    assert "id: /filtered" not in path.read_text(encoding="utf-8")
    assert "name: VoxelGridXYZI_1" in path.read_text(encoding="utf-8")
    loaded = load_graph(str(path))

    assert loaded.nodes[1].input_type == "PointXYZI"
    assert loaded.nodes[1].output_type == "PointXYZI,PointIndices"
    assert loaded.nodes[1].parameters["filter.leaf_size_x"] == 0.1
    assert loaded.nodes[1].sync["policy"] == "ExactTime"
    assert loaded.nodes[0].position == {"x": 10.0, "y": 20.0}
    assert loaded.editor == {"orientation": "top_down"}
    assert loaded.nodes[2].topic == "/pcl_pipeline/voxel_to_output"
    assert loaded.nodes[2].qos == {"depth": 7, "reliability": "reliable"}
    assert loaded.nodes[2].position == {"x": 120.0, "y": 80.0}
    assert len(loaded.edges) == 3


def test_graph_rejects_incompatible_custom_types() -> None:
    graph = Graph(
        nodes=[
            Node(id="/indices", type="topic", topic="/indices", input_type="PointIndices", output_type="PointIndices"),
            Node(
                id="VoxelGridXYZI_1",
                name="VoxelGridXYZI_1",
                type="filter",
                package="pcl_filter_components",
                filter="VoxelGridXYZI",
                input_type="PointXYZI",
                output_type="PointXYZI",
            ),
        ],
        edges=[Edge(PortRef("/indices"), PortRef("VoxelGridXYZI_1"))],
    )

    with pytest.raises(ValueError):
        graph.validate({"/indices": "PointIndices", "VoxelGridXYZI_1": "PointXYZI"})


def test_graph_rejects_topic_type_mismatch() -> None:
    graph = Graph(
        nodes=[
            Node(id="filter_1", type="filter", package="pcl_filter_components", filter="VoxelGridXYZI", output_type="PointXYZI"),
            Node(id="/indices", type="topic", topic="/indices", input_type="PointIndices", output_type="PointIndices"),
        ],
        edges=[Edge(PortRef("filter_1"), PortRef("/indices"))],
    )

    with pytest.raises(ValueError):
        graph.validate()


def test_graph_accepts_indices_output_port() -> None:
    graph = Graph(
        nodes=[
            Node(
                id="VoxelGridXYZI_1",
                name="VoxelGridXYZI_1",
                type="filter",
                package="pcl_filter_components",
                filter="VoxelGridXYZI",
                output_type="PointXYZI,PointIndices",
            ),
            Node(id="/indices", type="topic", topic="/indices", input_type="PointIndices", output_type="PointIndices"),
        ],
        edges=[Edge(PortRef("VoxelGridXYZI_1", "indices"), PortRef("/indices", "in"))],
    )

    graph.validate()


def test_graph_accepts_repeated_input_ports() -> None:
    graph = Graph(
        nodes=[
            Node(id="/a", type="topic", topic="/a", input_type="PointXYZI", output_type="PointXYZI"),
            Node(id="/b", type="topic", topic="/b", input_type="PointXYZI", output_type="PointXYZI"),
            Node(
                id="PointCloudMergerXYZI_1",
                name="PointCloudMergerXYZI_1",
                type="filter",
                package="pcl_filter_components",
                filter="PointCloudMergerXYZI",
                input_type="PointXYZI,PointXYZI",
                output_type="PointXYZI",
            ),
        ],
        edges=[
            Edge(PortRef("/a", "out"), PortRef("PointCloudMergerXYZI_1", "input_1")),
            Edge(PortRef("/b", "out"), PortRef("PointCloudMergerXYZI_1", "input_2")),
        ],
    )

    graph.validate()


def test_graph_rejects_duplicate_topic_nodes() -> None:
    graph = Graph(
        nodes=[
            Node(id="/duplicate", type="topic", topic="/duplicate", input_type="PointXYZI", output_type="PointXYZI"),
            Node(id="/duplicate_2", type="topic", topic="/duplicate", input_type="PointXYZI", output_type="PointXYZI"),
        ],
    )

    with pytest.raises(ValueError):
        graph.validate()


def test_discovery_reads_filter_and_type_adapter_exports() -> None:
    discovery = discover_filters()

    filters = {(item.package, item.filter): item for item in discovery.filters}
    assert ("pcl_filter_components", "VoxelGridXYZI") in filters
    assert ("pcl_filter_components", "PointCloudMergerXYZI") in filters
    voxel = filters[("pcl_filter_components", "VoxelGridXYZI")]
    assert voxel.component_class == "pcl_filter_components::VoxelGridXYZIComponent"
    assert voxel.input_type == "PointXYZI"
    assert voxel.output_type == "PointXYZI,PointIndices"
    merger = filters[("pcl_filter_components", "PointCloudMergerXYZI")]
    assert merger.input_type == "PointXYZI,PointXYZI"
    assert merger.output_type == "PointXYZI"

    types = {(item.package, item.point_type): item for item in discovery.types}
    assert types[("pcl_filter_components", "PointXYZI")].message_type == "sensor_msgs/msg/PointCloud2"
    assert (
        types[("pcl_filter_components", "PointXYZI")].type_adapter
        == "pcl_filter_components::ros::PclCloudAdapterPointXYZI"
    )
    assert types[("pcl_filter_components", "PointIndices")].message_type == "pcl_msgs/msg/PointIndices"
