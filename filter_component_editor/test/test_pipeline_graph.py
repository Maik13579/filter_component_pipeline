# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

import pytest

from filter_component_editor.pipeline_graph import Edge, Graph, Node, PortRef, graph_from_dict, load_graph, save_graph


def output(node: str, port: str = "") -> PortRef:
    return PortRef(node, port, "output")


def input_(node: str, port: str = "") -> PortRef:
    return PortRef(node, port, "input")


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
                package="pcl_filter_components_xyzi",
                filter="VoxelGridXYZI",
                component_class="pcl_filter_components_xyzi::VoxelGridXYZIComponent",
                input_type="PointXYZI",
                output_type="PointXYZI",
                parameters={"filter.leaf_size_x": 0.1},
                inputs={"cloud": {"qos": {"reliability": "reliable", "history": "keep_last", "depth": 8}}},
                outputs={"cloud": {"qos": {"durability": "transient_local"}}},
                sync={"mode": "receipt_time", "max_interval": 0.1},
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
            Edge(output("/points", "out"), input_("VoxelGridXYZI_1", "in")),
            Edge(output("VoxelGridXYZI_1", "out"), input_("/pcl_pipeline/voxel_to_output", "in")),
            Edge(output("/pcl_pipeline/voxel_to_output", "out"), input_("/filtered", "in")),
        ],
    )
    path = tmp_path / "pipeline.yaml"

    save_graph(graph, str(path))
    assert "id: /pcl_pipeline/voxel_to_output" not in path.read_text(encoding="utf-8")
    assert "id: VoxelGridXYZI_1" not in path.read_text(encoding="utf-8")
    assert "id: /points" not in path.read_text(encoding="utf-8")
    assert "id: /filtered" not in path.read_text(encoding="utf-8")
    assert "name: VoxelGridXYZI_1" in path.read_text(encoding="utf-8")
    assert "inputs:" in path.read_text(encoding="utf-8")
    assert "outputs:" in path.read_text(encoding="utf-8")
    assert "compatibility:" not in path.read_text(encoding="utf-8")
    assert "transient_local" in path.read_text(encoding="utf-8")
    assert "qos:\n      depth: 7" not in path.read_text(encoding="utf-8")
    assert "version: 2" in path.read_text(encoding="utf-8")
    assert "direction: output" in path.read_text(encoding="utf-8")
    assert "direction: input" in path.read_text(encoding="utf-8")
    assert "port: cloud" in path.read_text(encoding="utf-8")
    loaded = load_graph(str(path))

    assert loaded.nodes[1].input_type == "PointXYZI"
    assert loaded.nodes[1].output_type == "PointXYZI"
    assert loaded.nodes[1].parameters["filter.leaf_size_x"] == 0.1
    assert loaded.nodes[1].inputs["cloud"]["qos"]["depth"] == 8
    assert loaded.nodes[1].outputs["cloud"]["qos"]["durability"] == "transient_local"
    assert loaded.nodes[1].sync["mode"] == "receipt_time"
    assert loaded.nodes[1].sync["max_interval"] == 0.1
    assert loaded.nodes[0].position == {"x": 10.0, "y": 20.0}
    assert loaded.editor == {"orientation": "top_down"}
    assert loaded.nodes[2].topic == "/pcl_pipeline/voxel_to_output"
    assert loaded.nodes[2].qos == {}
    assert loaded.nodes[2].position == {"x": 120.0, "y": 80.0}
    assert len(loaded.edges) == 3


def test_graph_load_ignores_old_edge_qos() -> None:
    loaded = graph_from_dict(
        {
            "version": 2,
            "nodes": [
                {"type": "topic", "topic": "/a", "input_type": "PointXYZI", "output_type": "PointXYZI"},
                {
                    "type": "filter",
                    "name": "VoxelGridXYZI_1",
                    "package": "pcl_filter_components_xyzi",
                    "filter": "VoxelGridXYZI",
                    "input_type": "PointXYZI",
                    "output_type": "PointXYZI",
                },
            ],
            "edges": [
                {
                    "from": {"node": "/a", "port": "out", "direction": "output"},
                    "to": {"node": "VoxelGridXYZI_1", "port": "in", "direction": "input"},
                    "qos": {"reliability": "reliable"},
                }
            ],
        }
    )

    assert loaded.edges[0].qos == {}


