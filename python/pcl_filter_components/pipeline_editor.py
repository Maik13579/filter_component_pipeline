# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

import json
import math

from python_qt_binding.QtCore import QPointF, QRectF, Qt
from python_qt_binding.QtGui import QColor, QBrush, QPainter, QPen, QPolygonF
from python_qt_binding.QtWidgets import (
    QDialog,
    QDialogButtonBox,
    QCheckBox,
    QFileDialog,
    QFormLayout,
    QGraphicsEllipseItem,
    QGraphicsItem,
    QGraphicsLineItem,
    QGraphicsPolygonItem,
    QGraphicsRectItem,
    QGraphicsScene,
    QGraphicsSimpleTextItem,
    QGraphicsView,
    QHBoxLayout,
    QInputDialog,
    QLabel,
    QLineEdit,
    QListWidget,
    QMessageBox,
    QPushButton,
    QMenu,
    QTextEdit,
    QSplitter,
    QVBoxLayout,
    QWidget,
)
from qt_gui.plugin import Plugin

from pcl_filter_components.filter_discovery import FilterExport, discover_filters
from pcl_filter_components.pipeline_graph import Edge, Graph, Node, PortRef, load_graph, save_graph


class EdgeHandleItem(QGraphicsPolygonItem):
    def __init__(self, edge_item: "EdgeItem") -> None:
        size = 9.0
        super().__init__(QPolygonF([
            QPointF(0.0, -size),
            QPointF(size, 0.0),
            QPointF(0.0, size),
            QPointF(-size, 0.0),
        ]))
        self.edge_item = edge_item
        self.setFlag(QGraphicsItem.ItemIsMovable)
        self.setFlag(QGraphicsItem.ItemIsSelectable)
        self.setFlag(QGraphicsItem.ItemSendsGeometryChanges)
        self.setBrush(QBrush(edge_item.source.editor.theme_color("highlight")))
        self.setPen(QPen(edge_item.source.editor.theme_color("highlight"), 1))
        self.setZValue(2)

    def itemChange(self, change, value):
        result = super().itemChange(change, value)
        if change == QGraphicsItem.ItemPositionHasChanged:
            self.edge_item.edge.position = {"x": float(self.pos().x()), "y": float(self.pos().y())}
            self.edge_item.refresh_label()
        return result


class EdgeItem(QGraphicsLineItem):
    def __init__(self, edge: Edge, source: "NodeItem", target: "NodeItem") -> None:
        super().__init__()
        self.edge = edge
        self.source = source
        self.target = target
        self.setFlag(QGraphicsItem.ItemIsSelectable)
        self.setZValue(-1)
        self.setPen(QPen(source.editor.theme_color("highlight"), 2))
        self.arrow = QGraphicsPolygonItem(self)
        self.arrow.setBrush(QBrush(source.editor.theme_color("highlight")))
        self.arrow.setPen(QPen(source.editor.theme_color("highlight"), 1))
        self.refresh()

    def paint(self, painter, option, widget=None) -> None:
        width = 4 if self.isSelected() else 2
        self.setPen(QPen(self.source.editor.theme_color("highlight"), width))
        super().paint(painter, option, widget)

    def refresh(self) -> None:
        source = self.source.output_anchor()
        target = self.target.input_anchor()
        self.setLine(source.x(), source.y(), target.x(), target.y())
        self._refresh_arrow(self.arrow, source, target)

    def refresh_label(self) -> None:
        pass

    def _label_text(self) -> str:
        return self.edge.topic or f"/pcl_pipeline/{self.edge.source.node}_to_{self.edge.target.node}"

    def _edge_type_text(self) -> str:
        source_type = self.source.editor._edge_type(self.source.node, True)
        target_type = self.source.editor._edge_type(self.target.node, False)
        return source_type or target_type or "unknown"

    def _refresh_arrow(self, arrow: QGraphicsPolygonItem, source: QPointF, target: QPointF) -> None:
        angle = math.atan2(target.y() - source.y(), target.x() - source.x())
        size = 17.0
        back = QPointF(target.x() - math.cos(angle) * size, target.y() - math.sin(angle) * size)
        left = QPointF(
            back.x() + math.cos(angle + math.pi / 2.0) * size * 0.55,
            back.y() + math.sin(angle + math.pi / 2.0) * size * 0.55,
        )
        right = QPointF(
            back.x() + math.cos(angle - math.pi / 2.0) * size * 0.55,
            back.y() + math.sin(angle - math.pi / 2.0) * size * 0.55,
        )
        arrow.setPolygon(QPolygonF([target, left, right]))


