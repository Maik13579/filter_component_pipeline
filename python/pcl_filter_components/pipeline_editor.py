# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

import json

from python_qt_binding.QtCore import QRectF, Qt
from python_qt_binding.QtGui import QColor, QPainter, QPen
from python_qt_binding.QtWidgets import (
    QDialog,
    QDialogButtonBox,
    QFileDialog,
    QFormLayout,
    QGraphicsEllipseItem,
    QGraphicsItem,
    QGraphicsLineItem,
    QGraphicsRectItem,
    QGraphicsScene,
    QGraphicsSimpleTextItem,
    QGraphicsView,
    QHBoxLayout,
    QInputDialog,
    QLabel,
    QListWidget,
    QMessageBox,
    QPushButton,
    QMenu,
    QTextEdit,
    QVBoxLayout,
    QWidget,
)
from qt_gui.plugin import Plugin

from pcl_filter_components.filter_discovery import FilterExport, discover_filters
from pcl_filter_components.pipeline_graph import Edge, Graph, Node, PortRef, load_graph, save_graph


class EdgeItem(QGraphicsLineItem):
    def __init__(self, source: "NodeItem", target: "NodeItem") -> None:
        super().__init__()
        self.source = source
        self.target = target
        self.setZValue(-1)
        self.setPen(QPen(source.editor.theme_color("highlight"), 2))
        self.refresh()

    def refresh(self) -> None:
        self.setLine(
            self.source.sceneBoundingRect().right(),
            self.source.sceneBoundingRect().center().y(),
            self.target.sceneBoundingRect().left(),
            self.target.sceneBoundingRect().center().y(),
        )


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
        self.input_port = QGraphicsEllipseItem(-6, 28, 12, 12, self)
        self.output_port = QGraphicsEllipseItem(184, 28, 12, 12, self)
        self.input_port.setBrush(self.editor.theme_color("text"))
        self.output_port.setBrush(self.editor.theme_color("text"))

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

    def _port_label(self) -> str:
        if self.node.type == "input":
            return f"out: {self.node.output_type or '?'}"
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
        while item is not None and not isinstance(item, NodeItem):
            item = item.parentItem()
        if isinstance(item, NodeItem):
            item.setSelected(True)
            self.editor.edit_node(item)
            return
        super().mouseDoubleClickEvent(event)

    def mousePressEvent(self, event) -> None:
        if event.button() == Qt.LeftButton and self.editor.handle_port_click(self.itemAt(event.pos())):
            return
        super().mousePressEvent(event)

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

        self.widget = QWidget()
        layout = QHBoxLayout(self.widget)
        side = QVBoxLayout()
        layout.addLayout(side, 0)

        side.addWidget(QLabel("Filters"))
        self.filter_list = QListWidget()
        for export in self.discovery.filters:
            self.filter_list.addItem(
                f"{export.package}/{export.filter} [{export.input_type} -> {export.output_type}]"
            )
        side.addWidget(self.filter_list)

        self.status = QLabel("Double-click a filter to add it. Click output dot, then input dot to connect.")
        self.status.setWordWrap(True)
        side.addWidget(self.status)

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
        layout.addWidget(self.view, 1)

        if add_input is not None:
            add_input.clicked.connect(self._add_input)
        if add_output is not None:
            add_output.clicked.connect(self._add_output)
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
        if node_type == "input":
            return base.lighter(112) if base.lightness() < 128 else base.darker(104)
        if node_type == "output":
            return base.darker(112) if base.lightness() < 128 else base.lighter(104)
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
        if row < 0 or row >= len(self.discovery.filters):
            return None
        return self.discovery.filters[row]

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
                parameters={},
                qos={"reliability": "best_effort", "history": "keep_last", "depth": 5},
                position={"x": x, "y": y},
            )
        )

    def _selected_node_items(self) -> list[NodeItem]:
        return [item for item in self.scene.selectedItems() if isinstance(item, NodeItem)]

    def _edge_type(self, node: Node, outgoing: bool) -> str:
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
        self.graph.edges.append(Edge(PortRef(source.node.id, "out"), PortRef(target.node.id, "in")))
        self._add_edge_item(source, target)
        self.status.setText(f"Connected {source.node.id} -> {target.node.id}")

    def handle_port_click(self, clicked_item) -> bool:
        if clicked_item is None:
            return False
        node_item = clicked_item.parentItem()
        if not isinstance(node_item, NodeItem):
            return False
        if clicked_item == node_item.output_port:
            if node_item.node.type == "output":
                self.status.setText("Output nodes cannot start connections.")
                return True
            self.connection_source = node_item
            node_item.setSelected(True)
            self.status.setText(f"Connection start: {node_item.node.id}. Click a compatible input dot.")
            return True
        if clicked_item == node_item.input_port:
            if self.connection_source is None:
                self.status.setText("Click an output dot first.")
                return True
            source = self.connection_source
            self.connection_source = None
            self._connect_nodes(source, node_item)
            return True
        return False

    def _add_edge_item(self, source: NodeItem, target: NodeItem) -> None:
        item = EdgeItem(source, target)
        self.scene.addItem(item)
        self.edge_items.append(item)

    def refresh_edges(self) -> None:
        for edge in self.edge_items:
            edge.refresh()

    def delete_selected(self) -> None:
        selected = self._selected_node_items()
        if not selected:
            return
        selected_ids = {item.node.id for item in selected}
        self.graph.nodes = [node for node in self.graph.nodes if node.id not in selected_ids]
        self.graph.edges = [
            edge
            for edge in self.graph.edges
            if edge.source.node not in selected_ids and edge.target.node not in selected_ids
        ]
        for item in selected:
            self.scene.removeItem(item)
            self.items_by_id.pop(item.node.id, None)
        self._rebuild_edges()

    def edit_selected(self) -> None:
        selected = self._selected_node_items()
        if len(selected) != 1:
            QMessageBox.warning(self.widget, "Edit", "Select one node.")
            return
        self.edit_node(selected[0])

    def edit_node(self, item: NodeItem) -> None:
        node = item.node
        editor = QTextEdit(self.widget)
        editor.setMinimumSize(520, 320)
        editor.setPlainText(
            json.dumps(
                {
                    "id": node.id,
                    "parameters": node.parameters,
                    "qos": node.qos,
                    "sync": node.sync,
                    "topic": node.topic,
                },
                indent=2,
            )
        )
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
            node.qos = data.get("qos", {})
            node.sync = data.get("sync", {})
            node.topic = data.get("topic", node.topic)
            self._redraw_node(item)

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

    def _save(self) -> None:
        self._sync_positions()
        path, _ = QFileDialog.getSaveFileName(self.widget, "Save Pipeline", "", "YAML (*.yaml *.yml)")
        if path:
            try:
                save_graph(self.graph, path)
            except ValueError as error:
                QMessageBox.critical(self.widget, "Invalid Graph", str(error))

    def _load(self) -> None:
        path, _ = QFileDialog.getOpenFileName(self.widget, "Load Pipeline", "", "YAML (*.yaml *.yml)")
        if not path:
            return
        self.graph = load_graph(path)
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
                self._add_edge_item(source, target)