def test_graph_load_rejects_missing_endpoint_direction() -> None:
    with pytest.raises(ValueError, match="from.direction"):
        graph_from_dict(
            {
                "version": 2,
                "nodes": [
                    {"type": "topic", "topic": "/a", "input_type": "PointXYZI", "output_type": "PointXYZI"},
                    {
                        "type": "filter",
                        "name": "VoxelGridXYZI_1",
                        "package": "pcl_filter_components_xyzi",
                        "filter": "VoxelGridXYZI",
                        "input_type": "PointXYZI",
                        "output_type": "PointXYZI",
                    },
                ],
                "edges": [
                    {
                        "from": {"node": "/a", "port": "out"},
                        "to": {"node": "VoxelGridXYZI_1", "port": "cloud", "direction": "input"},
                    }
                ],
            }
        )


def test_graph_load_rejects_wrong_endpoint_direction() -> None:
    with pytest.raises(ValueError, match="from.direction"):
        graph_from_dict(
            {
                "version": 2,
                "nodes": [
                    {"type": "topic", "topic": "/a", "input_type": "PointXYZI", "output_type": "PointXYZI"},
                    {
                        "type": "filter",
                        "name": "VoxelGridXYZI_1",
                        "package": "pcl_filter_components_xyzi",
                        "filter": "VoxelGridXYZI",
                        "input_type": "PointXYZI",
                        "output_type": "PointXYZI",
                    },
                ],
                "edges": [
                    {
                        "from": {"node": "/a", "port": "out", "direction": "input"},
                        "to": {"node": "VoxelGridXYZI_1", "port": "cloud", "direction": "input"},
                    }
                ],
            }
        )
    with pytest.raises(ValueError, match="to.direction"):
        graph_from_dict(
            {
                "version": 2,
                "nodes": [
                    {"type": "topic", "topic": "/a", "input_type": "PointXYZI", "output_type": "PointXYZI"},
                    {
                        "type": "filter",
                        "name": "VoxelGridXYZI_1",
                        "package": "pcl_filter_components_xyzi",
                        "filter": "VoxelGridXYZI",
                        "input_type": "PointXYZI",
                        "output_type": "PointXYZI",
                    },
                ],
                "edges": [
                    {
                        "from": {"node": "/a", "port": "out", "direction": "output"},
                        "to": {"node": "VoxelGridXYZI_1", "port": "cloud", "direction": "output"},
                    }
                ],
            }
        )


def test_graph_load_rejects_version_1() -> None:
    with pytest.raises(ValueError, match="unsupported graph version 1"):
        graph_from_dict({"version": 1, "nodes": [], "edges": []})


def test_graph_save_canonicalizes_same_named_filter_ports(tmp_path: Path) -> None:
    graph = Graph(
        nodes=[
            Node(id="/input", type="topic", topic="/input", input_type="PointXYZI", output_type="PointXYZI"),
            Node(
                id="VoxelGridXYZI_1",
                name="VoxelGridXYZI_1",
                type="filter",
                package="pcl_filter_components_xyzi",
                filter="VoxelGridXYZI",
                input_ports="cloud:PointXYZI",
                output_ports="cloud:PointXYZI",
            ),
            Node(id="/output", type="topic", topic="/output", input_type="PointXYZI", output_type="PointXYZI"),
        ],
        edges=[
            Edge(output("/input", "out"), input_("VoxelGridXYZI_1", "in")),
            Edge(output("VoxelGridXYZI_1", "out"), input_("/output", "in")),
        ],
    )
    path = tmp_path / "pipeline.yaml"

    save_graph(graph, str(path))
    loaded = load_graph(str(path))

    assert loaded.edges[0].target.port == "cloud"
    assert loaded.edges[0].target.direction == "input"
    assert loaded.edges[1].source.port == "cloud"
    assert loaded.edges[1].source.direction == "output"


def test_graph_load_preserves_sync_parameters() -> None:
    loaded = graph_from_dict(
        {
            "version": 2,
            "nodes": [
                {
                    "type": "filter",
                    "name": "PointCloudMergerXYZI_1",
                    "package": "pcl_filter_components_xyzi",
                    "filter": "PointCloudMergerXYZI",
                    "input_type": "PointXYZI,PointXYZI",
                    "output_type": "PointXYZI",
                    "parameters": {
                        "queue_size": 5,
                    },
                    "sync": {"mode": "receipt_time", "max_interval": 0.2},
                },
            ],
        }
    )

    assert "queue_size" not in loaded.nodes[0].parameters
    assert loaded.nodes[0].sync["mode"] == "receipt_time"
    assert loaded.nodes[0].sync["queue_size"] == 5
    assert loaded.nodes[0].sync["max_interval"] == 0.2


