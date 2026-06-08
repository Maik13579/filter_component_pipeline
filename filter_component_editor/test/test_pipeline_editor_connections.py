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
    filter_discovery.FilterPluginExport = object
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


def chain_editor_for(graph: Graph) -> PipelineEditor:
    editor = editor_for(graph)
    editor.discovery = types.SimpleNamespace(
        filters=[
            types.SimpleNamespace(
                package="pcl_filter_components_filter_chain",
                filter="RosFilterChainXYZI",
                component_class="pcl_filter_components_filter_chain::RosFilterChainXYZIComponent",
                kind="filter_chain",
                chain_data_type="pcl::PointCloud<pcl::PointXYZI>",
                chain_param_prefix="filters",
            ),
        ],
        filter_plugins=[
            types.SimpleNamespace(
                name="pcl_filters/VoxelGridXYZI",
                type="pcl_filters::VoxelGridXYZI",
                base_class_type="filters::FilterBase<pcl::PointCloud<pcl::PointXYZI>>",
                description="",
            ),
            types.SimpleNamespace(
                name="pcl_filters/MultiChannelXYZI",
                type="pcl_filters::MultiChannelXYZI",
                base_class_type="filters::MultiChannelFilterBase<pcl::PointCloud<pcl::PointXYZI>>",
                description="",
            ),
            types.SimpleNamespace(
                name="pcl_filters/VoxelGridXYZ",
                type="pcl_filters::VoxelGridXYZ",
                base_class_type="filters::FilterBase<pcl::PointCloud<pcl::PointXYZ>>",
                description="",
            ),
        ],
    )
    editor.parameter_defaults_by_component = {}
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


