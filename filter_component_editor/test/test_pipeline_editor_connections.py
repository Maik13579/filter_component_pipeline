# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

import sys
import types

from filter_component_editor.pipeline_graph import Edge, Graph, Node, PortRef


class DummyPlugin:
    pass


class DummyQt:
    Horizontal = 1
    DashLine = 2


def install_qt_stubs() -> None:
    qt_binding = types.ModuleType("python_qt_binding")
    qt_core = types.ModuleType("python_qt_binding.QtCore")
    qt_gui = types.ModuleType("python_qt_binding.QtGui")
    qt_widgets = types.ModuleType("python_qt_binding.QtWidgets")
    qt_gui_plugin = types.ModuleType("qt_gui.plugin")
    qt_gui_plugin.Plugin = DummyPlugin
    qt_core.QPointF = object
    qt_core.QRectF = object
    qt_core.Qt = DummyQt
    for name in ("QColor", "QPen"):
        setattr(qt_gui, name, object)
    for name in (
        "QDialog",
        "QDialogButtonBox",
        "QCheckBox",
        "QComboBox",
        "QFileDialog",
        "QFormLayout",
        "QFrame",
        "QGraphicsItem",
        "QGraphicsLineItem",
        "QGraphicsScene",
        "QHBoxLayout",
        "QInputDialog",
        "QLabel",
        "QLineEdit",
        "QListWidget",
        "QMessageBox",
        "QPushButton",
        "QScrollArea",
        "QTextEdit",
        "QSplitter",
        "QTabWidget",
        "QVBoxLayout",
        "QWidget",
    ):
        setattr(qt_widgets, name, object)
    sys.modules.setdefault("python_qt_binding", qt_binding)
    sys.modules.setdefault("python_qt_binding.QtCore", qt_core)
    sys.modules.setdefault("python_qt_binding.QtGui", qt_gui)
    sys.modules.setdefault("python_qt_binding.QtWidgets", qt_widgets)
    sys.modules.setdefault("qt_gui.plugin", qt_gui_plugin)


def install_editor_dependency_stubs() -> None:
    filter_discovery = types.ModuleType("filter_component_editor.filter_discovery")
    filter_discovery.FilterExport = object
    filter_discovery.discover_filters = lambda: None
    parameter_discovery = types.ModuleType("filter_component_editor.parameter_discovery")
    parameter_discovery.ComponentParameterDiscovery = object
    runtime = types.ModuleType("filter_component_editor.runtime")
    runtime.LivePipelineRuntime = object
    views = types.ModuleType("filter_component_editor.views")
    views.PipelineView = object
    items = types.ModuleType("filter_component_editor.items")
    items.EdgeHandleItem = object
    items.EdgeItem = object
    items.NodeItem = object
    sys.modules.setdefault("filter_component_editor.filter_discovery", filter_discovery)
    sys.modules.setdefault("filter_component_editor.parameter_discovery", parameter_discovery)
    sys.modules.setdefault("filter_component_editor.runtime", runtime)
    sys.modules.setdefault("filter_component_editor.views", views)
    sys.modules.setdefault("filter_component_editor.items", items)


install_qt_stubs()
install_editor_dependency_stubs()

from filter_component_editor.pipeline_editor import PipelineEditor, ROS_MESSAGE_COMPATIBILITY


def output(node: str, port: str = "") -> PortRef:
    return PortRef(node, port, "output")


def input_(node: str, port: str = "") -> PortRef:
    return PortRef(node, port, "input")


def editor_for(graph: Graph) -> PipelineEditor:
    editor = object.__new__(PipelineEditor)
    editor.graph = graph
    editor.message_type_by_logical = {
        "PointXYZ": "sensor_msgs/msg/PointCloud2",
        "PointXYZI": "sensor_msgs/msg/PointCloud2",
        "PointIndices": "pcl_msgs/msg/PointIndices",
    }
    return editor


class FakeNodeItem:
    def __init__(self, node: Node) -> None:
        self.node = node
        self.visible = True

    def setVisible(self, visible: bool) -> None:
        self.visible = visible


def topic(node_id: str, stream_type: str) -> Node:
    return Node(id=node_id, type="topic", topic=node_id, input_type=stream_type, output_type=stream_type)


