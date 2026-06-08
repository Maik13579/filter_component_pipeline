# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

import math

from python_qt_binding.QtCore import QPointF
from python_qt_binding.QtGui import QBrush, QPen, QPolygonF
from python_qt_binding.QtWidgets import QGraphicsItem, QGraphicsLineItem, QGraphicsPolygonItem

from filter_component_editor.pipeline_graph import Edge


class EdgeItem(QGraphicsLineItem):
    def __init__(self, edge: Edge, source: "NodeItem", target: "NodeItem") -> None:
        super().__init__()
        self.edge = edge
        self.source = source
        self.target = target
        self.setFlag(QGraphicsItem.ItemIsSelectable)
        self.setZValue(-1)
        self.setPen(QPen(source.editor.edge_color(edge), 2))
        self.arrow = QGraphicsPolygonItem(self)
        self.arrow.setBrush(QBrush(source.editor.edge_color(edge)))
        self.arrow.setPen(QPen(source.editor.edge_color(edge), 1))
        self.refresh()

    def paint(self, painter, option, widget=None) -> None:
        width = 4 if self.isSelected() else 2
        color = self.source.editor.edge_color(self.edge, selected=self.isSelected())
        self.setPen(QPen(color, width))
        self.arrow.setBrush(QBrush(color))
        self.arrow.setPen(QPen(color, 1))
        super().paint(painter, option, widget)

    def refresh(self) -> None:
        source = self.source.center_anchor()
        target = self.target.center_anchor()
        tip = self.target.boundary_anchor_from(source)
        self.setLine(source.x(), source.y(), target.x(), target.y())
        self._refresh_arrow(self.arrow, source, tip)

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