def test_graph_load_rejects_removed_sync_policy() -> None:
    with pytest.raises(ValueError, match="sync.policy"):
        graph_from_dict(
            {
                "version": 2,
                "nodes": [
                    {
                        "type": "filter",
                        "name": "PointCloudMergerXYZI_1",
                        "package": "pcl_filter_components_xyzi",
                        "filter": "PointCloudMergerXYZI",
                        "input_type": "PointXYZI,PointXYZI",
                        "output_type": "PointXYZI",
                        "sync": {"policy": "ExactTime"},
                    },
                ],
            }
        )


def test_graph_round_trip_preserves_ros_message_compatibility(tmp_path: Path) -> None:
    graph = Graph(
        nodes=[
            Node(id="/points", type="topic", topic="/points", input_type="PointXYZ", output_type="PointXYZ"),
            Node(
                id="VoxelGridXYZI_1",
                name="VoxelGridXYZI_1",
                type="filter",
                package="pcl_filter_components_xyzi",
                filter="VoxelGridXYZI",
                input_type="PointXYZI",
                output_type="PointXYZI",
            ),
        ],
        edges=[Edge(output("/points", "out"), input_("VoxelGridXYZI_1", "in"), compatibility="ros_message")],
    )
    path = tmp_path / "pipeline.yaml"

    save_graph(graph, str(path))
    text = path.read_text(encoding="utf-8")
    loaded = load_graph(str(path))

    assert "compatibility: ros_message" in text
    assert loaded.edges[0].compatibility == "ros_message"


def test_graph_accepts_marked_ros_message_compatible_type_mismatch() -> None:
    graph = Graph(
        nodes=[
            Node(id="/points", type="topic", topic="/points", input_type="PointXYZ", output_type="PointXYZ"),
            Node(
                id="VoxelGridXYZI_1",
                name="VoxelGridXYZI_1",
                type="filter",
                package="pcl_filter_components_xyzi",
                filter="VoxelGridXYZI",
                input_type="PointXYZI",
                output_type="PointXYZI",
            ),
        ],
        edges=[Edge(output("/points"), input_("VoxelGridXYZI_1"), compatibility="ros_message")],
    )

    graph.validate(
        message_type_by_logical={
            "PointXYZ": "sensor_msgs/msg/PointCloud2",
            "PointXYZI": "sensor_msgs/msg/PointCloud2",
        }
    )


def test_graph_rejects_marked_ros_message_type_mismatch_when_ros_types_differ() -> None:
    graph = Graph(
        nodes=[
            Node(id="/indices", type="topic", topic="/indices", input_type="PointIndices", output_type="PointIndices"),
            Node(
                id="VoxelGridXYZI_1",
                name="VoxelGridXYZI_1",
                type="filter",
                package="pcl_filter_components_xyzi",
                filter="VoxelGridXYZI",
                input_type="PointXYZI",
                output_type="PointXYZI",
            ),
        ],
        edges=[Edge(output("/indices"), input_("VoxelGridXYZI_1"), compatibility="ros_message")],
    )

    with pytest.raises(ValueError, match="type mismatch"):
        graph.validate(
            message_type_by_logical={
                "PointIndices": "pcl_msgs/msg/PointIndices",
                "PointXYZI": "sensor_msgs/msg/PointCloud2",
            }
        )


def test_graph_rejects_incompatible_custom_types() -> None:
    graph = Graph(
        nodes=[
            Node(id="/indices", type="topic", topic="/indices", input_type="PointIndices", output_type="PointIndices"),
            Node(
                id="VoxelGridXYZI_1",
                name="VoxelGridXYZI_1",
                type="filter",
                package="pcl_filter_components_xyzi",
                filter="VoxelGridXYZI",
                input_type="PointXYZI",
                output_type="PointXYZI",
            ),
        ],
        edges=[Edge(output("/indices"), input_("VoxelGridXYZI_1"))],
    )

    with pytest.raises(ValueError):
        graph.validate({"/indices": "PointIndices", "VoxelGridXYZI_1": "PointXYZI"})