def filter_node(
    node_id: str,
    input_type: str = "PointXYZI",
    output_type: str = "PointXYZI",
    input_ports: str = "",
    output_ports: str = "",
) -> Node:
    return Node(
        id=node_id,
        name=node_id,
        type="filter",
        package="pcl_filter_components_xyzi",
        filter="VoxelGridXYZI",
        input_type=input_type,
        output_type=output_type,
        input_ports=input_ports,
        output_ports=output_ports,
    )


def test_connection_verdict_returns_exact_for_matching_types() -> None:
    source = topic("/points", "PointXYZI")
    target = filter_node("voxel")
    editor = editor_for(Graph(nodes=[source, target]))

    verdict = editor._connection_verdict(source, "out", target, "in")

    assert verdict.verdict == "exact"


def test_connection_verdict_returns_ros_message_for_message_compatible_types() -> None:
    source = topic("/points", "PointXYZ")
    target = filter_node("voxel", input_type="PointXYZI")
    editor = editor_for(Graph(nodes=[source, target]))

    verdict = editor._connection_verdict(source, "out", target, "in")

    assert verdict.verdict == ROS_MESSAGE_COMPATIBILITY


def test_connection_verdict_returns_invalid_for_incompatible_types() -> None:
    source = topic("/indices", "PointIndices")
    target = filter_node("voxel", input_type="PointXYZI")
    editor = editor_for(Graph(nodes=[source, target]))

    verdict = editor._connection_verdict(source, "out", target, "in")

    assert verdict.verdict == "invalid"
    assert "expects PointXYZI" in verdict.reason


def test_connection_verdict_returns_unavailable_for_occupied_input() -> None:
    source = topic("/points_a", "PointXYZI")
    existing = topic("/points_b", "PointXYZI")
    target = filter_node("voxel", input_ports="cloud:PointXYZI")
    graph = Graph(
        nodes=[source, existing, target],
        edges=[Edge(output("/points_b"), input_("voxel", "cloud"))],
    )
    editor = editor_for(graph)

    verdict = editor._connection_verdict(source, "out", target, "cloud")

    assert verdict.verdict == "unavailable"
    assert "already connected" in verdict.reason


def test_connection_verdict_returns_invalid_for_same_node() -> None:
    node = filter_node("voxel")
    editor = editor_for(Graph(nodes=[node]))

    verdict = editor._connection_verdict(node, "out", node, "in")

    assert verdict.verdict == "invalid"
    assert "itself" in verdict.reason


def test_refresh_topic_visibility_hides_only_topic_items() -> None:
    source = filter_node("source")
    topic_node = topic("/between", "PointXYZI")
    editor = editor_for(Graph(nodes=[source, topic_node]))
    source_item = FakeNodeItem(source)
    topic_item = FakeNodeItem(topic_node)
    editor.items_by_id = {source.id: source_item, topic_node.id: topic_item}
    editor.show_topics = False

    editor._refresh_topic_visibility()

    assert source_item.visible is True
    assert topic_item.visible is False


def test_hidden_topic_edges_collapse_filter_topic_filter_path() -> None:
    source = filter_node("source", output_ports="cloud:PointXYZI")
    topic_node = topic("/between", "PointXYZI")
    target = filter_node("target", input_ports="cloud:PointXYZI")
    graph = Graph(
        nodes=[source, topic_node, target],
        edges=[
            Edge(output("source", "cloud"), input_("/between", "in")),
            Edge(output("/between", "out"), input_("target", "cloud")),
        ],
    )
    editor = editor_for(graph)
    editor.items_by_id = {
        source.id: FakeNodeItem(source),
        target.id: FakeNodeItem(target),
    }
    rendered = []
    editor._add_visual_edge_item = lambda edge, source_item, target_item: rendered.append(
        (edge, source_item.node.id, target_item.node.id)
    )
    editor._add_edge_item = lambda edge, source_item, target_item: None

    editor._rebuild_collapsed_topic_edges()

    assert len(rendered) == 1
    visual_edge, source_id, target_id = rendered[0]
    assert source_id == "source"
    assert target_id == "target"
    assert visual_edge.source.node == "source"
    assert visual_edge.source.port == "cloud"
    assert visual_edge.target.node == "target"
    assert visual_edge.target.port == "cloud"
