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
    QComboBox,
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
    QTabWidget,
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
        self.setBrush(QBrush(edge_item.source.editor.accent_color("selected")))
        self.setPen(QPen(edge_item.source.editor.accent_color("selected"), 1))
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
        self.setPen(QPen(source.editor.accent_color("default"), 2))
        self.arrow = QGraphicsPolygonItem(self)
        self.arrow.setBrush(QBrush(source.editor.accent_color("default")))
        self.arrow.setPen(QPen(source.editor.accent_color("default"), 1))
        self.refresh()

    def paint(self, painter, option, widget=None) -> None:
        width = 4 if self.isSelected() else 2
        color = self.source.editor.accent_color("selected" if self.isSelected() else "default")
        self.setPen(QPen(color, width))
        self.arrow.setBrush(QBrush(color))
        self.arrow.setPen(QPen(color, 1))
        super().paint(painter, option, widget)

    def refresh(self) -> None:
        source = self.source.output_anchor(self.edge.source.port)
        target = self.target.input_anchor(self.edge.target.port)
        self.setLine(source.x(), source.y(), target.x(), target.y())
        self._refresh_arrow(self.arrow, source, target)

    def refresh_label(self) -> None:
        pass

    def _label_text(self) -> str:
        return self.edge.topic or f"~/{self.edge.source.node}-{self.edge.target.node}"

    def _edge_type_text(self) -> str:
        source_type = self.source.editor._edge_type(self.source.node, True, self.edge.source.port)
        target_type = self.source.editor._edge_type(self.target.node, False, self.edge.target.port)
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
        width = 280 if node.type == "topic" else 190
        super().__init__(0, 0, width, 68)
        self.node = node
        self.editor = editor
        self.setFlag(QGraphicsItem.ItemIsMovable)
        self.setFlag(QGraphicsItem.ItemIsSelectable)
        self.setFlag(QGraphicsItem.ItemSendsGeometryChanges)
        self.border_color = self.editor.theme_color("mid")
        self.setBrush(self.editor.node_fill(node.type))
        self.setPen(QPen(self.border_color, 1.5))
        title = QGraphicsSimpleTextItem(node.topic if node.type == "topic" else node.name or node.id, self)
        title.setPos(54 if node.type == "topic" else 8, 10 if node.type == "topic" else 6)
        title.setBrush(self.editor.theme_color("text"))
        type_label = node.output_type or node.input_type if node.type == "topic" else node.filter if node.type == "filter" else ""
        subtitle = QGraphicsSimpleTextItem(type_label, self)
        subtitle.setPos(54 if node.type == "topic" else 8, 34 if node.type == "topic" else 28)
        subtitle.setBrush(self.editor.theme_color("text"))
        port_label = self._port_label()
        ports = QGraphicsSimpleTextItem(port_label, self)
        ports.setPos(8, 48)
        ports.setBrush(self.editor.theme_color("text"))
        if node.type in {"filter", "topic"}:
            ports.setVisible(False)
        input_pos, output_pos = self._port_positions()
        self.input_port = QGraphicsEllipseItem(input_pos.x(), input_pos.y(), 12, 12, self)
        self.output_port = QGraphicsEllipseItem(output_pos.x(), output_pos.y(), 12, 12, self)
        self.input_port.setBrush(self.editor.theme_color("text"))
        self.output_port.setBrush(self.editor.theme_color("text"))

    def paint(self, painter, option, widget=None) -> None:
        if self.isSelected():
            self.setPen(QPen(self.editor.accent_color("selected"), 3.0))
            self.setBrush(self.editor.selected_node_fill(self.node.type))
        else:
            self.setPen(QPen(self.border_color, 1.5))
            self.setBrush(self.editor.node_fill(self.node.type))
        if self.node.type == "topic":
            painter.setRenderHint(QPainter.Antialiasing)
            painter.setPen(self.pen())
            painter.setBrush(self.brush())
            painter.drawPolygon(
                QPolygonF([
                    QPointF(28.0, 8.0),
                    QPointF(52.0, 34.0),
                    QPointF(28.0, 60.0),
                    QPointF(4.0, 34.0),
                ])
            )
            return
        super().paint(painter, option, widget)

    def itemChange(self, change, value):
        result = super().itemChange(change, value)
        if change == QGraphicsItem.ItemPositionHasChanged:
            self.editor.expand_scene_for_item(self)
            self.editor.refresh_edges()
        return result

    def input_anchor(self, port: str = "in") -> QPointF:
        return self.input_port.sceneBoundingRect().center()

    def output_anchor(self, port: str = "out") -> QPointF:
        return self.output_port.sceneBoundingRect().center()

    def _port_positions(self) -> tuple[QPointF, QPointF]:
        width = self.rect().width()
        if self.editor.top_down_mode:
            center_x = 28 if self.node.type == "topic" else width / 2.0
            if self.node.type == "topic":
                return QPointF(center_x - 6, 0), QPointF(center_x - 6, 56)
            return QPointF(center_x - 6, -6), QPointF(center_x - 6, 62)
        if self.node.type == "topic":
            return QPointF(0, 28), QPointF(40, 28)
        return QPointF(-6, 28), QPointF(width - 6, 28)

    def output_port_name(self, item) -> str:
        return "out"

    def _port_label(self) -> str:
        if self.node.type == "topic":
            return f"topic: {self.node.output_type or self.node.input_type or '?'}"
        return f"{self.node.input_type or '?'} -> {self.node.output_type or '?'}"


