# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

import sys
import types

import pytest

from filter_component_editor.pipeline_graph import Edge, Graph, Node, PortRef


class DummyPlugin:
    pass


class DummyQt:
    Horizontal = 1
    DashLine = 2
    UserRole = 3


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
        "QTreeWidget",
        "QTreeWidgetItem",
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

import filter_component_editor.pipeline_editor as pipeline_editor_module
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
        "GridMap": "grid_map_msgs/msg/GridMap",
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
    def __init__(self, node: Node, _editor=None) -> None:
        self.node = node
        self.visible = True
        self._pos = types.SimpleNamespace(x=lambda: 0.0, y=lambda: 0.0)
        self.input_port = FakePort(self)
        self.output_port = FakePort(self)

    def setVisible(self, visible: bool) -> None:
        self.visible = visible

    def setPos(self, x: float, y: float) -> None:
        self._pos = types.SimpleNamespace(x=lambda: x, y=lambda: y)

    def pos(self):
        return self._pos

    def output_port_name(self, _item) -> str:
        return "out"


class FakePort:
    def __init__(self, parent) -> None:
        self._parent = parent

    def parentItem(self):
        return self._parent


class FakeToggle:
    def __init__(self) -> None:
        self.checked = None
        self.blocked = False

    def blockSignals(self, blocked: bool) -> None:
        self.blocked = blocked

    def setChecked(self, checked: bool) -> None:
        self.checked = checked


class FakeScene:
    def __init__(self) -> None:
        self.items = []
        self.cleared = False

    def clear(self) -> None:
        self.cleared = True
        self.items.clear()

    def addItem(self, item) -> None:
        self.items.append(item)


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


def grid_map_chain_node(parameters: dict[str, object] | None = None) -> Node:
    return Node(
        id="grid_map_chain",
        name="grid_map_chain",
        type="filter",
        package="grid_map_components_filter_chain",
        filter="RosFilterChainGridMap",
        component_class="grid_map_components_filter_chain::RosFilterChainGridMapComponent",
        input_type="GridMap",
        output_type="GridMap",
        parameters=parameters or {},
    )


def grid_map_chain_editor_for(graph: Graph) -> PipelineEditor:
    editor = editor_for(graph)
    editor.discovery = types.SimpleNamespace(
        filters=[
            types.SimpleNamespace(
                package="grid_map_components_filter_chain",
                filter="RosFilterChainGridMap",
                component_class="grid_map_components_filter_chain::RosFilterChainGridMapComponent",
                kind="filter_chain",
                chain_data_type="grid_map::GridMap",
                chain_param_prefix="filters",
            ),
        ],
        filter_plugins=[],
        filter_plugin_defaults={
            "gridMapFilters/ThresholdFilter": {
                "layer": "elevation",
                "lower_threshold": 0.0,
                "set_to": 0.0,
            },
        },
    )
    editor.parameter_defaults_by_component = {
        "grid_map_components_filter_chain::RosFilterChainGridMapComponent": {"in_place": False}
    }
    return editor


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


def test_filter_chain_row_texts_include_configured_filters() -> None:
    node = chain_node(
        {
            "filters.filter1.name": "voxel",
            "filters.filter1.type": "pcl_filters/VoxelGridXYZI",
            "filters.filter2.name": "pass",
            "filters.filter2.type": "pcl_filters/PassThroughXYZI",
        }
    )
    editor = chain_editor_for(Graph(nodes=[node]))
    editor.show_filters = True

    assert editor.filter_row_texts_for_node(node) == [
        "chain",
        "Type: RosFilterChainXYZI",
        "Package: pcl_filter_components_filter_chain",
        "Filters:",
        "1. voxel",
        "   pcl_filters/VoxelGridXYZI",
        "2. pass",
        "   pcl_filters/PassThroughXYZI",
    ]


def test_filter_chain_row_texts_show_empty_chain_stably() -> None:
    node = chain_node()
    editor = chain_editor_for(Graph(nodes=[node]))
    editor.show_filters = True

    assert editor.filter_row_texts_for_node(node)[-2:] == ["Filters:", "Filters: none"]