def test_graph_rejects_topic_type_mismatch() -> None:
    graph = Graph(
        nodes=[
            Node(id="filter_1", type="filter", package="pcl_filter_components_xyzi", filter="VoxelGridXYZI", output_type="PointXYZI"),
            Node(id="/indices", type="topic", topic="/indices", input_type="PointIndices", output_type="PointIndices"),
        ],
        edges=[Edge(output("filter_1"), input_("/indices"))],
    )

    with pytest.raises(ValueError):
        graph.validate()


def test_graph_accepts_original_cloud_output_port() -> None:
    graph = Graph(
        nodes=[
            Node(
                id="VoxelGridXYZI_1",
                name="VoxelGridXYZI_1",
                type="filter",
                package="pcl_filter_components_xyzi",
                filter="VoxelGridXYZI",
                output_type="PointXYZI",
            ),
            Node(id="/original", type="topic", topic="/original", input_type="PointXYZI", output_type="PointXYZI"),
        ],
        edges=[Edge(output("VoxelGridXYZI_1", "orig_cloud"), input_("/original", "in"))],
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
                package="pcl_filter_components_xyzi",
                filter="PointCloudMergerXYZI",
                input_type="PointXYZI,PointXYZI",
                output_type="PointXYZI",
            ),
        ],
        edges=[
            Edge(output("/a", "out"), input_("PointCloudMergerXYZI_1", "input_1")),
            Edge(output("/b", "out"), input_("PointCloudMergerXYZI_1", "input_2")),
        ],
    )

    graph.validate()


def test_graph_rejects_duplicate_filter_input_port() -> None:
    graph = Graph(
        nodes=[
            Node(id="/a", type="topic", topic="/a", input_type="PointXYZI", output_type="PointXYZI"),
            Node(id="/b", type="topic", topic="/b", input_type="PointXYZI", output_type="PointXYZI"),
            Node(
                id="PointCloudMergerXYZI_1",
                name="PointCloudMergerXYZI_1",
                type="filter",
                package="pcl_filter_components_xyzi",
                filter="PointCloudMergerXYZI",
                input_type="PointXYZI,PointXYZI",
                output_type="PointXYZI",
            ),
        ],
        edges=[
            Edge(output("/a", "out"), input_("PointCloudMergerXYZI_1", "input_1")),
            Edge(output("/b", "out"), input_("PointCloudMergerXYZI_1", "input_1")),
        ],
    )

    with pytest.raises(ValueError, match="already connected"):
        graph.validate()


def test_graph_rejects_duplicate_filter_output_port() -> None:
    graph = Graph(
        nodes=[
            Node(
                id="VoxelGridXYZI_1",
                name="VoxelGridXYZI_1",
                type="filter",
                package="pcl_filter_components_xyzi",
                filter="VoxelGridXYZI",
                input_type="PointXYZI",
                output_type="PointXYZI",
            ),
            Node(id="/a", type="topic", topic="/a", input_type="PointXYZI", output_type="PointXYZI"),
            Node(id="/b", type="topic", topic="/b", input_type="PointXYZI", output_type="PointXYZI"),
        ],
        edges=[
            Edge(output("VoxelGridXYZI_1", "out"), input_("/a", "in")),
            Edge(output("VoxelGridXYZI_1", "out"), input_("/b", "in")),
        ],
    )

    with pytest.raises(ValueError, match="already connected"):
        graph.validate()


def test_graph_accepts_explicit_repeated_output_ports() -> None:
    graph = Graph(
        nodes=[
            Node(
                id="VoxelGridXYZI_1",
                name="VoxelGridXYZI_1",
                type="filter",
                package="pcl_filter_components_xyzi",
                filter="VoxelGridXYZI",
                input_type="PointXYZI",
                output_type="PointXYZI,PointXYZI",
                output_ports="cloud:PointXYZI,orig_cloud:PointXYZI",
            ),
            Node(id="/filtered", type="topic", topic="/filtered", input_type="PointXYZI", output_type="PointXYZI"),
            Node(id="/original", type="topic", topic="/original", input_type="PointXYZI", output_type="PointXYZI"),
        ],
        edges=[
            Edge(output("VoxelGridXYZI_1", "cloud"), input_("/filtered", "in")),
            Edge(output("VoxelGridXYZI_1", "orig_cloud"), input_("/original", "in")),
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