class NodeItem(QGraphicsRectItem):
    def __init__(self, node: Node, editor: "PipelineEditor") -> None:
        super().__init__(0, 0, 190, 68)
        self.node = node
        self.editor = editor
        self.setFlag(QGraphicsItem.ItemIsMovable)
        self.setFlag(QGraphicsItem.ItemIsSelectable)
        self.setFlag(QGraphicsItem.ItemSendsGeometryChanges)
        self.border_color = self.editor.theme_color("mid")
        self.setBrush(self.editor.node_fill(node.type))
        self.setPen(QPen(self.border_color, 1.5))
        title = QGraphicsSimpleTextItem(node.id, self)
        title.setPos(8, 6)
        title.setBrush(self.editor.theme_color("text"))
        type_label = node.filter if node.type == "filter" else node.type
        subtitle = QGraphicsSimpleTextItem(type_label, self)
        subtitle.setPos(8, 28)
        subtitle.setBrush(self.editor.theme_color("text"))
        port_label = self._port_label()
        ports = QGraphicsSimpleTextItem(port_label, self)
        ports.setPos(8, 48)
        ports.setBrush(self.editor.theme_color("text"))
        input_pos, output_pos = self._port_positions()
        self.input_port = QGraphicsEllipseItem(input_pos.x(), input_pos.y(), 12, 12, self)
        self.output_port = QGraphicsEllipseItem(output_pos.x(), output_pos.y(), 12, 12, self)
        self.input_port.setBrush(self.editor.theme_color("text"))
        self.output_port.setBrush(self.editor.theme_color("text"))
        if node.type == "input":
            self.input_port.setVisible(False)
        elif node.type == "output":
            self.output_port.setVisible(False)

    def paint(self, painter, option, widget=None) -> None:
        if self.isSelected():
            self.setPen(QPen(self.editor.theme_color("highlight"), 3.0))
        else:
            self.setPen(QPen(self.border_color, 1.5))
        super().paint(painter, option, widget)

    def itemChange(self, change, value):
        result = super().itemChange(change, value)
        if change == QGraphicsItem.ItemPositionHasChanged:
            self.editor.refresh_edges()
        return result

    def input_anchor(self) -> QPointF:
        return self.input_port.sceneBoundingRect().center()

    def output_anchor(self) -> QPointF:
        return self.output_port.sceneBoundingRect().center()

    def _port_positions(self) -> tuple[QPointF, QPointF]:
        if self.editor.top_down_mode:
            return QPointF(89, -6), QPointF(89, 62)
        return QPointF(-6, 28), QPointF(184, 28)

    def _port_label(self) -> str:
        if self.node.type == "input":
            return f"out: {self.node.output_type or '?'}"
        if self.node.type == "topic":
            return f"topic: {self.node.output_type or self.node.input_type or '?'}"
        if self.node.type == "output":
            return f"in: {self.node.input_type or '?'}"
        return f"{self.node.input_type or '?'} -> {self.node.output_type or '?'}"


class PipelineView(QGraphicsView):
    def __init__(self, scene: QGraphicsScene, editor: "PipelineEditor") -> None:
        super().__init__(scene)
        self.editor = editor
        self.setRenderHint(QPainter.Antialiasing)
        self.setDragMode(QGraphicsView.RubberBandDrag)

    def mouseDoubleClickEvent(self, event) -> None:
        item = self.itemAt(event.pos())
        while item is not None and not isinstance(item, (NodeItem, EdgeItem, EdgeHandleItem)):
            item = item.parentItem()
        if isinstance(item, NodeItem):
            item.setSelected(True)
            self.editor.edit_node(item)
            return
        if isinstance(item, EdgeHandleItem):
            item.setSelected(True)
            self.editor.edit_edge(item.edge_item)
            return
        if isinstance(item, EdgeItem):
            item.setSelected(True)
            self.editor.begin_edge_rewire(item, self.mapToScene(event.pos()))
            return
        super().mouseDoubleClickEvent(event)

    def mousePressEvent(self, event) -> None:
        if event.button() == Qt.LeftButton and self.editor.begin_connection_drag(
            self.itemAt(event.pos()),
            self.mapToScene(event.pos()),
        ):
            return
        super().mousePressEvent(event)

    def mouseMoveEvent(self, event) -> None:
        if self.editor.update_connection_drag(self.mapToScene(event.pos())):
            return
        if self.editor.update_edge_rewire(self.mapToScene(event.pos())):
            return
        super().mouseMoveEvent(event)

    def mouseReleaseEvent(self, event) -> None:
        if event.button() == Qt.LeftButton and self.editor.finish_connection_drag(
            self.itemAt(event.pos()),
            self.mapToScene(event.pos()),
        ):
            return
        if event.button() == Qt.LeftButton and self.editor.finish_edge_rewire(self.itemAt(event.pos())):
            return
        super().mouseReleaseEvent(event)

    def keyPressEvent(self, event) -> None:
        if event.key() == Qt.Key_Delete:
            self.editor.delete_selected()
            return
        if event.key() == Qt.Key_C:
            self.editor.connect_selected()
            return
        super().keyPressEvent(event)

    def contextMenuEvent(self, event) -> None:
        menu = QMenu(self)
        edit_action = menu.addAction("Edit")
        connect_action = menu.addAction("Connect selected")
        delete_action = menu.addAction("Delete")
        action = menu.exec_(event.globalPos())
        if action == edit_action:
            self.editor.edit_selected()
        elif action == connect_action:
            self.editor.connect_selected()
        elif action == delete_action:
            self.editor.delete_selected()