def test_filter_row_texts_skip_chain_filters_for_normal_nodes() -> None:
    node = filter_node("voxel")
    editor = chain_editor_for(Graph(nodes=[node]))
    editor.show_filters = True

    assert editor.filter_row_texts_for_node(node) == [
        "voxel",
        "Type: VoxelGridXYZI",
        "Package: pcl_filter_components_xyzi",
    ]


def test_filter_row_texts_respect_show_filters_toggle() -> None:
    node = chain_node({"filters.filter1.name": "voxel", "filters.filter1.type": "pcl_filters/VoxelGridXYZI"})
    editor = chain_editor_for(Graph(nodes=[node]))
    editor.show_filters = False

    assert "Filters:" not in editor.filter_row_texts_for_node(node)


def test_compatible_chain_plugins_match_exact_filter_base_type() -> None:
    node = chain_node()
    editor = chain_editor_for(Graph(nodes=[node]))

    plugins = editor._compatible_chain_plugins(node)

    assert [plugin.name for plugin in plugins] == ["pcl_filters/VoxelGridXYZI"]


def test_sync_positions_writes_show_filters_to_editor_state() -> None:
    node = chain_node()
    item = FakeNodeItem(node)
    item.setPos(12.0, 34.0)
    editor = chain_editor_for(Graph(nodes=[node]))
    editor.items_by_id = {node.id: item}
    editor.top_down_mode = False
    editor.show_topics = False
    editor.show_filters = False

    editor._sync_positions()

    assert node.position == {"x": 12.0, "y": 34.0}
    assert editor.graph.editor == {
        "orientation": "left_right",
        "show_topics": False,
        "show_filters": False,
    }


def test_load_reads_show_filters_and_updates_toggle(monkeypatch: pytest.MonkeyPatch) -> None:
    node = chain_node()
    graph = Graph(editor={"orientation": "left_right", "show_topics": False, "show_filters": False}, nodes=[node])
    editor = chain_editor_for(Graph())
    editor.widget = object()
    editor.scene = FakeScene()
    editor.items_by_id = {}
    editor.edge_items = []
    editor.top_down_toggle = FakeToggle()
    editor.topic_visibility_toggle = FakeToggle()
    editor.filter_visibility_toggle = FakeToggle()
    editor.expand_scene_for_item = lambda _item: None
    editor.fit_graph_view = lambda: None
    editor._sync_live_pipeline = lambda: None
    editor._rebuild_edges = lambda: None
    monkeypatch.setattr(
        pipeline_editor_module,
        "QFileDialog",
        types.SimpleNamespace(getOpenFileName=lambda *_args: ("pipeline.yaml", "")),
    )
    monkeypatch.setattr(pipeline_editor_module, "load_graph", lambda _path: graph)
    monkeypatch.setattr(pipeline_editor_module, "NodeItem", FakeNodeItem)

    editor._load()

    assert editor.top_down_mode is False
    assert editor.show_topics is False
    assert editor.show_filters is False
    assert editor.top_down_toggle.checked is False
    assert editor.topic_visibility_toggle.checked is False
    assert editor.filter_visibility_toggle.checked is False
    assert editor.items_by_id == {"chain": editor.scene.items[0]}


def test_load_defaults_missing_show_filters_to_enabled(monkeypatch: pytest.MonkeyPatch) -> None:
    graph = Graph(editor={}, nodes=[chain_node()])
    editor = chain_editor_for(Graph())
    editor.widget = object()
    editor.scene = FakeScene()
    editor.items_by_id = {}
    editor.edge_items = []
    editor.top_down_toggle = FakeToggle()
    editor.topic_visibility_toggle = FakeToggle()
    editor.filter_visibility_toggle = FakeToggle()
    editor.expand_scene_for_item = lambda _item: None
    editor.fit_graph_view = lambda: None
    editor._sync_live_pipeline = lambda: None
    editor._rebuild_edges = lambda: None
    monkeypatch.setattr(
        pipeline_editor_module,
        "QFileDialog",
        types.SimpleNamespace(getOpenFileName=lambda *_args: ("pipeline.yaml", "")),
    )
    monkeypatch.setattr(pipeline_editor_module, "load_graph", lambda _path: graph)
    monkeypatch.setattr(pipeline_editor_module, "NodeItem", FakeNodeItem)

    editor._load()

    assert editor.show_filters is True
    assert editor.filter_visibility_toggle.checked is True