def chain_node(parameters: dict[str, object] | None = None) -> Node:
    return Node(
        id="chain",
        name="chain",
        type="filter",
        package="pcl_filter_components_filter_chain",
        filter="RosFilterChainXYZI",
        component_class="pcl_filter_components_filter_chain::RosFilterChainXYZIComponent",
        input_type="PointXYZI",
        output_type="PointXYZI",
        parameters=parameters or {},
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
    editor._add_visual_edge_item = lambda edge, source_item, target_item, graph_edges, collapsed_topic_id: rendered.append(
        (edge, source_item.node.id, target_item.node.id, graph_edges, collapsed_topic_id)
    )
    editor._add_edge_item = lambda edge, source_item, target_item: None

    editor._rebuild_collapsed_topic_edges()

    assert len(rendered) == 1
    visual_edge, source_id, target_id, graph_edges, collapsed_topic_id = rendered[0]
    assert source_id == "source"
    assert target_id == "target"
    assert visual_edge.source.node == "source"
    assert visual_edge.source.port == "cloud"
    assert visual_edge.target.node == "target"
    assert visual_edge.target.port == "cloud"
    assert graph_edges == [graph.edges[1]]
    assert collapsed_topic_id == "/between"


def test_orphan_collapsed_topic_publisher_is_removed_after_last_subscriber() -> None:
    source = filter_node("source", output_ports="cloud:PointXYZI")
    topic_node = topic("/between", "PointXYZI")
    target = filter_node("target", input_ports="cloud:PointXYZI")
    publisher = Edge(output("source", "cloud"), input_("/between", "in"))
    subscriber = Edge(output("/between", "out"), input_("target", "cloud"))
    graph = Graph(nodes=[source, topic_node, target], edges=[publisher, subscriber])
    editor = editor_for(graph)
    editor.graph.edges = [publisher]

    editor._remove_orphan_collapsed_topic_publishers({"/between"})

    assert editor.graph.edges == []


def test_collapsed_topic_publisher_stays_while_other_subscribers_remain() -> None:
    source = filter_node("source", output_ports="cloud:PointXYZI")
    topic_node = topic("/between", "PointXYZI")
    target = filter_node("target", input_ports="cloud:PointXYZI")
    other_target = filter_node("other_target", input_ports="cloud:PointXYZI")
    publisher = Edge(output("source", "cloud"), input_("/between", "in"))
    remaining_subscriber = Edge(output("/between", "out"), input_("other_target", "cloud"))
    graph = Graph(
        nodes=[source, topic_node, target, other_target],
        edges=[publisher, remaining_subscriber],
    )
    editor = editor_for(graph)

    editor._remove_orphan_collapsed_topic_publishers({"/between"})

    assert editor.graph.edges == [publisher, remaining_subscriber]


def test_filter_chain_metadata_detects_chain_nodes() -> None:
    node = chain_node()
    editor = chain_editor_for(Graph(nodes=[node]))

    assert editor._is_filter_chain(node) is True
    assert editor._chain_param_prefix(node) == "filters"
    assert editor._chain_data_type(node) == "pcl::PointCloud<pcl::PointXYZI>"
    assert editor._is_filter_chain(filter_node("voxel")) is False


def test_compatible_chain_plugins_match_exact_filter_base_type() -> None:
    node = chain_node()
    editor = chain_editor_for(Graph(nodes=[node]))

    plugins = editor._compatible_chain_plugins(node)

    assert [plugin.name for plugin in plugins] == ["pcl_filters/VoxelGridXYZI"]


def test_chain_parameter_rewrite_compacts_indices_and_removes_stale_entries() -> None:
    node = chain_node(
        {
            "filters.filter1.name": "old",
            "filters.filter1.type": "old_pkg/Old",
            "filters.filter1.params.keep": 1,
            "filters.filter3.name": "stale",
            "filters.filter3.type": "old_pkg/Stale",
            "filter.leaf_size": 0.1,
        }
    )
    editor = chain_editor_for(Graph(nodes=[node]))

    editor._rewrite_chain_parameters(
        node,
        [
            {
                "name": "second",
                "type": "pkg/Second",
                "params": {"threshold": 2},
            },
            {
                "name": "first",
                "type": "pkg/First",
                "params": {"enabled": True},
            },
        ],
    )

    assert node.parameters == {
        "filter.leaf_size": 0.1,
        "filters.filter1.name": "second",
        "filters.filter1.type": "pkg/Second",
        "filters.filter1.params.threshold": 2,
        "filters.filter2.name": "first",
        "filters.filter2.type": "pkg/First",
        "filters.filter2.params.enabled": True,
    }


def test_chain_entries_round_trip_plugin_params() -> None:
    node = chain_node(
        {
            "filters.filter2.name": "second",
            "filters.filter2.type": "pkg/Second",
            "filters.filter2.params.threshold": 2,
            "filters.filter1.name": "first",
            "filters.filter1.type": "pkg/First",
            "filters.filter1.params.enabled": True,
        }
    )
    editor = chain_editor_for(Graph(nodes=[node]))

    assert editor._chain_entries(node) == [
        {"name": "first", "type": "pkg/First", "params": {"enabled": True}},
        {"name": "second", "type": "pkg/Second", "params": {"threshold": 2}},
    ]


def test_filter_chain_sanitize_preserves_dynamic_plugin_parameters() -> None:
    node = chain_node(
        {
            "filters.filter1.name": "first",
            "filters.filter1.type": "pkg/First",
            "filters.filter1.params.threshold": 2,
            "in_place": True,
            "filter_component_only": "drop",
        }
    )
    editor = chain_editor_for(Graph(nodes=[node]))
    editor.parameter_defaults_by_component = {
        "pcl_filter_components_filter_chain::RosFilterChainXYZIComponent": {"in_place": False}
    }

    editor._sanitize_filter_parameters(node)

    assert node.parameters == {
        "filters.filter1.name": "first",
        "filters.filter1.type": "pkg/First",
        "filters.filter1.params.threshold": 2,
        "in_place": True,
    }


def test_filter_chain_live_spec_reloads_on_parameter_change() -> None:
    node = chain_node(
        {
            "filters.filter1.name": "first",
            "filters.filter1.type": "pkg/First",
            "in_place": True,
        }
    )
    editor = chain_editor_for(Graph(nodes=[node]))
    editor.parameter_defaults_by_component = {
        "pcl_filter_components_filter_chain::RosFilterChainXYZIComponent": {"in_place": False}
    }

    specs = editor._live_component_specs()

    assert specs["chain"]["reload_on_parameter_change"] is True
    assert specs["chain"]["parameters"]["filters.filter1.name"] == "first"
    assert specs["chain"]["parameters"]["filters.filter1.type"] == "pkg/First"
    assert specs["chain"]["parameters"]["in_place"] is True