class PipelineEditor(Plugin):
    def __init__(self, context) -> None:
        super().__init__(context)
        self.setObjectName("PipelineEditor")
        self.discovery = discover_filters()
        self.graph = Graph()
        self.items_by_id: dict[str, NodeItem] = {}
        self.edge_items: list[EdgeItem] = []
        self.connection_source: NodeItem | None = None
        self.connection_preview: QGraphicsLineItem | None = None
        self.rewire_edge: EdgeItem | None = None
        self.rewire_preview: QGraphicsLineItem | None = None
        self.top_down_mode = False

        self.widget = QWidget()
        layout = QHBoxLayout(self.widget)
        splitter = QSplitter(Qt.Horizontal)
        layout.addWidget(splitter)
        side_widget = QWidget()
        side = QVBoxLayout()
        side_widget.setLayout(side)

        side.addWidget(QLabel("Filters"))
        self.filter_search = QLineEdit()
        self.filter_search.setPlaceholderText("Search filters")
        side.addWidget(self.filter_search)
        self.filter_list = QListWidget()
        self.visible_filters: list[FilterExport] = []
        self._refresh_filter_list()
        side.addWidget(self.filter_list)

        self.status = QLabel("Double-click a filter to add it. Drag output dot to input dot to connect.")
        self.status.setWordWrap(True)
        side.addWidget(self.status)

        self.top_down_toggle = QCheckBox("Top-down ports")
        side.addWidget(self.top_down_toggle)

        add_input = QPushButton("Add Input") if self.discovery.types else None
        add_output = QPushButton("Add Output") if self.discovery.types else None
        save = QPushButton("Save")
        load = QPushButton("Load")
        for button in (add_input, add_output, save, load):
            if button is not None:
                side.addWidget(button)
        side.addStretch(1)

        self.scene = QGraphicsScene()
        self.scene.setSceneRect(QRectF(-2000, -1200, 4000, 2400))
        self.scene.setBackgroundBrush(self.theme_color("base"))
        self.view = PipelineView(self.scene, self)
        splitter.addWidget(side_widget)
        splitter.addWidget(self.view)
        splitter.setStretchFactor(0, 0)
        splitter.setStretchFactor(1, 1)
        splitter.setSizes([260, 900])

        if add_input is not None:
            add_input.clicked.connect(self._add_input)
        if add_output is not None:
            add_output.clicked.connect(self._add_output)
        self.top_down_toggle.toggled.connect(self._set_top_down_mode)
        self.filter_search.textChanged.connect(self._refresh_filter_list)
        self.filter_list.itemDoubleClicked.connect(lambda _item: self._add_filter())
        save.clicked.connect(self._save)
        load.clicked.connect(self._load)

        context.add_widget(self.widget)

    def shutdown_plugin(self) -> None:
        pass

    def theme_color(self, role: str) -> QColor:
        palette = self.widget.palette()
        roles = {
            "base": palette.Base,
            "button": palette.Button,
            "highlight": palette.Highlight,
            "mid": palette.Mid,
            "text": palette.Text,
        }
        return QColor(palette.color(roles[role]))

    def node_fill(self, node_type: str) -> QColor:
        base = self.theme_color("button")
        highlight = self.theme_color("highlight")
        if node_type == "input":
            return QColor(
                (base.red() * 3 + highlight.red()) // 4,
                (base.green() * 3 + highlight.green()) // 4,
                (base.blue() * 3 + highlight.blue()) // 4,
            )
        if node_type == "output":
            return QColor(
                (base.red() * 2 + highlight.red() * 2) // 4,
                (base.green() * 2 + highlight.green() * 2) // 4,
                (base.blue() * 2 + highlight.blue() * 2) // 4,
            )
        if node_type == "topic":
            return highlight.lighter(112) if highlight.lightness() < 128 else highlight.darker(112)
        return base

    def _new_id(self, prefix: str) -> str:
        index = 1
        existing = {node.id for node in self.graph.nodes}
        while f"{prefix}_{index}" in existing:
            index += 1
        return f"{prefix}_{index}"

    def _add_node(self, node: Node) -> None:
        self.graph.nodes.append(node)
        item = NodeItem(node, self)
        item.setPos(node.position["x"], node.position["y"])
        self.scene.addItem(item)
        self.items_by_id[node.id] = item

    def _set_top_down_mode(self, enabled: bool) -> None:
        self.top_down_mode = enabled
        for item in list(self.items_by_id.values()):
            self._redraw_node(item)
        self.refresh_edges()

    def _add_input(self) -> None:
        topic, ok = QInputDialog.getText(self.widget, "Input Topic", "Topic")
        if ok and topic:
            stream_type = self._choose_stream_type("Input Type")
            if not stream_type:
                return
            self._add_node(
                Node(
                    id=self._new_id("input"),
                    type="input",
                    topic=topic,
                    output_type=stream_type,
                )
            )

    def _add_output(self) -> None:
        topic, ok = QInputDialog.getText(self.widget, "Output Topic", "Topic")
        if ok and topic:
            stream_type = self._choose_stream_type("Output Type")
            if not stream_type:
                return
            self._add_node(
                Node(
                    id=self._new_id("output"),
                    type="output",
                    topic=topic,
                    input_type=stream_type,
                )
            )

    def _choose_stream_type(self, title: str) -> str:
        types = [item.point_type for item in self.discovery.types if item.point_type]
        if not types:
            return ""
        if len(types) == 1:
            return types[0]
        value, ok = QInputDialog.getItem(self.widget, title, "Type", types, 0, False)
        return value if ok else ""

    def _default_stream_type(self) -> str:
        for item in self.discovery.types:
            if item.point_type != "PointIndices":
                return item.point_type
        return self.discovery.types[0].point_type if self.discovery.types else ""

    def _selected_filter(self) -> FilterExport | None:
        row = self.filter_list.currentRow()
        if row < 0 or row >= len(self.visible_filters):
            return None
        return self.visible_filters[row]

    def _refresh_filter_list(self) -> None:
        query = self.filter_search.text().strip().lower() if hasattr(self, "filter_search") else ""
        self.visible_filters = [
            export
            for export in self.discovery.filters
            if not query
            or query in export.filter.lower()
            or query in export.package.lower()
            or query in export.input_type.lower()
            or query in export.output_type.lower()
        ]
        self.filter_list.clear()
        for export in self.visible_filters:
            self.filter_list.addItem(
                f"{export.package}/{export.filter} [{export.input_type} -> {export.output_type}]"
            )

    def _add_filter(self) -> None:
        export = self._selected_filter()
        if export is None:
            QMessageBox.warning(self.widget, "No Filter", "Select a filter first.")
            return
        x = len(self.graph.nodes) * 34.0
        y = len(self.graph.nodes) * 18.0
        self._add_node(
            Node(
                id=self._new_id(export.filter.lower()),
                type="filter",
                package=export.package,
                filter=export.filter,
                component_class=export.component_class,
                input_type=export.input_type,
                output_type=export.output_type,
                optional_output_type=export.optional_output_type,
                parameters=self._default_parameters(export.filter),
                position={"x": x, "y": y},
            )
        )

    def _default_parameters(self, filter_name: str) -> dict[str, object]:
        common = {"queue_size": 5, "filter.output_indices": False}
        filter_defaults: dict[str, dict[str, object]] = {
            "VoxelGridXYZI": {
                "filter.leaf_size_x": 0.05,
                "filter.leaf_size_y": 0.05,
                "filter.leaf_size_z": 0.05,
                "filter.invert": False,
            },
            "PassThroughXYZI": {
                "filter.field_name": "z",
                "filter.min_value": -1.0,
                "filter.max_value": 2.0,
                "filter.invert": False,
            },
            "CropBoxXYZI": {
                "filter.min_x": -10.0,
                "filter.min_y": -10.0,
                "filter.min_z": -2.0,
                "filter.max_x": 10.0,
                "filter.max_y": 10.0,
                "filter.max_z": 3.0,
                "filter.invert": False,
            },
        }
        return {**common, **filter_defaults.get(filter_name, {})}

    def _selected_node_items(self) -> list[NodeItem]:
        return [item for item in self.scene.selectedItems() if isinstance(item, NodeItem)]

    def _selected_edge_items(self) -> list[EdgeItem]:
        edges: list[EdgeItem] = []
        for item in self.scene.selectedItems():
            if isinstance(item, EdgeItem):
                edges.append(item)
            elif isinstance(item, EdgeHandleItem):
                edges.append(item.edge_item)
        return list(dict.fromkeys(edges))

    def _edge_type(self, node: Node, outgoing: bool) -> str:
        if node.type == "topic":
            return node.output_type or node.input_type
        if outgoing:
            return node.output_type
        return node.input_type

    def _ordered_connection(self, selected: list[NodeItem]) -> tuple[NodeItem, NodeItem]:
        first, second = selected
        if first.node.type == "output" or second.node.type == "input":
            return second, first
        return first, second

    def connect_selected(self) -> None:
        selected = self._selected_node_items()
        if len(selected) != 2:
            QMessageBox.warning(self.widget, "Connect", "Select exactly two nodes.")
            return
        source, target = self._ordered_connection(selected)
        self._connect_nodes(source, target)

    def _connect_nodes(self, source: NodeItem, target: NodeItem) -> None:
        if source.node.type == "output" or target.node.type == "input":
            QMessageBox.warning(self.widget, "Connect", "Connect from input/filter to filter/output.")
            return
        if source.node.type != "topic" and target.node.type != "topic":
            topic = self._create_topic_between(source, target)
            self._connect_nodes(source, topic)
            self._connect_nodes(topic, target)
            return
        if source.node.type == "topic" and target.node.type == "topic":
            QMessageBox.warning(self.widget, "Connect", "Connect through one topic node.")
            return
        if source.node.id == target.node.id:
            QMessageBox.warning(self.widget, "Connect", "Cannot connect a node to itself.")
            return
        source_type = self._edge_type(source.node, True)
        target_type = self._edge_type(target.node, False)
        if source_type and target_type and source_type != target_type:
            QMessageBox.warning(
                self.widget,
                "Type Mismatch",
                f"{source.node.id} produces {source_type}, but {target.node.id} expects {target_type}.",
            )
            return
        for edge in self.graph.edges:
            if edge.source.node == source.node.id and edge.target.node == target.node.id:
                return
        edge = Edge(
            PortRef(source.node.id, "out"),
            PortRef(target.node.id, "in"),
        )
        self.graph.edges.append(edge)
        self._add_edge_item(edge, source, target)
        self.status.setText(f"Connected {source.node.id} -> {target.node.id}")

    def _create_topic_between(self, source: NodeItem, target: NodeItem) -> NodeItem:
        topic_type = self._edge_type(source.node, True) or self._edge_type(target.node, False)
        topic_id = self._new_id("topic")
        topic_name = self._unique_topic(f"/pcl_pipeline/{source.node.id}_to_{target.node.id}")
        position = {
            "x": (float(source.pos().x()) + float(target.pos().x())) / 2.0,
            "y": (float(source.pos().y()) + float(target.pos().y())) / 2.0,
        }
        self._add_node(
            Node(
                id=topic_id,
                type="topic",
                topic=topic_name,
                input_type=topic_type,
                output_type=topic_type,
                qos={"reliability": "best_effort", "history": "keep_last", "depth": 5},
                position=position,
            )
        )
        return self.items_by_id[topic_id]

    def _port_owner(self, item) -> NodeItem | None:
        if item is None:
            return None
        parent = item.parentItem()
        return parent if isinstance(parent, NodeItem) else None

    def _node_item_for_graphics_item(self, item) -> NodeItem | None:
        while item is not None:
            if isinstance(item, NodeItem):
                return item
            item = item.parentItem()
        return None

    def _unique_topic(self, base: str) -> str:
        existing = {node.topic for node in self.graph.nodes if node.type == "topic" and node.topic}
        if base not in existing:
            return base
        index = 2
        while f"{base}_{index}" in existing:
            index += 1
        return f"{base}_{index}"

    def _topic_type_is_compatible(self, topic: str, topic_type: str, edge_to_ignore: Edge) -> bool:
        for edge in self.graph.edges:
            if edge is edge_to_ignore or edge.topic != topic:
                continue
            source = self.items_by_id.get(edge.source.node)
            target = self.items_by_id.get(edge.target.node)
            existing_type = ""
            if source is not None:
                existing_type = self._edge_type(source.node, True)
            if not existing_type and target is not None:
                existing_type = self._edge_type(target.node, False)
            if existing_type and topic_type and existing_type != topic_type:
                return False
        return True

    def begin_connection_drag(self, clicked_item, scene_pos: QPointF) -> bool:
        if clicked_item is None:
            return False
        node_item = self._port_owner(clicked_item)
        if node_item is None:
            return False
        if clicked_item == node_item.output_port:
            if node_item.node.type == "output":
                self.status.setText("Output nodes cannot start connections.")
                return True
            self.connection_source = node_item
            node_item.setSelected(True)
            self._set_connection_preview(node_item.output_anchor(), scene_pos)
            self.status.setText(f"Connection start: {node_item.node.id}. Click a compatible input dot.")
            return True
        return False

    def update_connection_drag(self, scene_pos: QPointF) -> bool:
        if self.connection_source is None or self.connection_preview is None:
            return False
        source = self.connection_source.output_anchor()
        self.connection_preview.setLine(source.x(), source.y(), scene_pos.x(), scene_pos.y())
        return True

    def begin_edge_rewire(self, edge_item: EdgeItem, scene_pos: QPointF) -> None:
        source = self.items_by_id.get(edge_item.edge.source.node)
        target = self.items_by_id.get(edge_item.edge.target.node)
        if source is None or target is None:
            return
        if source.node.type != "topic" and target.node.type != "topic":
            self.status.setText("Only arrows connected to a topic can be rewired.")
            return
        self.rewire_edge = edge_item
        anchor = target.input_anchor() if source.node.type == "topic" else source.output_anchor()
        self._clear_rewire_preview()
        self.rewire_preview = QGraphicsLineItem()
        pen = QPen(self.theme_color("highlight"), 2)
        pen.setStyle(Qt.DashLine)
        self.rewire_preview.setPen(pen)
        self.rewire_preview.setZValue(-0.25)
        self.rewire_preview.setLine(anchor.x(), anchor.y(), scene_pos.x(), scene_pos.y())
        self.scene.addItem(self.rewire_preview)
        self.status.setText("Rewire mode: drop onto another topic.")

    def update_edge_rewire(self, scene_pos: QPointF) -> bool:
        if self.rewire_edge is None or self.rewire_preview is None:
            return False
        source = self.items_by_id.get(self.rewire_edge.edge.source.node)
        target = self.items_by_id.get(self.rewire_edge.edge.target.node)
        if source is None or target is None:
            return False
        anchor = target.input_anchor() if source.node.type == "topic" else source.output_anchor()
        self.rewire_preview.setLine(anchor.x(), anchor.y(), scene_pos.x(), scene_pos.y())
        return True

    def finish_edge_rewire(self, clicked_item) -> bool:
        if self.rewire_edge is None:
            return False
        edge_item = self.rewire_edge
        self.rewire_edge = None
        self._clear_rewire_preview()
        topic_item = self._node_item_for_graphics_item(clicked_item)
        if topic_item is None or topic_item.node.type != "topic":
            self.status.setText("Rewire canceled.")
            return True
        source = self.items_by_id.get(edge_item.edge.source.node)
        target = self.items_by_id.get(edge_item.edge.target.node)
        if source is None or target is None:
            return True
        if source.node.type == "topic":
            expected = self._edge_type(target.node, False)
            actual = self._edge_type(topic_item.node, True)
            if expected and actual and expected != actual:
                QMessageBox.warning(self.widget, "Type Mismatch", f"{topic_item.node.id} is {actual}, expected {expected}.")
                return True
            edge_item.edge.source.node = topic_item.node.id
        else:
            expected = self._edge_type(source.node, True)
            actual = self._edge_type(topic_item.node, False)
            if expected and actual and expected != actual:
                QMessageBox.warning(self.widget, "Type Mismatch", f"{topic_item.node.id} is {actual}, expected {expected}.")
                return True
            edge_item.edge.target.node = topic_item.node.id
        self._rebuild_edges()
        self.status.setText(f"Rewired to {topic_item.node.id}")
        return True

    def finish_connection_drag(self, clicked_item, scene_pos: QPointF) -> bool:
        if self.connection_source is None:
            return False
        source = self.connection_source
        self._clear_connection_preview()
        self.connection_source = None
        node_item = self._port_owner(clicked_item)
        if node_item is None or clicked_item != node_item.input_port:
            self.status.setText("Connection canceled.")
            return True
        self._connect_nodes(source, node_item)
        return True

    def _set_connection_preview(self, source: QPointF, target: QPointF) -> None:
        self._clear_connection_preview()
        self.connection_preview = QGraphicsLineItem()
        pen = QPen(self.theme_color("highlight"), 2)
        pen.setStyle(Qt.DashLine)
        self.connection_preview.setPen(pen)
        self.connection_preview.setZValue(-0.5)
        self.connection_preview.setLine(source.x(), source.y(), target.x(), target.y())
        self.scene.addItem(self.connection_preview)

    def _clear_connection_preview(self) -> None:
        if self.connection_preview is not None:
            self.scene.removeItem(self.connection_preview)
            self.connection_preview = None

    def _clear_rewire_preview(self) -> None:
        if self.rewire_preview is not None:
            self.scene.removeItem(self.rewire_preview)
            self.rewire_preview = None

    def _add_edge_item(self, edge: Edge, source: NodeItem, target: NodeItem) -> None:
        item = EdgeItem(edge, source, target)
        self.scene.addItem(item)
        self.edge_items.append(item)

    def refresh_edges(self) -> None:
        for edge in self.edge_items:
            edge.refresh()

    def delete_selected(self) -> None:
        selected_nodes = self._selected_node_items()
        selected_edges = self._selected_edge_items()
        if not selected_nodes and not selected_edges:
            return
        selected_ids = {item.node.id for item in selected_nodes}
        selected_edge_ids = {id(item.edge) for item in selected_edges}
        self.graph.nodes = [node for node in self.graph.nodes if node.id not in selected_ids]
        self.graph.edges = [
            edge
            for edge in self.graph.edges
            if id(edge) not in selected_edge_ids
            and edge.source.node not in selected_ids
            and edge.target.node not in selected_ids
        ]
        for item in selected_nodes:
            self.scene.removeItem(item)
            self.items_by_id.pop(item.node.id, None)
        self._rebuild_edges()

    def edit_selected(self) -> None:
        selected_nodes = self._selected_node_items()
        selected_edges = self._selected_edge_items()
        if len(selected_edges) == 1 and not selected_nodes:
            self.edit_edge(selected_edges[0])
            return
        if len(selected_nodes) != 1:
            QMessageBox.warning(self.widget, "Edit", "Select one node.")
            return
        self.edit_node(selected_nodes[0])

    def edit_edge(self, item: EdgeItem) -> None:
        editor = QTextEdit(self.widget)
        editor.setMinimumSize(520, 260)
        source_type = self._edge_type(item.source.node, True)
        target_type = self._edge_type(item.target.node, False)
        editor.setPlainText(
            json.dumps(
                {
                    "topic": item.edge.topic or item._label_text(),
                    "type": source_type or target_type or "unknown",
                    "qos": item.edge.qos,
                },
                indent=2,
            )
        )
        dialog = QDialog(self.widget)
        dialog.setWindowTitle(f"Edit {item.edge.source.node} -> {item.edge.target.node}")
        layout = QVBoxLayout(dialog)
        form = QFormLayout(dialog)
        form.addRow(editor)
        layout.addLayout(form)
        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel, dialog)
        buttons.accepted.connect(dialog.accept)
        buttons.rejected.connect(dialog.reject)
        layout.addWidget(buttons)
        if dialog.exec_() == QDialog.Accepted:
            try:
                data = json.loads(editor.toPlainText())
            except json.JSONDecodeError as error:
                QMessageBox.critical(self.widget, "Invalid JSON", str(error))
                return
            topic = str(data.get("topic", item.edge.topic)).strip()
            topic_type = source_type or target_type or ""
            if topic and not self._topic_type_is_compatible(topic, topic_type, item.edge):
                QMessageBox.critical(self.widget, "Duplicate Topic", f"Topic {topic} is already used.")
                return
            item.edge.topic = topic
            item.edge.qos = data.get("qos", {}) or {}
            item.refresh()

    def edit_node(self, item: NodeItem) -> None:
        node = item.node
        editor = QTextEdit(self.widget)
        editor.setMinimumSize(520, 320)
        editor.setPlainText(json.dumps(self._editable_node_data(node), indent=2))
        dialog = QDialog(self.widget)
        dialog.setWindowTitle(f"Edit {node.id}")
        layout = QVBoxLayout(dialog)
        form = QFormLayout(dialog)
        form.addRow(editor)
        layout.addLayout(form)
        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel, dialog)
        buttons.accepted.connect(dialog.accept)
        buttons.rejected.connect(dialog.reject)
        layout.addWidget(buttons)
        if dialog.exec_() == QDialog.Accepted:
            try:
                data = json.loads(editor.toPlainText())
            except json.JSONDecodeError as error:
                QMessageBox.critical(self.widget, "Invalid JSON", str(error))
                return
            new_id = str(data.get("id", node.id)).strip()
            if new_id and new_id != node.id:
                if new_id in self.items_by_id:
                    QMessageBox.critical(self.widget, "Duplicate Node", f"Node {new_id} already exists.")
                    return
                old_id = node.id
                node.id = new_id
                self.items_by_id[new_id] = self.items_by_id.pop(old_id)
                for edge in self.graph.edges:
                    if edge.source.node == old_id:
                        edge.source.node = new_id
                    if edge.target.node == old_id:
                        edge.target.node = new_id
            node.parameters = data.get("parameters", {})
            node.qos = {}
            node.sync = data.get("sync", {})
            if node.type == "input":
                node.topic = data.get("topic", node.topic)
                node.output_type = data.get("output_type", node.output_type)
            elif node.type == "topic":
                node.topic = data.get("topic", node.topic)
                topic_type = data.get("type", node.output_type or node.input_type)
                node.input_type = topic_type
                node.output_type = topic_type
                node.qos = data.get("qos", node.qos) or {}
            elif node.type == "output":
                node.topic = data.get("topic", node.topic)
                node.input_type = data.get("input_type", node.input_type)
            self._redraw_node(item)

    def _editable_node_data(self, node: Node) -> dict[str, object]:
        data: dict[str, object] = {"id": node.id, "type": node.type}
        if node.type == "input":
            data["topic"] = node.topic
            data["output_type"] = node.output_type
            return data
        if node.type == "output":
            data["topic"] = node.topic
            data["input_type"] = node.input_type
            return data
        if node.type == "topic":
            data["topic"] = node.topic
            data["type"] = node.output_type or node.input_type
            data["qos"] = node.qos
            return data
        data["input_type"] = node.input_type
        data["output_type"] = node.output_type
        if node.optional_output_type:
            data["optional_output_type"] = node.optional_output_type
        data["parameters"] = node.parameters
        data["sync"] = node.sync
        return data

    def _redraw_node(self, item: NodeItem) -> None:
        node = item.node
        position = item.pos()
        self.scene.removeItem(item)
        new_item = NodeItem(node, self)
        new_item.setPos(position)
        self.scene.addItem(new_item)
        self.items_by_id[node.id] = new_item
        self._rebuild_edges()

    def _sync_positions(self) -> None:
        for item in self.items_by_id.values():
            item.node.position = {"x": float(item.pos().x()), "y": float(item.pos().y())}
        self.graph.editor = {"orientation": "top_down" if self.top_down_mode else "left_right"}

    def _save(self) -> None:
        self._sync_positions()
        path, _ = QFileDialog.getSaveFileName(self.widget, "Save Pipeline", "", "YAML (*.yaml *.yml)")
        if path:
            if not path.endswith((".yaml", ".yml")):
                path = f"{path}.yaml"
            try:
                save_graph(self.graph, path)
            except ValueError as error:
                QMessageBox.critical(self.widget, "Invalid Graph", str(error))

    def _load(self) -> None:
        path, _ = QFileDialog.getOpenFileName(self.widget, "Load Pipeline", "", "YAML (*.yaml *.yml)")
        if not path:
            return
        self.graph = load_graph(path)
        orientation = self.graph.editor.get("orientation", "left_right")
        self.top_down_mode = orientation == "top_down"
        self.top_down_toggle.blockSignals(True)
        self.top_down_toggle.setChecked(self.top_down_mode)
        self.top_down_toggle.blockSignals(False)
        self.items_by_id.clear()
        self.edge_items.clear()
        self.scene.clear()
        for node in self.graph.nodes:
            item = NodeItem(node, self)
            item.setPos(node.position["x"], node.position["y"])
            self.scene.addItem(item)
            self.items_by_id[node.id] = item
        self._rebuild_edges()

    def _rebuild_edges(self) -> None:
        for item in self.edge_items:
            self.scene.removeItem(item)
        self.edge_items.clear()
        for edge in self.graph.edges:
            source = self.items_by_id.get(edge.source.node)
            target = self.items_by_id.get(edge.target.node)
            if source and target:
                self._add_edge_item(edge, source, target)
