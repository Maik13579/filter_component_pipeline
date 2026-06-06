# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

import pytest

from pcl_filter_components.filter_discovery import discover_filters
from pcl_filter_components.pipeline_graph import Edge, Graph, Node, PortRef, load_graph, save_graph


def test_graph_round_trip_preserves_editor_fields(tmp_path: Path) -> None:
    graph = Graph(
        nodes=[
            Node(
                id="input_1",
                type="input",
                topic="/points",
                output_type="PointXYZI",
                position={"x": 10.0, "y": 20.0},
            ),
            Node(
                id="voxel_1",
                type="filter",
                package="pcl_filter_components",
                filter="VoxelGridXYZI",
                component_class="pcl_filter_components::VoxelGridXYZIComponent",
                input_type="PointXYZI",
                output_type="PointXYZI",
                optional_output_type="PointIndices",
                parameters={"filter.leaf_size_x": 0.1},
                qos={"depth": 5},
                sync={"policy": "ExactTime"},
            ),
            Node(id="output_1", type="output", topic="/filtered", input_type="PointXYZI"),
        ],
        edges=[
            Edge(PortRef("input_1", "out"), PortRef("voxel_1", "in")),
            Edge(PortRef("voxel_1", "out"), PortRef("output_1", "in")),
        ],
    )
    path = tmp_path / "pipeline.yaml"

    save_graph(graph, str(path))
    loaded = load_graph(str(path))

    assert loaded.nodes[1].input_type == "PointXYZI"
    assert loaded.nodes[1].output_type == "PointXYZI"
    assert loaded.nodes[1].optional_output_type == "PointIndices"
    assert loaded.nodes[1].parameters["filter.leaf_size_x"] == 0.1
    assert loaded.nodes[1].qos["depth"] == 5
    assert loaded.nodes[1].sync["policy"] == "ExactTime"
    assert loaded.nodes[0].position == {"x": 10.0, "y": 20.0}
    assert len(loaded.edges) == 2


def test_graph_rejects_incompatible_custom_types() -> None:
    graph = Graph(
        nodes=[
            Node(id="input_1", type="input", topic="/indices", output_type="PointIndices"),
            Node(
                id="filter_1",
                type="filter",
                package="pcl_filter_components",
                filter="VoxelGridXYZI",
                input_type="PointXYZI",
                output_type="PointXYZI",
            ),
        ],
        edges=[Edge(PortRef("input_1"), PortRef("filter_1"))],
    )

    with pytest.raises(ValueError):
        graph.validate({"input_1": "PointIndices", "filter_1": "PointXYZI"})


def test_discovery_reads_filter_and_type_adapter_exports() -> None:
    discovery = discover_filters()

    filters = {(item.package, item.filter): item for item in discovery.filters}
    assert ("pcl_filter_components", "VoxelGridXYZI") in filters
    voxel = filters[("pcl_filter_components", "VoxelGridXYZI")]
    assert voxel.component_class == "pcl_filter_components::VoxelGridXYZIComponent"
    assert voxel.input_type == "PointXYZI"
    assert voxel.output_type == "PointXYZI"
    assert voxel.optional_output_type == "PointIndices"

    types = {(item.package, item.point_type): item for item in discovery.types}
    assert types[("pcl_filter_components", "PointXYZI")].message_type == "sensor_msgs/msg/PointCloud2"
    assert (
        types[("pcl_filter_components", "PointXYZI")].type_adapter
        == "pcl_filter_components::ros::PclCloudAdapterPointXYZI"
    )
    assert types[("pcl_filter_components", "PointIndices")].message_type == "pcl_msgs/msg/PointIndices"