def test_create_topic_from_output_port_cancels_pending_drag() -> None:
    node = filter_node("source", output_ports="cloud:PointXYZI")
    item = FakeNodeItem(node)
    topic_item = FakeNodeItem(topic("/source_out", "PointXYZI"))
    editor = editor_for(Graph(nodes=[node]))
    editor.items_by_id = {node.id: item, topic_item.node.id: topic_item}
    editor.connection_source = item
    editor.connection_source_port = "out"
    editor._clear_connection_preview = lambda: None
    editor._clear_connection_highlights = lambda: None
    editor._resolve_source_port_for_new_topic = lambda _item, _port: "cloud"
    editor._create_topic_for_port = lambda _item, _outgoing, _port: topic_item
    connections = []
    editor._connect_nodes = lambda source, target, source_port, target_port: connections.append(
        (source.node.id, target.node.id, source_port, target_port)
    )

    assert editor.create_topic_from_port(item.output_port) is True

    assert editor.connection_source is None
    assert editor.connection_source_port == "out"
    assert connections == [("source", "/source_out", "cloud", "in")]


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


def test_unique_chain_filter_name_suffixes_duplicate_plugins() -> None:
    editor = chain_editor_for(Graph(nodes=[chain_node()]))
    entries = [
        {"name": "VoxelGridXYZI", "type": "pkg/VoxelGridXYZI", "params": {}},
        {"name": "VoxelGridXYZI_2", "type": "pkg/VoxelGridXYZI", "params": {}},
    ]

    assert editor._unique_chain_filter_name("pkg/VoxelGridXYZI", entries) == "VoxelGridXYZI_3"
    assert editor._unique_chain_filter_name("pkg/PassThroughXYZI", entries) == "PassThroughXYZI"


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


def test_grid_map_chain_filter_defaults_are_seeded_from_discovery() -> None:
    editor = grid_map_chain_editor_for(Graph())

    assert editor._default_chain_filter_params("gridMapFilters/ThresholdFilter") == {
        "layer": "elevation",
        "lower_threshold": 0.0,
        "set_to": 0.0,
    }


def test_grid_map_filter_chain_live_spec_configures_with_seeded_plugin_params() -> None:
    node = grid_map_chain_node(
        {
            "filters.filter1.name": "threshold",
            "filters.filter1.type": "gridMapFilters/ThresholdFilter",
            "filters.filter1.params.layer": "elevation",
            "filters.filter1.params.lower_threshold": 0.0,
            "filters.filter1.params.set_to": 0.0,
        }
    )
    editor = grid_map_chain_editor_for(Graph(nodes=[node]))

    specs = editor._live_component_specs()

    assert "configure" not in specs["grid_map_chain"]
    assert specs["grid_map_chain"]["reload_on_parameter_change"] is True
    assert specs["grid_map_chain"]["parameters"]["filters.filter1.params.layer"] == "elevation"


def test_grid_map_filter_chain_live_spec_uses_normal_configure_path() -> None:
    node = grid_map_chain_node(
        {
            "filters.filter1.name": "threshold",
            "filters.filter1.type": "gridMapFilters/ThresholdFilter",
        }
    )
    editor = grid_map_chain_editor_for(Graph(nodes=[node]))

    specs = editor._live_component_specs()

    assert "configure" not in specs["grid_map_chain"]
    assert specs["grid_map_chain"]["reload_on_parameter_change"] is True