class PipelineView(QGraphicsView):
    def __init__(self, scene: QGraphicsScene, editor: "PipelineEditor") -> None:
        super().__init__(scene)
        self.editor = editor
        self.setRenderHint(QPainter.Antialiasing)
        self.setDragMode(QGraphicsView.RubberBandDrag)

    def mouseDoubleClickEvent(self, event) -> None:
        item = self.itemAt(event.pos())
        if self.editor.create_topic_from_port(item):
            return
        while item is not None and not isinstance(item, (NodeItem, EdgeItem, EdgeHandleItem)):
            item = item.parentItem()
        if isinstance(item, NodeItem):
            item.setSelected(True)
            self.editor.edit_node(item)
            return
        if isinstance(item, EdgeHandleItem):
            item.setSelected(True)
            self.editor.begin_edge_rewire(item.edge_item, self.mapToScene(event.pos()))
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

    def wheelEvent(self, event) -> None:
        if event.angleDelta().y() > 0:
            self.editor.zoom_canvas(1.15)
        else:
            self.editor.zoom_canvas(1.0 / 1.15)

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
        self.connection_source_port = "out"
        self.connection_preview: QGraphicsLineItem | None = None
        self.rewire_edge: EdgeItem | None = None
        self.rewire_preview: QGraphicsLineItem | None = None
        self.top_down_mode = True

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
        side.addWidget(self.filter_list, 1)

        self.status = QLabel("Double-click a filter to add it. Drag output dot to input dot to connect.")
        self.status.setWordWrap(True)
        side.addWidget(self.status)

        self.top_down_toggle = QCheckBox("Top-down ports")
        self.top_down_toggle.setChecked(self.top_down_mode)
        side.addWidget(self.top_down_toggle)

        zoom_row = QHBoxLayout()
        zoom_out = QPushButton("-")
        zoom_in = QPushButton("+")
        zoom_reset = QPushButton("Reset")
        zoom_row.addWidget(zoom_out)
        zoom_row.addWidget(zoom_reset)
        zoom_row.addWidget(zoom_in)
        side.addLayout(zoom_row)

        save = QPushButton("Save")
        load = QPushButton("Load")
        for button in (save, load):
            side.addWidget(button)

        self.scene = QGraphicsScene()
        self.scene.setSceneRect(QRectF(-2000, -1200, 4000, 2400))
        self.scene.setBackgroundBrush(self.theme_color("base"))
        self.view = PipelineView(self.scene, self)
        splitter.addWidget(side_widget)
        splitter.addWidget(self.view)
        splitter.setStretchFactor(0, 0)
        splitter.setStretchFactor(1, 1)
        splitter.setSizes([260, 900])

        self.top_down_toggle.toggled.connect(self._set_top_down_mode)
        zoom_out.clicked.connect(lambda: self.zoom_canvas(1.0 / 1.2))
        zoom_in.clicked.connect(lambda: self.zoom_canvas(1.2))
        zoom_reset.clicked.connect(self.reset_zoom)
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

    def accent_color(self, role: str) -> QColor:
        if role == "selected":
            return QColor("#ff9f1c")
        return QColor("#2d7ff9")

    def node_fill(self, node_type: str) -> QColor:
        base = self.theme_color("button")
        highlight = self.theme_color("highlight")
        if node_type == "topic":
            return highlight.lighter(112) if highlight.lightness() < 128 else highlight.darker(112)
        return base

    def selected_node_fill(self, node_type: str) -> QColor:
        fill = self.node_fill(node_type)
        highlight = self.accent_color("selected")
        return QColor(
            (fill.red() * 2 + highlight.red()) // 3,
            (fill.green() * 2 + highlight.green()) // 3,
            (fill.blue() * 2 + highlight.blue()) // 3,
        )

    def zoom_canvas(self, factor: float) -> None:
        current = self.view.transform().m11()
        next_scale = max(0.15, min(4.0, current * factor))
        factor = next_scale / current if current else 1.0
        self.view.scale(factor, factor)

    def reset_zoom(self) -> None:
        self.view.resetTransform()

    def expand_scene_for_item(self, item: QGraphicsItem) -> None:
        margin = 1200.0
        rect = self.scene.sceneRect()
        item_rect = item.sceneBoundingRect().adjusted(-margin, -margin, margin, margin)
        if not rect.contains(item_rect):
            self.scene.setSceneRect(rect.united(item_rect))

    def _new_id(self, prefix: str) -> str:
        index = 1
        existing = {node.id for node in self.graph.nodes}
        while f"{prefix}_{index}" in existing:
            index += 1
        return f"{prefix}_{index}"

    def _add_node(self, node: Node) -> None:
        if node.id in self.items_by_id:
            QMessageBox.warning(self.widget, "Duplicate Node", f"Node {node.id} already exists.")
            return
        self.graph.nodes.append(node)
        item = NodeItem(node, self)
        item.setPos(node.position["x"], node.position["y"])
        self.scene.addItem(item)
        self.items_by_id[node.id] = item
        self.expand_scene_for_item(item)

    def _set_top_down_mode(self, enabled: bool) -> None:
        self.top_down_mode = enabled
        for item in list(self.items_by_id.values()):
            self._redraw_node(item)
        self.refresh_edges()

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
        name = self._new_id(export.filter)
        x = len(self.graph.nodes) * 34.0
        y = len(self.graph.nodes) * 18.0
        self._add_node(
            Node(
                id=name,
                type="filter",
                name=name,
                package=export.package,
                filter=export.filter,
                component_class=export.component_class,
                input_type=export.input_type,
                output_type=export.output_type,
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

    def _parameter_description(self, parameter_name: str) -> str:
        descriptions = {
            "queue_size": "Subscription queue depth for incoming point cloud messages.",
            "filter.output_indices": "Publish the selected point indices in addition to the filtered cloud.",
            "filter.leaf_size_x": "Voxel leaf size along the X axis in meters.",
            "filter.leaf_size_y": "Voxel leaf size along the Y axis in meters.",
            "filter.leaf_size_z": "Voxel leaf size along the Z axis in meters.",
            "filter.field_name": "Point field used by the pass-through filter.",
            "filter.min_value": "Minimum accepted field value.",
            "filter.max_value": "Maximum accepted field value.",
            "filter.min_x": "Minimum accepted X coordinate.",
            "filter.min_y": "Minimum accepted Y coordinate.",
            "filter.min_z": "Minimum accepted Z coordinate.",
            "filter.max_x": "Maximum accepted X coordinate.",
            "filter.max_y": "Maximum accepted Y coordinate.",
            "filter.max_z": "Maximum accepted Z coordinate.",
            "filter.invert": "Invert the filter result.",
        }
        return descriptions.get(parameter_name, "")

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

    def _edge_type(self, node: Node, outgoing: bool, port: str = "") -> str:
        if node.type == "topic":
            return self._type_for_port(node.output_type or node.input_type, port, outgoing)
        return self._type_for_port(node.output_type if outgoing else node.input_type, port, outgoing)

    def _stream_types(self, value: str) -> list[str]:
        return [item.strip() for item in value.replace(";", ",").split(",") if item.strip()]

    def _port_name_for_type(self, stream_type: str, index: int, total: int, outgoing: bool) -> str:
        if total > 1 and not outgoing:
            return f"input_{index + 1}"
        if stream_type == "PointIndices":
            return "indices"
        if stream_type.startswith("Point"):
            name = stream_type[5:].lower()
            return name or ("out" if outgoing else "in")
        return stream_type.replace("/", "_").replace(":", "_").lower() or ("out" if outgoing else "in")

    def _type_for_port(self, value: str, port: str, outgoing: bool) -> str:
        types = self._stream_types(value)
        if not types:
            return ""
        if not port or port in {"in", "out"}:
            return types[0]
        for index, stream_type in enumerate(types):
            if port in {stream_type, self._port_name_for_type(stream_type, index, len(types), outgoing)}:
                return stream_type
        return types[0] if outgoing else ""

    def _port_options(self, node: Node, outgoing: bool) -> list[tuple[str, str, str]]:
        types = self._stream_types(node.output_type if outgoing else node.input_type)
        options: list[tuple[str, str, str]] = []
        for index, stream_type in enumerate(types):
            port = self._port_name_for_type(stream_type, index, len(types), outgoing)
            label = f"{port} ({stream_type})"
            options.append((port, stream_type, label))
        return options

    def _resolve_source_port(
        self,
        source: NodeItem,
        target: NodeItem,
        source_port: str,
        target_port: str,
    ) -> str | None:
        if source.node.type != "filter" or source_port != "out":
            return source_port
        options = self._port_options(source.node, True)
        if len(options) <= 1:
            return source_port
        target_type = self._edge_type(target.node, False, target_port)
        if target_type:
            matches = [port for port, stream_type, _label in options if stream_type == target_type]
            if len(matches) == 1:
                return matches[0]
            if not matches:
                return source_port
        labels = [label for _port, _stream_type, label in options]
        label, ok = QInputDialog.getItem(self.widget, "Select Output", "Output", labels, 0, False)
        if not ok:
            return None
        for port, _stream_type, option_label in options:
            if option_label == label:
                return port
        return None

    def _resolve_source_port_for_new_topic(self, source: NodeItem, source_port: str) -> str | None:
        if source.node.type != "filter" or source_port != "out":
            return source_port
        options = self._port_options(source.node, True)
        if len(options) <= 1:
            return source_port
        labels = [label for _port, _stream_type, label in options]
        label, ok = QInputDialog.getItem(self.widget, "Select Output", "Output", labels, 0, False)
        if not ok:
            return None
        for port, _stream_type, option_label in options:
            if option_label == label:
                return port
        return None

    def _resolve_target_port(self, source: NodeItem, target: NodeItem, source_port: str, target_port: str) -> str | None:
        if target.node.type != "filter" or target_port != "in":
            return target_port
        options = self._port_options(target.node, False)
        if len(options) <= 1:
            return target_port
        source_type = self._edge_type(source.node, True, source_port)
        matches = [(port, label) for port, stream_type, label in options if not source_type or stream_type == source_type]
        if len(matches) == 1:
            return matches[0][0]
        if not matches:
            return target_port
        label, ok = QInputDialog.getItem(self.widget, "Select Input", "Input", [label for _port, label in matches], 0, False)
        if not ok:
            return None
        for port, option_label in matches:
            if option_label == label:
                return port
        return None

    def _ordered_connection(self, selected: list[NodeItem]) -> tuple[NodeItem, NodeItem]:
        first, second = selected
        if first.node.type == "topic" and second.node.type == "filter":
            return first, second
        if first.node.type == "filter" and second.node.type == "topic":
            return first, second
        return first, second

    def connect_selected(self) -> None:
        selected = self._selected_node_items()
        if len(selected) != 2:
            QMessageBox.warning(self.widget, "Connect", "Select exactly two nodes.")
            return
        source, target = self._ordered_connection(selected)
        self._connect_nodes(source, target)

    def _connect_nodes(
        self,
        source: NodeItem,
        target: NodeItem,
        source_port: str = "out",
        target_port: str = "in",
    ) -> None:
        resolved_source_port = self._resolve_source_port(source, target, source_port, target_port)
        if resolved_source_port is None:
            self.status.setText("Connection canceled.")
            return
        source_port = resolved_source_port
        resolved_target_port = self._resolve_target_port(source, target, source_port, target_port)
        if resolved_target_port is None:
            self.status.setText("Connection canceled.")
            return
        target_port = resolved_target_port
        if source.node.type != "topic" and target.node.type != "topic":
            topic = self._create_topic_between(source, target, source_port, target_port)
            self._connect_nodes(source, topic, source_port, "in")
            self._connect_nodes(topic, target, "out", target_port)
            return
        if source.node.type == "topic" and target.node.type == "topic":
            QMessageBox.warning(self.widget, "Connect", "Connect through one topic node.")
            return
        if source.node.id == target.node.id:
            QMessageBox.warning(self.widget, "Connect", "Cannot connect a node to itself.")
            return
        source_type = self._edge_type(source.node, True, source_port)
        target_type = self._edge_type(target.node, False, target_port)
        if source_type and target_type and source_type != target_type:
            QMessageBox.warning(
                self.widget,
                "Type Mismatch",
                f"{source.node.id} produces {source_type}, but {target.node.id} expects {target_type}.",
            )
            return
        if source.node.type == "topic":
            source.node.output_type = source.node.output_type or target_type
            source.node.input_type = source.node.input_type or target_type
        if target.node.type == "topic":
            target.node.output_type = target.node.output_type or source_type
            target.node.input_type = target.node.input_type or source_type
        for edge in self.graph.edges:
            if (
                edge.source.node == source.node.id
                and edge.source.port == source_port
                and edge.target.node == target.node.id
                and edge.target.port == target_port
            ):
                return
        edge = Edge(
            PortRef(source.node.id, source_port),
            PortRef(target.node.id, target_port),
        )
        self.graph.edges.append(edge)
        self._add_edge_item(edge, source, target)
        self.status.setText(f"Connected {source.node.id} -> {target.node.id}")

    def _create_topic_between(
        self,
        source: NodeItem,
        target: NodeItem,
        source_port: str = "out",
        target_port: str = "in",
    ) -> NodeItem:
        topic_type = self._edge_type(source.node, True, source_port) or self._edge_type(
            target.node,
            False,
            target_port,
        )
        topic_name = self._unique_topic(
            f"~/{self._topic_name_part(source.node)}-{self._topic_name_part(target.node)}"
        )
        position = {
            "x": (float(source.pos().x()) + float(target.pos().x())) / 2.0,
            "y": (float(source.pos().y()) + float(target.pos().y())) / 2.0,
        }
        self._add_node(
            Node(
                id=topic_name,
                type="topic",
                topic=topic_name,
                input_type=topic_type,
                output_type=topic_type,
                qos={"reliability": "best_effort", "history": "keep_last", "depth": 5},
                position=position,
            )
        )
        return self.items_by_id[topic_name]

    def _create_topic_for_port(self, item: NodeItem, outgoing: bool, port: str) -> NodeItem:
        topic_type = self._edge_type(item.node, outgoing, port)
        direction = "out" if outgoing else "in"
        topic_name = self._unique_topic(f"~/{self._topic_name_part(item.node)}-{direction}")
        x = float(item.pos().x())
        y = float(item.pos().y())
        if self.top_down_mode:
            y += 120.0 if outgoing else -120.0
        else:
            x += 280.0 if outgoing else -280.0
        self._add_node(
            Node(
                id=topic_name,
                type="topic",
                topic=topic_name,
                input_type=topic_type,
                output_type=topic_type,
                qos={"reliability": "best_effort", "history": "keep_last", "depth": 5},
                position={"x": x, "y": y},
            )
        )
        return self.items_by_id[topic_name]

    def create_topic_from_port(self, clicked_item) -> bool:
        node_item = self._port_owner(clicked_item)
        if node_item is None:
            return False
        if node_item.node.type != "filter":
            return False
        if clicked_item == node_item.output_port:
            source_port = node_item.output_port_name(clicked_item)
            source_port = self._resolve_source_port_for_new_topic(node_item, source_port)
            if source_port is None:
                self.status.setText("Topic creation canceled.")
                return True
            topic = self._create_topic_for_port(node_item, True, source_port)
            self._connect_nodes(node_item, topic, source_port, "in")
            return True
        if clicked_item == node_item.input_port:
            topic = self._create_topic_for_port(node_item, False, "in")
            target_port = self._resolve_target_port(topic, node_item, "out", "in")
            if target_port is None:
                self.status.setText("Topic creation canceled.")
                return True
            self._connect_nodes(topic, node_item, "out", target_port)
            return True
        return False

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
        existing.update(self.items_by_id.keys())
        if base not in existing:
            return base
        index = 2
        while f"{base}_{index}" in existing:
            index += 1
        return f"{base}_{index}"

    def _topic_name_part(self, node: Node) -> str:
        name = node.name or node.id
        name = name.removeprefix("~/").strip("/")
        return name.replace("/", "_") or "node"

    def _topic_type_is_compatible(self, topic: str, topic_type: str, edge_to_ignore: Edge) -> bool:
        for edge in self.graph.edges:
            if edge is edge_to_ignore or edge.topic != topic:
                continue
            source = self.items_by_id.get(edge.source.node)
            target = self.items_by_id.get(edge.target.node)
            existing_type = ""
            if source is not None:
                existing_type = self._edge_type(source.node, True, edge.source.port)
            if not existing_type and target is not None:
                existing_type = self._edge_type(target.node, False, edge.target.port)
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
            self.connection_source = node_item
            self.connection_source_port = node_item.output_port_name(clicked_item)
            node_item.setSelected(True)
            self._set_connection_preview(node_item.output_anchor(self.connection_source_port), scene_pos)
            self.status.setText(f"Connection start: {node_item.node.id}. Click a compatible input dot.")
            return True
        return False

    def update_connection_drag(self, scene_pos: QPointF) -> bool:
        if self.connection_source is None or self.connection_preview is None:
            return False
        source = self.connection_source.output_anchor(self.connection_source_port)
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
        anchor = target.input_anchor(edge_item.edge.target.port) if source.node.type == "topic" else source.output_anchor(
            edge_item.edge.source.port
        )
        self._clear_rewire_preview()
        self.rewire_preview = QGraphicsLineItem()
        pen = QPen(self.accent_color("selected"), 2)
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
        anchor = target.input_anchor(self.rewire_edge.edge.target.port) if source.node.type == "topic" else source.output_anchor(
            self.rewire_edge.edge.source.port
        )
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
            expected = self._edge_type(target.node, False, edge_item.edge.target.port)
            actual = self._edge_type(topic_item.node, True)
            if expected and actual and expected != actual:
                QMessageBox.warning(self.widget, "Type Mismatch", f"{topic_item.node.id} is {actual}, expected {expected}.")
                return True
            edge_item.edge.source.node = topic_item.node.id
        else:
            expected = self._edge_type(source.node, True, edge_item.edge.source.port)
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
        source_port = self.connection_source_port
        self._clear_connection_preview()
        self.connection_source = None
        self.connection_source_port = "out"
        node_item = self._port_owner(clicked_item)
        if node_item is None or clicked_item != node_item.input_port:
            self.status.setText("Connection canceled.")
            return True
        self._connect_nodes(source, node_item, source_port, "in")
        return True

    def _set_connection_preview(self, source: QPointF, target: QPointF) -> None:
        self._clear_connection_preview()
        self.connection_preview = QGraphicsLineItem()
        pen = QPen(self.accent_color("default"), 2)
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
        source_type = self._edge_type(item.source.node, True, item.edge.source.port)
        target_type = self._edge_type(item.target.node, False, item.edge.target.port)
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
        dialog = QDialog(self.widget)
        dialog.setWindowTitle(f"Edit {node.id}")
        layout = QVBoxLayout(dialog)
        name_edit: QLineEdit | None = None
        topic_edit: QLineEdit | None = None
        parameter_widgets: dict[str, QLineEdit | QCheckBox] = {}
        qos_widgets: dict[str, QLineEdit | QComboBox] = {}
        sync_editor: QTextEdit | None = None
        if node.type == "filter":
            tabs = QTabWidget(dialog)
            general = QWidget(dialog)
            general_form = QFormLayout(general)
            name_edit = QLineEdit(node.name or node.id, dialog)
            general_form.addRow("Name", name_edit)
            general_form.addRow("Filter", self._readonly_field(f"{node.package}/{node.filter}"))
            general_form.addRow("Inputs", self._readonly_field(self._port_summary([node.input_type])))
            general_form.addRow(
                "Outputs",
                self._readonly_field(self._port_summary([node.output_type])),
            )
            tabs.addTab(general, "General")
            parameters = QWidget(dialog)
            parameters_form = QFormLayout(parameters)
            if node.parameters:
                for key in sorted(node.parameters):
                    value = node.parameters[key]
                    if isinstance(value, bool):
                        widget = QCheckBox(dialog)
                        widget.setChecked(value)
                    else:
                        widget = QLineEdit(str(value), dialog)
                    description = self._parameter_description(key)
                    if description:
                        label = QLabel(key, dialog)
                        label.setToolTip(description)
                        widget.setToolTip(description)
                        if isinstance(widget, QLineEdit):
                            widget.setPlaceholderText(description)
                    else:
                        label = QLabel(key, dialog)
                    parameter_widgets[key] = widget
                    parameters_form.addRow(label, widget)
            else:
                parameters_form.addRow(QLabel("No editable parameters.", dialog))
            tabs.addTab(parameters, "Parameters")
            if self._filter_has_multiple_inputs(node):
                sync = QWidget(dialog)
                sync_form = QFormLayout(sync)
                sync_editor = QTextEdit(dialog)
                sync_editor.setMinimumHeight(120)
                sync_editor.setPlainText(json.dumps(node.sync, indent=2))
                sync_form.addRow("Settings", sync_editor)
                tabs.addTab(sync, "Sync")
            layout.addWidget(tabs)
        else:
            form = QFormLayout(dialog)
            topic_edit = QLineEdit(node.topic, dialog)
            form.addRow("Topic", topic_edit)
            topic_type = node.output_type or node.input_type
            form.addRow("Topic type", self._readonly_field(topic_type or "unknown"))
            if node.type == "topic":
                form.addRow(QLabel("QoS", dialog))
                reliability = QComboBox(dialog)
                reliability.addItems(["", "best_effort", "reliable"])
                reliability.setCurrentText(str(node.qos.get("reliability", "")))
                qos_widgets["reliability"] = reliability
                form.addRow("reliability", reliability)
                history = QComboBox(dialog)
                history.addItems(["", "keep_last", "keep_all"])
                history.setCurrentText(str(node.qos.get("history", "")))
                qos_widgets["history"] = history
                form.addRow("history", history)
                depth = QLineEdit(str(node.qos.get("depth", "")), dialog)
                qos_widgets["depth"] = depth
                form.addRow("depth", depth)
            layout.addLayout(form)
        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel, dialog)
        buttons.accepted.connect(dialog.accept)
        buttons.rejected.connect(dialog.reject)
        layout.addWidget(buttons)
        if dialog.exec_() == QDialog.Accepted:
            if node.type == "filter" and name_edit is not None:
                if not self._rename_node(node, name_edit.text().strip()):
                    return
                node.name = node.id
                node.parameters = {
                    key: self._parameter_widget_value(widget, node.parameters[key])
                    for key, widget in parameter_widgets.items()
                }
                if sync_editor is not None:
                    try:
                        node.sync = json.loads(sync_editor.toPlainText() or "{}")
                    except json.JSONDecodeError as error:
                        QMessageBox.critical(self.widget, "Invalid Sync JSON", str(error))
                        return
            elif topic_edit is not None:
                if not self._rename_node(node, topic_edit.text().strip()):
                    return
                node.topic = node.id
                if node.type == "topic":
                    node.qos = {
                        key: self._qos_widget_value(widget)
                        for key, widget in qos_widgets.items()
                        if self._qos_widget_text(widget).strip()
                    }
            self._redraw_node(item)

    def _readonly_field(self, text: str) -> QLineEdit:
        field = QLineEdit(text, self.widget)
        field.setReadOnly(True)
        return field

    def _port_summary(self, values: list[str]) -> str:
        ports: list[str] = []
        for value in values:
            for item in value.replace(";", ",").split(","):
                item = item.strip()
                if item:
                    ports.append(item)
        return ", ".join(dict.fromkeys(ports)) or "unknown"

    def _filter_has_multiple_inputs(self, node: Node) -> bool:
        return "," in node.input_type or ";" in node.input_type

    def _rename_node(self, node: Node, new_id: str) -> bool:
        if not new_id:
            QMessageBox.critical(self.widget, "Invalid Name", "Name must not be empty.")
            return False
        if new_id == node.id:
            return True
        if new_id in self.items_by_id:
            QMessageBox.critical(self.widget, "Duplicate Node", f"Node {new_id} already exists.")
            return False
        old_id = node.id
        node.id = new_id
        self.items_by_id[new_id] = self.items_by_id.pop(old_id)
        for edge in self.graph.edges:
            if edge.source.node == old_id:
                edge.source.node = new_id
            if edge.target.node == old_id:
                edge.target.node = new_id
        return True

    def _parameter_widget_value(self, widget: QLineEdit | QCheckBox, current):
        if isinstance(widget, QCheckBox):
            return widget.isChecked()
        return self._parse_typed_scalar(widget.text(), current)

    def _qos_widget_text(self, widget: QLineEdit | QComboBox) -> str:
        if isinstance(widget, QComboBox):
            return widget.currentText()
        return widget.text()

    def _qos_widget_value(self, widget: QLineEdit | QComboBox):
        return self._parse_scalar(self._qos_widget_text(widget))

    def _parse_typed_scalar(self, text: str, current):
        if isinstance(current, bool):
            return text.strip().lower() in {"1", "true", "yes", "on"}
        if isinstance(current, int) and not isinstance(current, bool):
            return int(text)
        if isinstance(current, float):
            return float(text)
        return self._parse_scalar(text)

    def _parse_scalar(self, text: str):
        value = text.strip()
        if not value:
            return ""
        try:
            return json.loads(value)
        except json.JSONDecodeError:
            return value

    def _editable_node_data(self, node: Node) -> dict[str, object]:
        if node.type == "topic":
            data: dict[str, object] = {"node_type": node.type}
        elif node.type == "filter":
            data = {"name": node.name or node.id, "node_type": node.type}
        else:
            data = {"node_type": node.type}
        if node.type == "topic":
            data["topic"] = node.topic
            data["topic_type"] = node.output_type or node.input_type
            data["qos"] = node.qos
            return data
        data["package"] = node.package
        data["filter"] = node.filter
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
        orientation = self.graph.editor.get("orientation", "top_down")
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
            self.expand_scene_for_item(item)
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
