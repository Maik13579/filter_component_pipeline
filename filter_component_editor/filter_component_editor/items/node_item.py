# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from python_qt_binding.QtCore import QPointF, Qt
from python_qt_binding.QtGui import QColor, QBrush, QFontMetrics, QPainter, QPen, QPolygonF
from python_qt_binding.QtWidgets import (
    QGraphicsEllipseItem,
    QGraphicsItem,
    QGraphicsRectItem,
    QGraphicsSimpleTextItem,
)

from filter_component_editor.items.port_item import PortItem
from filter_component_editor.pipeline_graph import Node


class NodeItem(QGraphicsRectItem):
    def __init__(self, node: Node, editor: "PipelineEditor") -> None:
        title_text = node.topic if node.type == "topic" else node.name or node.id
        subtitle_text = self._topic_subtitle_text(node)
        title_font = editor.widget.font()
        title_font.setBold(True)
        if node.type == "topic":
            width = 280
            height = 68
        else:
            row_texts = self._filter_row_texts_for_node(node)
            metrics = QFontMetrics(editor.widget.font())
            title_metrics = QFontMetrics(title_font)
            row_widths = [
                title_metrics.horizontalAdvance(row_texts[0]),
                *(metrics.horizontalAdvance(text) for text in row_texts[1:]),
            ]
            width = max(210, max(row_widths) + 24)
            height = 78
        super().__init__(0, 0, width, height)
        self.node = node
        self.editor = editor
        self.setFlag(QGraphicsItem.ItemIsMovable)
        self.setFlag(QGraphicsItem.ItemIsSelectable)
        self.setFlag(QGraphicsItem.ItemSendsGeometryChanges)
        self.border_color = self.editor.theme_color("mid")
        self.connection_highlight = ""
        self.setBrush(self.editor.node_fill(node.type, node))
        self.setPen(QPen(self.border_color, 1.5))
        if node.type == "topic":
            title = QGraphicsSimpleTextItem(title_text, self)
            title.setPos(54, 14)
            title.setBrush(self.editor.theme_color("text"))
            subtitle = QGraphicsSimpleTextItem(subtitle_text, self)
            subtitle.setPos(54, 38)
            subtitle.setBrush(self.editor.theme_color("text"))
        else:
            for index, text in enumerate(row_texts):
                row = QGraphicsSimpleTextItem(text, self)
                if index == 0:
                    row.setFont(title_font)
                row.setPos(12, 8 + index * 21)
                row.setBrush(self.editor.theme_color("text"))
        input_pos, output_pos = self._port_positions()
        port_diameter = self._port_diameter()
        if node.type == "topic":
            self.input_port = QGraphicsEllipseItem(0, 0, port_diameter, port_diameter, self)
            self.output_port = QGraphicsEllipseItem(0, 0, port_diameter, port_diameter, self)
        else:
            label_color = self.editor.theme_color("text")
            self.input_port = PortItem("I", port_diameter, label_color, self)
            self.output_port = PortItem("O", port_diameter, label_color, self)
        self.input_port.setPos(input_pos)
        self.output_port.setPos(output_pos)
        if node.type == "topic":
            port_brush = QBrush(self.editor.theme_color("text"))
            self.input_port.setBrush(port_brush)
            self.output_port.setBrush(port_brush)
        else:
            self.input_port.setAcceptedMouseButtons(Qt.NoButton)
            port_brush = QBrush(self._filter_port_color())
            self.input_port.setBrush(port_brush)
            self.output_port.setBrush(port_brush)
            self.input_port.setPen(QPen(self.editor.theme_color("text"), 1.5))
            self.output_port.setPen(QPen(self.editor.theme_color("text"), 1.5))
        self.update_port_visibility()

    def paint(self, painter, option, widget=None) -> None:
        if self.connection_highlight:
            pen_color, pen_width, fill = self._connection_highlight_style()
            self.setPen(QPen(pen_color, pen_width))
            self.setBrush(fill)
        elif self.isSelected():
            self.setPen(QPen(self.editor.accent_color("selected"), 3.0))
            self.setBrush(self.editor.selected_node_fill(self.node.type, self.node))
        else:
            self.setPen(QPen(self.border_color, 1.5))
            self.setBrush(self.editor.node_fill(self.node.type, self.node))
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
        painter.setRenderHint(QPainter.Antialiasing)
        rect = self.rect()
        painter.setPen(self.pen())
        painter.setBrush(self.brush())
        painter.drawRoundedRect(rect, 6.0, 6.0)

    def set_connection_highlight(self, state: str) -> None:
        if state == self.connection_highlight:
            return
        self.connection_highlight = state
        self.update()

    def _connection_highlight_style(self) -> tuple[QColor, float, QColor]:
        fill = self.editor.node_fill(self.node.type, self.node)
        if self.connection_highlight == "compatible":
            return QColor("#2e7d32"), 3.0, fill
        if self.connection_highlight == "ros_message_compatible":
            return QColor("#f57c00"), 3.0, fill
        if self.connection_highlight == "incompatible":
            return QColor("#c62828"), 3.0, fill
        if self.connection_highlight == "unavailable":
            dim = QColor(
                (fill.red() + self.editor.theme_color("base").red()) // 2,
                (fill.green() + self.editor.theme_color("base").green()) // 2,
                (fill.blue() + self.editor.theme_color("base").blue()) // 2,
            )
            return self.editor.theme_color("mid"), 1.5, dim
        return self.border_color, 1.5, fill

    def itemChange(self, change, value):
        result = super().itemChange(change, value)
        if change == QGraphicsItem.ItemPositionHasChanged:
            self.editor.expand_scene_for_item(self)
            self.editor.refresh_edges()
        return result

    def input_anchor(self, port: str = "in") -> QPointF:
        if self.node.type == "topic":
            return self.mapToScene(QPointF(28.0, 34.0))
        return self.input_port.sceneBoundingRect().center()

    def output_anchor(self, port: str = "out") -> QPointF:
        return self.output_port.sceneBoundingRect().center()

    def center_anchor(self) -> QPointF:
        if self.node.type == "topic":
            return self.mapToScene(QPointF(28.0, 34.0))
        return self.mapToScene(self.rect().center())

    def boundary_anchor_from(self, source: QPointF) -> QPointF:
        center = self.center_anchor()
        dx = source.x() - center.x()
        dy = source.y() - center.y()
        if dx == 0.0 and dy == 0.0:
            return center
        if self.node.type == "topic":
            return self._diamond_boundary_anchor(center, dx, dy)
        return self._rect_boundary_anchor(center, dx, dy)

    def _diamond_boundary_anchor(self, center: QPointF, dx: float, dy: float) -> QPointF:
        half_width = 24.0
        half_height = 26.0
        scale = 1.0 / ((abs(dx) / half_width) + (abs(dy) / half_height))
        return QPointF(center.x() + dx * scale, center.y() + dy * scale)

    def _rect_boundary_anchor(self, center: QPointF, dx: float, dy: float) -> QPointF:
        half_width = self.rect().width() / 2.0
        half_height = self.rect().height() / 2.0
        scale = 1.0 / max(abs(dx) / half_width, abs(dy) / half_height)
        return QPointF(center.x() + dx * scale, center.y() + dy * scale)

    def _port_positions(self) -> tuple[QPointF, QPointF]:
        width = self.rect().width()
        height = self.rect().height()
        radius = self._port_diameter() / 2.0
        if self.editor.top_down_mode:
            if self.node.type == "topic":
                center_x = 28
                return QPointF(center_x - radius, 0), QPointF(center_x - radius, 56)
            center_x = width / 2.0
            return QPointF(center_x - radius, -radius), QPointF(center_x - radius, height - radius)
        if self.node.type == "topic":
            return QPointF(0, 28), QPointF(40, 28)
        center_y = height / 2.0
        return QPointF(-radius * 2.0, center_y - radius), QPointF(width, center_y - radius)

    def output_port_name(self, item) -> str:
        return "out"

    def update_port_visibility(self) -> None:
        if self.node.type == "topic":
            self.input_port.setVisible(False)
            self.output_port.setVisible(True)
            return
        if self.node.type != "filter":
            self.input_port.setVisible(True)
            self.output_port.setVisible(True)
            return
        self.input_port.setVisible(bool(self.editor.available_input_ports(self.node)))
        self.output_port.setVisible(bool(self.editor.available_output_ports(self.node)))

    def _topic_subtitle_text(self, node: Node) -> str:
        return node.output_type or node.input_type or "topic"

    def _filter_row_texts_for_node(self, node: Node) -> list[str]:
        return [
            node.name or node.id or "unknown",
            f"Type: {node.filter or 'filter'}",
            f"Package: {node.package or 'unknown'}",
        ]

    def _port_diameter(self) -> float:
        return 12.0 if self.node.type == "topic" else 18.0

    def _filter_port_color(self) -> QColor:
        return self.editor.theme_color("base")
