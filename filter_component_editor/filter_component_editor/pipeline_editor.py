# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

import json
from dataclasses import dataclass

from python_qt_binding.QtCore import QPointF, QRectF, Qt
from python_qt_binding.QtGui import QColor, QPen
from python_qt_binding.QtWidgets import (
    QDialog,
    QDialogButtonBox,
    QCheckBox,
    QComboBox,
    QFileDialog,
    QFormLayout,
    QFrame,
    QGraphicsItem,
    QGraphicsLineItem,
    QGraphicsScene,
    QHBoxLayout,
    QInputDialog,
    QLabel,
    QLineEdit,
    QListWidget,
    QMessageBox,
    QPushButton,
    QScrollArea,
    QTextEdit,
    QSplitter,
    QTabWidget,
    QTreeWidget,
    QTreeWidgetItem,
    QVBoxLayout,
    QWidget,
)
from qt_gui.plugin import Plugin

from filter_component_editor.filter_discovery import FilterExport, FilterPluginExport, discover_filters
from filter_component_editor.items import EdgeHandleItem, EdgeItem, NodeItem
from filter_component_editor.parameter_discovery import ComponentParameterDiscovery
from filter_component_editor.pipeline_graph import Edge, Graph, Node, PortRef, load_graph, save_graph
from filter_component_editor.runtime import LivePipelineRuntime
from filter_component_editor.views import PipelineView


ROS_MESSAGE_COMPATIBILITY = "ros_message"
ROS_MESSAGE_COMPATIBILITY_WARNING = (
    "Logical types differ. ROS message type matches, so this connection is allowed, "
    "but zero-copy will not work."
)


@dataclass(frozen=True)
class ConnectionVerdict:
    verdict: str
    reason: str = ""
    source_port: str = ""
    target_port: str = ""


class PipelineEditor(Plugin):
    def __init__(self, context) -> None:
        super().__init__(context)
        self.setObjectName("PipelineEditor")
        self.discovery = discover_filters()
        self.live_runtime = LivePipelineRuntime()
        self.parameter_discovery = ComponentParameterDiscovery(self.live_runtime)
        self.parameter_descriptions: dict[str, dict[str, str]] = {}
        self.parameter_defaults_by_component: dict[str, dict[str, object]] = {}
        self.graph = Graph()
        self.last_live_runtime_error = ""
        self.selected_logical_types = self._all_discovered_logical_types()
        self.selected_packages = self._all_discovered_packages()
        self.message_type_by_logical: dict[str, str] = {}
        self.type_adapter_by_logical: dict[str, str] = {}
        self.type_package_by_logical: dict[str, str] = {}
        for item in self.discovery.types:
            if item.point_type and item.message_type and item.point_type not in self.message_type_by_logical:
                self.message_type_by_logical[item.point_type] = item.message_type
            if item.point_type and item.type_adapter and item.point_type not in self.type_adapter_by_logical:
                self.type_adapter_by_logical[item.point_type] = item.type_adapter
            if item.point_type and item.package and item.point_type not in self.type_package_by_logical:
                self.type_package_by_logical[item.point_type] = item.package
        self.items_by_id: dict[str, NodeItem] = {}
        self.edge_items: list[EdgeItem] = []
        self.connection_source: NodeItem | None = None
        self.connection_source_port = "out"
        self.connection_preview: QGraphicsLineItem | None = None
        self.rewire_edge: EdgeItem | None = None
        self.rewire_preview: QGraphicsLineItem | None = None
        self.top_down_mode = True
        self.show_topics = True
        self.show_filters = True

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
        filter_control_row = QHBoxLayout()
        self.type_filter_button = QPushButton("Type Filter")
        self.package_filter_button = QPushButton("Package Filter")
        self.clear_filter_button = QPushButton("Clear Filter")
        filter_control_row.addWidget(self.type_filter_button)
        filter_control_row.addWidget(self.package_filter_button)
        filter_control_row.addWidget(self.clear_filter_button)
        side.addLayout(filter_control_row)
        side.addWidget(self.filter_search)
        self.filter_list = QListWidget()
        self.visible_filters: list[FilterExport] = []
        self._refresh_filter_list()
        side.addWidget(self.filter_list, 1)

        self.status = QLabel("Double-click a filter to add it. Drag output dots onto filters or topics to connect.")
        self.status.setWordWrap(True)
        side.addWidget(self.status)

        view_option_row = QHBoxLayout()
        self.top_down_toggle = QCheckBox("Top-down ports")
        self.top_down_toggle.setChecked(self.top_down_mode)
        self.topic_visibility_toggle = QCheckBox("Show topics")
        self.topic_visibility_toggle.setChecked(self.show_topics)
        self.filter_visibility_toggle = QCheckBox("Show Filters")
        self.filter_visibility_toggle.setChecked(self.show_filters)
        view_option_row.addWidget(self.top_down_toggle)
        view_option_row.addWidget(self.topic_visibility_toggle)
        view_option_row.addWidget(self.filter_visibility_toggle)
        side.addLayout(view_option_row)

        zoom_row = QHBoxLayout()
        zoom_out = QPushButton("-")
        zoom_in = QPushButton("+")
        zoom_reset = QPushButton("Reset")
        zoom_fit = QPushButton("Fit")
        zoom_row.addWidget(zoom_out)
        zoom_row.addWidget(zoom_reset)
        zoom_row.addWidget(zoom_fit)
        zoom_row.addWidget(zoom_in)
        side.addLayout(zoom_row)

        save = QPushButton("Save")
        load = QPushButton("Load")
        refresh = QPushButton("Refresh")
        save_load_row = QHBoxLayout()
        save_load_row.addWidget(save)
        save_load_row.addWidget(load)
        side.addLayout(save_load_row)
        side.addWidget(refresh)

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
        self.topic_visibility_toggle.toggled.connect(self._set_topic_visibility)
        self.filter_visibility_toggle.toggled.connect(self._set_filter_visibility)
        zoom_out.clicked.connect(lambda: self.zoom_canvas(1.0 / 1.2))
        zoom_in.clicked.connect(lambda: self.zoom_canvas(1.2))
        zoom_reset.clicked.connect(self.reset_zoom)
        zoom_fit.clicked.connect(self.fit_graph_view)
        self.filter_search.textChanged.connect(self._refresh_filter_list)
        self.type_filter_button.clicked.connect(self._edit_type_filter)
        self.package_filter_button.clicked.connect(self._edit_package_filter)
        self.clear_filter_button.clicked.connect(self._clear_filters)
        self.filter_list.itemDoubleClicked.connect(lambda _item: self._add_filter())
        save.clicked.connect(self._save)
        load.clicked.connect(self._load)
        refresh.clicked.connect(self._refresh_live_pipeline)

        context.add_widget(self.widget)

    def shutdown_plugin(self) -> None:
        self.live_runtime.stop()

    def _refresh_live_pipeline(self) -> None:
        try:
            self.live_runtime.stop()
            self.live_runtime = LivePipelineRuntime()
            self.parameter_discovery = ComponentParameterDiscovery(self.live_runtime)
            self.last_live_runtime_error = ""
            self._sync_live_pipeline()
        except Exception as error:
            message = str(error)
            self.status.setText(f"Live pipeline refresh failed: {message}")
            QMessageBox.critical(self.widget, "Live Pipeline Refresh Failed", message)

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

    def edge_color(self, edge: Edge, selected: bool = False) -> QColor:
        if edge.compatibility == ROS_MESSAGE_COMPATIBILITY:
            return QColor("#c62828" if selected else "#d32f2f")
        return self.accent_color("selected" if selected else "default")

    def node_fill(self, node_type: str, node: Node | None = None) -> QColor:
        base = self.theme_color("button")
        highlight = self.theme_color("highlight")
        if node_type == "topic":
            return highlight.lighter(112) if highlight.lightness() < 128 else highlight.darker(112)
        return base

    def selected_node_fill(self, node_type: str, node: Node | None = None) -> QColor:
        fill = self.node_fill(node_type, node)
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

    def fit_graph_view(self) -> None:
        if not self.scene.items():
            return
        margin = 80.0
        rect = self.scene.itemsBoundingRect().adjusted(-margin, -margin, margin, margin)
        if rect.isEmpty():
            return
        self.scene.setSceneRect(self.scene.sceneRect().united(rect))
        self.view.fitInView(rect, Qt.KeepAspectRatio)

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
        if node.type == "topic":
            item.setVisible(self.show_topics)
        self.expand_scene_for_item(item)
        self._sync_live_pipeline()

    def _set_top_down_mode(self, enabled: bool) -> None:
        self.top_down_mode = enabled
        for item in list(self.items_by_id.values()):
            self._redraw_node(item)
        self.refresh_edges()

    def _set_topic_visibility(self, enabled: bool) -> None:
        self.show_topics = enabled
        self._refresh_topic_visibility()
        self._rebuild_edges()

    def _set_filter_visibility(self, enabled: bool) -> None:
        self.show_filters = enabled
        for item in list(self.items_by_id.values()):
            if self._is_filter_chain(item.node):
                self._redraw_node(item)
        self._refresh_port_visibility()
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

    def _all_discovered_logical_types(self) -> set[str]:
        return {
            item.point_type
            for item in self.discovery.types
            if item.point_type
        }

    def _all_discovered_packages(self) -> set[str]:
        return {export.package for export in self.discovery.filters}

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
            if self._filter_matches_selected_types(export)
            and self._filter_matches_selected_packages(export)
            and (
                not query
                or query in export.filter.lower()
                or query in export.package.lower()
                or query in export.input_type.lower()
                or query in export.output_type.lower()
            )
        ]
        self.filter_list.clear()
        for export in self.visible_filters:
            self.filter_list.addItem(
                f"{export.package}/{export.filter} [{export.input_type} -> {export.output_type}]"
            )

    def _filter_matches_selected_types(self, export: FilterExport) -> bool:
        if not self.selected_logical_types:
            return False
        types = set(self._stream_types(export.input_type))
        types.update(self._stream_types(export.output_type))
        return bool(types.intersection(self.selected_logical_types))

    def _filter_matches_selected_packages(self, export: FilterExport) -> bool:
        return export.package in self.selected_packages

    def _set_dialog_default_size(self, dialog: QDialog, width: int = 720, height: int | None = None) -> None:
        dialog.setMinimumWidth(width)
        if height is None:
            dialog.resize(width, dialog.sizeHint().height())
            return
        dialog.resize(width, height)

    def _add_checkbox_selection_buttons(self, layout: QVBoxLayout, widgets: dict[str, QCheckBox]) -> None:
        row = QHBoxLayout()
        select_all = QPushButton("Select All")
        deselect_all = QPushButton("Deselect All")
        select_all.clicked.connect(lambda: self._set_checkboxes_checked(widgets, True))
        deselect_all.clicked.connect(lambda: self._set_checkboxes_checked(widgets, False))
        row.addWidget(select_all)
        row.addWidget(deselect_all)
        layout.addLayout(row)

    def _set_checkboxes_checked(self, widgets: dict[str, QCheckBox], checked: bool) -> None:
        for checkbox in widgets.values():
            checkbox.setChecked(checked)

    def _edit_type_filter(self) -> None:
        dialog = QDialog(self.widget)
        dialog.setWindowTitle("Type Filter")
        layout = QVBoxLayout(dialog)
        widgets: dict[str, QCheckBox] = {}
        for item in sorted(self.discovery.types, key=lambda value: (value.point_type, value.type_adapter)):
            if not item.point_type:
                continue
            label = item.point_type
            if item.type_adapter:
                label = f"{label} - {item.type_adapter}"
            checkbox = QCheckBox(label, dialog)
            checkbox.setChecked(item.point_type in self.selected_logical_types)
            widgets[item.point_type] = checkbox
            layout.addWidget(checkbox)
        self._add_checkbox_selection_buttons(layout, widgets)
        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel, dialog)
        buttons.accepted.connect(dialog.accept)
        buttons.rejected.connect(dialog.reject)
        layout.addWidget(buttons)
        self._set_dialog_default_size(dialog)
        if dialog.exec_() == QDialog.Accepted:
            self.selected_logical_types = {
                point_type for point_type, checkbox in widgets.items() if checkbox.isChecked()
            }
            self._refresh_filter_list()

    def _edit_package_filter(self) -> None:
        dialog = QDialog(self.widget)
        dialog.setWindowTitle("Package Filter")
        layout = QVBoxLayout(dialog)
        widgets: dict[str, QCheckBox] = {}
        for package in sorted(self._all_discovered_packages()):
            checkbox = QCheckBox(package, dialog)
            checkbox.setChecked(package in self.selected_packages)
            widgets[package] = checkbox
            layout.addWidget(checkbox)
        self._add_checkbox_selection_buttons(layout, widgets)
        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel, dialog)
        buttons.accepted.connect(dialog.accept)
        buttons.rejected.connect(dialog.reject)
        layout.addWidget(buttons)
        self._set_dialog_default_size(dialog)
        if dialog.exec_() == QDialog.Accepted:
            self.selected_packages = {
                package for package, checkbox in widgets.items() if checkbox.isChecked()
            }
            self._refresh_filter_list()

    def _clear_filters(self) -> None:
        self.selected_logical_types = self._all_discovered_logical_types()
        self.selected_packages = self._all_discovered_packages()
        self.filter_search.clear()
        self._refresh_filter_list()

    def _add_filter(self) -> None:
        export = self._selected_filter()
        if export is None:
            QMessageBox.warning(self.widget, "No Filter", "Select a filter first.")
            return
        name = self._prompt_new_filter_name(export.filter)
        if not name:
            return
        try:
            metadata = self._component_parameter_metadata(export)
        except Exception as error:
            QMessageBox.critical(self.widget, "Parameter Discovery Failed", str(error))
            return
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
                input_ports=export.input_ports,
                output_ports=export.output_ports,
                parameters=metadata.defaults,
                inputs=self._default_port_configs(export.input_ports or export.input_type, False),
                outputs=self._default_port_configs(export.output_ports or export.output_type, True),
                sync=self._default_sync() if self._has_multiple_input_types(export.input_ports or export.input_type) else {},
                position={"x": x, "y": y},
            )
        )

    def _prompt_new_filter_name(self, default_name: str) -> str:
        while True:
            name, ok = QInputDialog.getText(self.widget, "Filter Name", "Name", text=default_name)
            if not ok:
                return ""
            name = name.strip()
            if not name:
                QMessageBox.critical(self.widget, "Invalid Name", "Name must not be empty.")
                continue
            if name in self.items_by_id:
                QMessageBox.critical(self.widget, "Duplicate Node", f"Node {name} already exists.")
                continue
            return name

    def _default_qos(self) -> dict[str, object]:
        return {
            "reliability": "best_effort",
            "history": "keep_last",
            "depth": 5,
            "durability": "volatile",
        }

    def _default_port_configs(self, type_value: str, outgoing: bool) -> dict[str, object]:
        return {
            port: {"qos": self._default_qos()}
            for port, _stream_type, _label in self._port_options_for_type(type_value, outgoing)
        }

    def _component_parameter_metadata(self, export: FilterExport):
        metadata = self.parameter_discovery.parameters_for_component(export.package, export.component_class)
        self.parameter_descriptions[export.component_class] = metadata.descriptions
        self.parameter_defaults_by_component[self._component_cache_key(export.package, export.filter, export.component_class)] = metadata.defaults
        return metadata

    def _refresh_filter_parameters_for_dialog(self, node: Node) -> None:
        old_parameters = dict(node.parameters)
        loaded = self.live_runtime.loaded.get(node.id)
        if loaded is not None:
            metadata = self.parameter_discovery.parameters_for_loaded_node(
                self.live_runtime.node,
                loaded.full_node_name,
            )
            if self._is_filter_chain(node) and not self._metadata_has_chain_plugin_params(node, metadata.defaults):
                metadata = self.parameter_discovery.parameters_for_configured_component(
                    node.package,
                    self._component_class_for_node(node),
                    old_parameters,
                )
            node.parameters = metadata.defaults
            self._restore_filter_chain_parameters(node, old_parameters)
            self.parameter_descriptions[node.component_class] = metadata.descriptions
            self.parameter_defaults_by_component[self._component_cache_key(node.package, node.filter, node.component_class)] = metadata.defaults
            return
        export = FilterExport(
            package=node.package,
            filter=node.filter,
            component_class=node.component_class,
            input_type=node.input_type,
            output_type=node.output_type,
            input_ports=node.input_ports,
            output_ports=node.output_ports,
        )
        if self._is_filter_chain(node) and self._chain_entries(node):
            metadata = self.parameter_discovery.parameters_for_configured_component(
                node.package,
                self._component_class_for_node(node),
                old_parameters,
            )
            self.parameter_descriptions[node.component_class] = metadata.descriptions
        else:
            metadata = self._component_parameter_metadata(export)
        node.parameters = {
            key: value
            for key, value in node.parameters.items()
            if key in metadata.defaults
        }
        for key, value in metadata.defaults.items():
            node.parameters.setdefault(key, value)
        self._restore_filter_chain_parameters(node, old_parameters)

    def _metadata_has_chain_plugin_params(self, node: Node, parameters: dict[str, object]) -> bool:
        if not self._chain_entries(node):
            return True
        prefix = self._chain_param_prefix(node)
        return any(
            key.startswith(f"{prefix}.filter") and ".params." in key
            for key in parameters
        )

    def _parameter_description(self, node: Node, parameter_name: str) -> str:
        return self.parameter_descriptions.get(node.component_class, {}).get(parameter_name, "")

    def _filter_export_for_node(self, node: Node) -> FilterExport | None:
        component_class = self._component_class_for_node(node)
        for export in self.discovery.filters:
            if (
                export.package == node.package
                and export.filter == node.filter
                and export.component_class == component_class
            ):
                return export
        return None

    def _is_filter_chain(self, node: Node) -> bool:
        if node.type != "filter":
            return False
        export = self._filter_export_for_node(node)
        return export is not None and export.kind == "filter_chain"

    def _chain_param_prefix(self, node: Node) -> str:
        export = self._filter_export_for_node(node)
        if export is not None and export.chain_param_prefix:
            return export.chain_param_prefix
        return "filters"

    def _chain_data_type(self, node: Node) -> str:
        export = self._filter_export_for_node(node)
        return export.chain_data_type if export is not None else ""

    def _compatible_chain_plugins(self, node: Node) -> list[FilterPluginExport]:
        data_type = self._chain_data_type(node)
        if not data_type:
            return []
        base_class_type = f"filters::FilterBase<{data_type}>"
        plugins_by_name = {
            plugin.name: plugin
            for plugin in self.discovery.filter_plugins
            if plugin.base_class_type == base_class_type
        }
        return sorted(plugins_by_name.values(), key=lambda item: item.name)

    def _restore_filter_chain_parameters(self, node: Node, old_parameters: dict[str, object]) -> None:
        if not self._is_filter_chain(node):
            return
        prefix = self._chain_param_prefix(node)
        for key, value in old_parameters.items():
            if self._is_chain_parameter_key(key, prefix):
                node.parameters[key] = value

    def _is_chain_parameter_key(self, key: str, prefix: str) -> bool:
        if not key.startswith(f"{prefix}.filter"):
            return False
        suffix = key[len(f"{prefix}.filter"):]
        index, dot, field = suffix.partition(".")
        return bool(dot and index.isdigit() and (field in {"name", "type"} or field.startswith("params.")))

    def _chain_entries(self, node: Node) -> list[dict[str, object]]:
        prefix = self._chain_param_prefix(node)
        entries_by_index: dict[int, dict[str, object]] = {}
        for key, value in node.parameters.items():
            if not key.startswith(f"{prefix}.filter"):
                continue
            suffix = key[len(f"{prefix}.filter"):]
            index_text, dot, field = suffix.partition(".")
            if not dot or not index_text.isdigit():
                continue
            entry = entries_by_index.setdefault(int(index_text), {"name": "", "type": "", "params": {}})
            if field == "name":
                entry["name"] = str(value)
            elif field == "type":
                entry["type"] = str(value)
            elif field.startswith("params."):
                params = entry.setdefault("params", {})
                if isinstance(params, dict):
                    params[field.removeprefix("params.")] = value
        return [
            entry
            for _index, entry in sorted(entries_by_index.items())
            if entry.get("name") or entry.get("type") or entry.get("params")
        ]

    def filter_row_texts_for_node(self, node: Node) -> list[str]:
        rows = [
            node.name or node.id or "unknown",
            f"Type: {node.filter or 'filter'}",
            f"Package: {node.package or 'unknown'}",
        ]
        if not self.show_filters or not self._is_filter_chain(node):
            return rows
        rows.append("Filters:")
        entries = self._chain_entries(node)
        if not entries:
            rows.append("Filters: none")
            return rows
        for index, entry in enumerate(entries, start=1):
            name = str(entry.get("name", "")).strip()
            plugin_type = str(entry.get("type", "")).strip()
            rows.append(f"{index}. {name or plugin_type or 'unnamed'}")
            if plugin_type:
                rows.append(f"   {plugin_type}")
        return rows

    def _unique_chain_filter_name(self, plugin_name: str, entries: list[dict[str, object]]) -> str:
        base = plugin_name.rsplit("/", 1)[-1].rsplit("::", 1)[-1] or "filter"
        used = {str(entry.get("name", "")).strip() for entry in entries}
        if base not in used:
            return base
        suffix = 2
        while f"{base}_{suffix}" in used:
            suffix += 1
        return f"{base}_{suffix}"

    def _default_chain_filter_params(self, plugin_name: str) -> dict[str, object]:
        defaults = getattr(self.discovery, "filter_plugin_defaults", {})
        if isinstance(defaults, dict):
            plugin_defaults = defaults.get(plugin_name, {})
            if isinstance(plugin_defaults, dict):
                return dict(plugin_defaults)
        return {}

    def _rewrite_chain_parameters(self, node: Node, entries: list[dict[str, object]]) -> None:
        prefix = self._chain_param_prefix(node)
        preserved = {
            key: value
            for key, value in node.parameters.items()
            if not self._is_chain_parameter_key(key, prefix)
        }
        for index, entry in enumerate(entries, start=1):
            name = str(entry.get("name", "")).strip()
            plugin_type = str(entry.get("type", "")).strip()
            if not name and plugin_type:
                name = plugin_type.rsplit("/", 1)[-1].rsplit("::", 1)[-1]
            if not plugin_type and name:
                plugin_type = name
            if not name and not plugin_type:
                continue
            preserved[f"{prefix}.filter{index}.name"] = name
            preserved[f"{prefix}.filter{index}.type"] = plugin_type
            params = entry.get("params", {})
            if isinstance(params, dict):
                for param_name, value in sorted(params.items()):
                    if param_name:
                        preserved[f"{prefix}.filter{index}.params.{param_name}"] = value
        node.parameters = preserved

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
        return self._type_for_port(
            (node.output_ports or node.output_type) if outgoing else (node.input_ports or node.input_type),
            port,
            outgoing,
        )

    def _connection_compatibility(self, source_type: str, target_type: str) -> str:
        if not source_type or not target_type or source_type == target_type:
            return "exact"
        source_message = self.message_type_by_logical.get(source_type, "")
        target_message = self.message_type_by_logical.get(target_type, "")
        if source_message and source_message == target_message:
            return ROS_MESSAGE_COMPATIBILITY
        return "invalid"

    def _types_are_connectable(self, source_type: str, target_type: str) -> bool:
        return self._connection_compatibility(source_type, target_type) != "invalid"

    def _warn_ros_message_compatibility(self) -> None:
        QMessageBox.warning(self.widget, "ROS Message Compatibility", ROS_MESSAGE_COMPATIBILITY_WARNING)

    def _show_action_error(self, title: str, message: str) -> None:
        self.status.setText(message)
        QMessageBox.warning(self.widget, title, message)

    def _stream_types(self, value: str) -> list[str]:
        return [item.strip() for item in value.replace(";", ",").split(",") if item.strip()]

    def _stream_ports(self, value: str) -> list[tuple[str, str]]:
        ports: list[tuple[str, str]] = []
        for item in self._stream_types(value):
            if ":" in item:
                name, stream_type = item.split(":", 1)
                ports.append((name.strip(), stream_type.strip()))
            else:
                ports.append(("", item))
        return [(name, stream_type) for name, stream_type in ports if stream_type]

    def _port_name_for_type(self, stream_type: str, index: int, total: int, outgoing: bool) -> str:
        if total > 1 and not outgoing:
            return f"input_{index + 1}"
        if stream_type == "PointIndices":
            return "indices"
        if stream_type.startswith("Point"):
            return "cloud"
        return stream_type.replace("/", "_").replace(":", "_").lower() or ("out" if outgoing else "in")

    def _type_for_port(self, value: str, port: str, outgoing: bool) -> str:
        ports = self._stream_ports(value)
        if not ports:
            return ""
        if not port or port in {"in", "out"}:
            return ports[0][1]
        for index, (port_name, stream_type) in enumerate(ports):
            inferred_port = self._port_name_for_type(stream_type, index, len(ports), outgoing)
            if port in {stream_type, port_name, inferred_port}:
                return stream_type
        return ports[0][1] if outgoing else ""

    def _port_options(self, node: Node, outgoing: bool) -> list[tuple[str, str, str]]:
        return self._port_options_for_type(
            (node.output_ports or node.output_type) if outgoing else (node.input_ports or node.input_type),
            outgoing,
        )

    def _port_options_for_type(self, type_value: str, outgoing: bool) -> list[tuple[str, str, str]]:
        ports = self._stream_ports(type_value)
        options: list[tuple[str, str, str]] = []
        for index, (port_name, stream_type) in enumerate(ports):
            port = port_name or self._port_name_for_type(stream_type, index, len(ports), outgoing)
            label = f"{port} ({stream_type})"
            options.append((port, stream_type, label))
        return options

    def _canonical_port(self, node: Node, port: str, outgoing: bool) -> str:
        options = self._port_options(node, outgoing)
        if not options:
            return port or ("out" if outgoing else "in")
        if not port or port in {"in", "out"}:
            return options[0][0]
        valid_ports = {option_port for option_port, _stream_type, _label in options}
        return port if port in valid_ports else port

    def _canonical_input_port(self, node: Node, port: str) -> str:
        return self._canonical_port(node, port, False)

    def _canonical_output_port(self, node: Node, port: str) -> str:
        return self._canonical_port(node, port, True)

    def _occupied_input_ports(self, node: Node) -> set[str]:
        occupied: set[str] = set()
        if node.type != "filter":
            return occupied
        nodes_by_id = {graph_node.id: graph_node for graph_node in self.graph.nodes}
        for edge in self.graph.edges:
            if edge.target.node != node.id:
                continue
            source = nodes_by_id.get(edge.source.node)
            if source is not None and source.type == "topic":
                occupied.add(self._canonical_input_port(node, edge.target.port))
        return occupied

    def _occupied_output_ports(self, node: Node) -> set[str]:
        occupied: set[str] = set()
        if node.type != "filter":
            return occupied
        nodes_by_id = {graph_node.id: graph_node for graph_node in self.graph.nodes}
        for edge in self.graph.edges:
            if edge.source.node != node.id:
                continue
            target = nodes_by_id.get(edge.target.node)
            if target is not None and target.type == "topic":
                occupied.add(self._canonical_output_port(node, edge.source.port))
        return occupied

    def available_input_ports(self, node: Node) -> list[tuple[str, str, str]]:
        if node.type != "filter":
            return []
        occupied = self._occupied_input_ports(node)
        return [(port, stream_type, label) for port, stream_type, label in self._port_options(node, False) if port not in occupied]

    def available_output_ports(self, node: Node) -> list[tuple[str, str, str]]:
        if node.type != "filter":
            return []
        occupied = self._occupied_output_ports(node)
        return [(port, stream_type, label) for port, stream_type, label in self._port_options(node, True) if port not in occupied]

    def _connection_verdict(
        self,
        source: Node,
        source_port: str,
        target: Node,
        target_port: str,
    ) -> ConnectionVerdict:
        if source.id == target.id:
            return ConnectionVerdict("invalid", "Cannot connect a node to itself.", source_port, target_port)
        if source.type == "topic" and target.type == "topic":
            return ConnectionVerdict("invalid", "Connect through one topic node.", source_port, target_port)

        canonical_source_port = (
            self._canonical_output_port(source, source_port) if source.type == "filter" else source_port
        )
        canonical_target_port = (
            self._canonical_input_port(target, target_port) if target.type == "filter" else target_port
        )
        if source.type == "filter" and canonical_source_port in self._occupied_output_ports(source):
            return ConnectionVerdict(
                "unavailable",
                f"{source.id} output {canonical_source_port} is already connected.",
                canonical_source_port,
                canonical_target_port,
            )
        if target.type == "filter" and canonical_target_port in self._occupied_input_ports(target):
            return ConnectionVerdict(
                "unavailable",
                f"{target.id} input {canonical_target_port} is already connected.",
                canonical_source_port,
                canonical_target_port,
            )

        source_type = self._edge_type(source, True, canonical_source_port)
        target_type = self._edge_type(target, False, canonical_target_port)
        compatibility = self._connection_compatibility(source_type, target_type)
        if compatibility == "invalid":
            return ConnectionVerdict(
                "invalid",
                f"{source.id} produces {source_type or 'an unknown type'}, "
                f"but {target.id} expects {target_type or 'an unknown type'}.",
                canonical_source_port,
                canonical_target_port,
            )
        return ConnectionVerdict(compatibility, "", canonical_source_port, canonical_target_port)

    def _candidate_input_verdicts(self, source: Node, source_port: str, target: Node) -> list[ConnectionVerdict]:
        if target.type != "filter":
            return [self._connection_verdict(source, source_port, target, "in")]
        return [
            self._connection_verdict(source, source_port, target, port)
            for port, _stream_type, _label in self.available_input_ports(target)
        ]

    def _candidate_source_ports(self, source: Node, source_port: str) -> list[str]:
        if source.type != "filter" or source_port != "out":
            return [source_port]
        ports = [port for port, _stream_type, _label in self.available_output_ports(source)]
        return ports or [source_port]

    def _best_connection_verdict_for_target(
        self,
        source: Node,
        source_port: str,
        target: Node,
    ) -> ConnectionVerdict:
        if source.id == target.id:
            return self._connection_verdict(source, source_port, target, "in")
        candidates: list[ConnectionVerdict] = []
        for candidate_source_port in self._candidate_source_ports(source, source_port):
            if target.type == "filter":
                candidates.extend(self._candidate_input_verdicts(source, candidate_source_port, target))
            else:
                candidates.append(self._connection_verdict(source, candidate_source_port, target, "in"))
        if not candidates:
            return ConnectionVerdict("unavailable", f"{target.id} has no free inputs.", source_port, "in")
        for wanted in ("exact", ROS_MESSAGE_COMPATIBILITY, "invalid", "unavailable"):
            for verdict in candidates:
                if verdict.verdict == wanted:
                    return verdict
        return candidates[0]

    def _highlight_nodes_for_connection_drag(self) -> None:
        if self.connection_source is None:
            self._clear_connection_highlights()
            return
        source = self.connection_source.node
        source_port = self.connection_source_port
        for item in self.items_by_id.values():
            if item.node.id == source.id:
                item.set_connection_highlight("unavailable")
                continue
            verdict = self._best_connection_verdict_for_target(source, source_port, item.node)
            state = {
                "exact": "compatible",
                ROS_MESSAGE_COMPATIBILITY: "ros_message_compatible",
                "invalid": "incompatible",
                "unavailable": "unavailable",
            }.get(verdict.verdict, "")
            item.set_connection_highlight(state)

    def _clear_connection_highlights(self) -> None:
        for item in self.items_by_id.values():
            item.set_connection_highlight("")

    def _resolve_source_port(
        self,
        source: NodeItem,
        target: NodeItem,
        source_port: str,
        target_port: str,
    ) -> str | None:
        if source.node.type != "filter" or source_port != "out":
            return source_port
        options = self.available_output_ports(source.node)
        if len(options) <= 1:
            if not options:
                self._show_action_error(
                    "Output Already Connected",
                    f"All outputs on {source.node.id} are already connected.",
                )
                return None
            return options[0][0] if len(self._port_options(source.node, True)) > 1 else source_port
        target_type = self._edge_type(target.node, False, target_port)
        if target_type:
            matches = [
                port
                for port, stream_type, _label in options
                if self._types_are_connectable(stream_type, target_type)
            ]
            if len(matches) == 1:
                return matches[0]
            if not matches:
                self._show_action_error(
                    "No Compatible Output",
                    f"No compatible free output on {source.node.id}.",
                )
                return None
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
        options = self.available_output_ports(source.node)
        if len(options) <= 1:
            if not options:
                self._show_action_error(
                    "Topic Creation Failed",
                    f"No free output on {source.node.id} is available for a new topic.",
                )
                return None
            return options[0][0] if len(self._port_options(source.node, True)) > 1 else source_port
        labels = [label for _port, _stream_type, label in options]
        label, ok = QInputDialog.getItem(self.widget, "Select Output", "Output", labels, 0, False)
        if not ok:
            return None
        for port, _stream_type, option_label in options:
            if option_label == label:
                return port
        return None

    def _resolve_target_port_for_new_topic(self, target: NodeItem, target_port: str) -> str | None:
        if target.node.type != "filter" or target_port != "in":
            return target_port
        options = self.available_input_ports(target.node)
        if len(options) <= 1:
            if not options:
                self._show_action_error(
                    "Topic Creation Failed",
                    f"No free input on {target.node.id} is available for a new topic.",
                )
                return None
            return options[0][0] if len(self._port_options(target.node, False)) > 1 else target_port
        labels = [label for _port, _stream_type, label in options]
        label, ok = QInputDialog.getItem(self.widget, "Select Input", "Input", labels, 0, False)
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
        occupied = self._occupied_input_ports(target.node)
        if len(options) <= 1:
            canonical = self._canonical_input_port(target.node, target_port)
            if canonical in occupied:
                self._show_action_error(
                    "Input Already Connected",
                    f"{target.node.id} input {canonical} is already connected.",
                )
                return None
            return target_port
        source_type = self._edge_type(source.node, True, source_port)
        matches = [
            (port, label)
            for port, stream_type, label in options
            if self._types_are_connectable(source_type, stream_type) and port not in occupied
        ]
        if len(matches) == 1:
            return matches[0][0]
        if not matches:
            self._show_action_error(
                "No Compatible Input",
                f"No compatible free input on {target.node.id}.",
            )
            return None
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
            self._show_action_error("Connect", "Select exactly two nodes.")
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
        if source.node.id == target.node.id:
            self._show_action_error("Connect", "Cannot connect a node to itself.")
            return
        if source.node.type == "topic" and target.node.type == "topic":
            self._show_action_error("Connect", "Connect through one topic node.")
            return
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
        source_type = self._edge_type(source.node, True, source_port)
        target_type = self._edge_type(target.node, False, target_port)
        if source.node.type == "topic":
            source.node.output_type = source.node.output_type or target_type
            source.node.input_type = source.node.input_type or target_type
        if target.node.type == "topic":
            target.node.output_type = target.node.output_type or source_type
            target.node.input_type = target.node.input_type or source_type
        source_type = self._edge_type(source.node, True, source_port)
        target_type = self._edge_type(target.node, False, target_port)
        compatibility = self._connection_compatibility(source_type, target_type)
        if compatibility == "invalid":
            self._show_action_error(
                "Type Mismatch",
                f"{source.node.id} produces {source_type}, but {target.node.id} expects {target_type}.",
            )
            return
        source_port = self._canonical_output_port(source.node, source_port) if source.node.type == "filter" else source_port
        target_port = self._canonical_input_port(target.node, target_port) if target.node.type == "filter" else target_port
        for edge in self.graph.edges:
            if (
                edge.source.node == source.node.id
                and edge.source.port == source_port
                and edge.target.node == target.node.id
                and edge.target.port == target_port
            ):
                self._show_action_error("Duplicate Connection", "That connection already exists.")
                return
        edge = Edge(
            PortRef(source.node.id, source_port, "output"),
            PortRef(target.node.id, target_port, "input"),
            compatibility=ROS_MESSAGE_COMPATIBILITY if compatibility == ROS_MESSAGE_COMPATIBILITY else "",
        )
        self.graph.edges.append(edge)
        if self.show_topics:
            self._add_edge_item(edge, source, target)
        else:
            self._rebuild_edges()
        self._refresh_port_visibility()
        self.status.setText(f"Connected {source.node.id} -> {target.node.id}")
        if compatibility == ROS_MESSAGE_COMPATIBILITY:
            self._warn_ros_message_compatibility()
        self._sync_live_pipeline()

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
            f"/{self._topic_name_part(source.node)}_{self._topic_name_part(target.node)}"
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
                position=position,
            )
        )
        return self.items_by_id[topic_name]

    def _create_topic_for_port(self, item: NodeItem, outgoing: bool, port: str) -> NodeItem:
        topic_type = self._edge_type(item.node, outgoing, port)
        direction = "out" if outgoing else "in"
        topic_name = self._unique_topic(f"/{self._topic_name_part(item.node)}_{direction}")
        anchor = item.output_anchor(port) if outgoing else item.input_anchor(port)
        x = float(anchor.x()) - 28.0
        y = float(anchor.y()) - 34.0
        if self.top_down_mode:
            y += 112.0 if outgoing else -112.0
        else:
            x += 168.0 if outgoing else -312.0
        self._add_node(
            Node(
                id=topic_name,
                type="topic",
                topic=topic_name,
                input_type=topic_type,
                output_type=topic_type,
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
            target_port = self._resolve_target_port_for_new_topic(node_item, "in")
            if target_port is None:
                self.status.setText("Topic creation canceled.")
                return True
            topic = self._create_topic_for_port(node_item, False, target_port)
            self._connect_nodes(topic, node_item, "out", target_port)
            return True
        return False

    def _port_owner(self, item) -> NodeItem | None:
        if item is None:
            return None
        parent = item.parentItem()
        if not isinstance(parent, NodeItem):
            return None
        return parent if item in {parent.input_port, parent.output_port} else None

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
        return self._topic_name_part_for_text(name) or "node"

    def _topic_name_part_for_text(self, text: str) -> str:
        name = text.removeprefix("~/").strip("/")
        return name.replace("/", "_").replace("-", "_")

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
            if existing_type and topic_type and not self._types_are_connectable(existing_type, topic_type):
                return False
        return True

    def begin_connection_drag(self, clicked_item, scene_pos: QPointF, allow_node_body: bool = False) -> bool:
        if clicked_item is None:
            return False
        port_owner = self._port_owner(clicked_item)
        if port_owner is not None and port_owner.node.type == "filter" and clicked_item == port_owner.input_port:
            return False
        node_item = port_owner or (self._node_item_for_graphics_item(clicked_item) if allow_node_body else None)
        if node_item is None:
            return False
        if node_item.node.type == "filter" and not self.available_output_ports(node_item.node):
            self._show_action_error(
                "Output Already Connected",
                f"All outputs on {node_item.node.id} are already connected.",
            )
            return False
        self.connection_source = node_item
        self.connection_source_port = node_item.output_port_name(clicked_item)
        node_item.setSelected(True)
        self._set_connection_preview(node_item.output_anchor(self.connection_source_port), scene_pos)
        self.status.setText(f"Connection start: {node_item.node.id}. Drop on a compatible node.")
        return True

    def update_connection_drag(self, scene_pos: QPointF) -> bool:
        if self.connection_source is None or self.connection_preview is None:
            return False
        source = self.connection_source.output_anchor(self.connection_source_port)
        self.connection_preview.setLine(source.x(), source.y(), scene_pos.x(), scene_pos.y())
        self._highlight_nodes_for_connection_drag()
        return True

    def begin_edge_rewire(self, edge_item: EdgeItem, scene_pos: QPointF) -> None:
        source = self.items_by_id.get(edge_item.edge.source.node)
        target = self.items_by_id.get(edge_item.edge.target.node)
        if source is None or target is None:
            return
        if source.node.type != "topic" and target.node.type != "topic":
            self._show_action_error("Rewire", "Only arrows connected to a topic can be rewired.")
            return
        self.rewire_edge = edge_item
        self._clear_connection_highlights()
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
        self._clear_connection_highlights()
        topic_item = self._node_item_for_graphics_item(clicked_item)
        if topic_item is None or topic_item.node.type != "topic":
            self._show_action_error("Rewire", "Drop onto a compatible topic to rewire this edge.")
            return True
        source = self.items_by_id.get(edge_item.edge.source.node)
        target = self.items_by_id.get(edge_item.edge.target.node)
        if source is None or target is None:
            return True
        if source.node.type == "topic":
            expected = self._edge_type(target.node, False, edge_item.edge.target.port)
            actual = self._edge_type(topic_item.node, True)
            compatibility = self._connection_compatibility(actual, expected)
            if compatibility == "invalid":
                self._show_action_error("Type Mismatch", f"{topic_item.node.id} is {actual}, expected {expected}.")
                return True
            edge_item.edge.source.node = topic_item.node.id
        else:
            expected = self._edge_type(source.node, True, edge_item.edge.source.port)
            actual = self._edge_type(topic_item.node, False)
            compatibility = self._connection_compatibility(expected, actual)
            if compatibility == "invalid":
                self._show_action_error("Type Mismatch", f"{topic_item.node.id} is {actual}, expected {expected}.")
                return True
            edge_item.edge.target.node = topic_item.node.id
        edge_item.edge.compatibility = ROS_MESSAGE_COMPATIBILITY if compatibility == ROS_MESSAGE_COMPATIBILITY else ""
        self._rebuild_edges()
        self._refresh_port_visibility()
        self.status.setText(f"Rewired to {topic_item.node.id}")
        if compatibility == ROS_MESSAGE_COMPATIBILITY:
            self._warn_ros_message_compatibility()
        self._sync_live_pipeline()
        return True

    def finish_connection_drag(self, clicked_item, scene_pos: QPointF) -> bool:
        if self.connection_source is None:
            return False
        source = self.connection_source
        source_port = self.connection_source_port
        self._clear_connection_preview()
        self._clear_connection_highlights()
        self.connection_source = None
        self.connection_source_port = "out"
        node_item = self._port_owner(clicked_item) or self._node_item_for_graphics_item(clicked_item)
        if node_item is None:
            self.status.setText("Connection canceled.")
            return True
        if node_item.node.id == source.node.id:
            self._show_action_error("Connect", "Cannot connect a node to itself.")
            return True
        verdict = self._best_connection_verdict_for_target(source.node, source_port, node_item.node)
        if verdict.verdict not in {"exact", ROS_MESSAGE_COMPATIBILITY}:
            self._show_action_error("Connect", verdict.reason or "No compatible free input is available.")
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
        self._clear_connection_highlights()

    def _clear_rewire_preview(self) -> None:
        if self.rewire_preview is not None:
            self.scene.removeItem(self.rewire_preview)
            self.rewire_preview = None
        self._clear_connection_highlights()

    def _add_edge_item(self, edge: Edge, source: NodeItem, target: NodeItem) -> None:
        item = EdgeItem(edge, source, target)
        self.scene.addItem(item)
        self.edge_items.append(item)

    def _add_visual_edge_item(
        self,
        edge: Edge,
        source: NodeItem,
        target: NodeItem,
        graph_edges: list[Edge],
        collapsed_topic_id: str,
    ) -> None:
        item = EdgeItem(
            edge,
            source,
            target,
            graph_edges=graph_edges,
            collapsed_topic_id=collapsed_topic_id,
        )
        self.scene.addItem(item)
        self.edge_items.append(item)

    def _refresh_port_visibility(self) -> None:
        for item in self.items_by_id.values():
            item.update_port_visibility()

    def _refresh_topic_visibility(self) -> None:
        for item in self.items_by_id.values():
            if item.node.type == "topic":
                item.setVisible(self.show_topics)

    def refresh_edges(self) -> None:
        for edge in self.edge_items:
            edge.refresh()

    def delete_selected(self) -> None:
        selected_nodes = self._selected_node_items()
        selected_edges = self._selected_edge_items()
        if not selected_nodes and not selected_edges:
            return
        selected_ids = {item.node.id for item in selected_nodes}
        selected_edge_ids = {id(edge) for item in selected_edges for edge in item.graph_edges}
        selected_collapsed_topic_ids = {
            item.collapsed_topic_id
            for item in selected_edges
            if item.collapsed_topic_id
        }
        self.graph.nodes = [node for node in self.graph.nodes if node.id not in selected_ids]
        self.graph.edges = [
            edge
            for edge in self.graph.edges
            if id(edge) not in selected_edge_ids
            and edge.source.node not in selected_ids
            and edge.target.node not in selected_ids
        ]
        self._remove_orphan_collapsed_topic_publishers(selected_collapsed_topic_ids)
        for item in selected_nodes:
            self.scene.removeItem(item)
            self.items_by_id.pop(item.node.id, None)
        self._rebuild_edges()
        self._refresh_port_visibility()
        self._sync_live_pipeline()

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
        self._set_dialog_default_size(dialog)
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
            item.refresh()
            self._sync_live_pipeline()

    def edit_node(self, item: NodeItem) -> None:
        node = item.node
        dialog = QDialog(self.widget)
        dialog.setWindowTitle(f"Edit {node.id}")
        layout = QVBoxLayout(dialog)
        name_edit: QLineEdit | None = None
        topic_edit: QLineEdit | None = None
        parameter_widgets: dict[str, QLineEdit | QCheckBox] = {}
        collect_chain_parameters = None
        input_qos_widgets: dict[str, dict[str, QLineEdit | QComboBox]] = {}
        output_qos_widgets: dict[str, dict[str, QLineEdit | QComboBox]] = {}
        sync_widgets: dict[str, QLineEdit | QComboBox] = {}
        if node.type == "filter":
            try:
                self._refresh_filter_parameters_for_dialog(node)
            except Exception as error:
                QMessageBox.critical(self.widget, "Parameter Discovery Failed", str(error))
                return
            self._ensure_filter_port_configs(node)
            if self._filter_has_multiple_inputs(node) and not node.sync:
                node.sync = self._default_sync()
            tabs = QTabWidget(dialog)
            general = QWidget(dialog)
            general_form = QFormLayout(general)
            name_edit = QLineEdit(node.name or node.id, dialog)
            general_form.addRow("Name", name_edit)
            general_form.addRow("Filter", self._readonly_field(f"{node.package}/{node.filter}"))
            general_form.addRow("Inputs", self._readonly_field(self._port_summary([node.input_ports or node.input_type])))
            general_form.addRow(
                "Outputs",
                self._readonly_field(self._port_summary([node.output_ports or node.output_type])),
            )
            tabs.addTab(general, "General")
            parameter_refreshers = []
            def collect_parameter_widgets() -> dict[str, object]:
                return {
                    key: self._parameter_widget_value(key, widget, node.parameters[key])
                    for key, widget in parameter_widgets.items()
                }

            if self._is_filter_chain(node):
                collect_chain_parameters = self._add_chain_tab(
                    tabs,
                    dialog,
                    node,
                    lambda: parameter_refreshers[0]() if parameter_refreshers else None,
                    collect_parameter_widgets,
                )
            parameter_widgets = self._add_parameters_tab(tabs, dialog, node, parameter_refreshers)
            input_qos_widgets = self._add_port_tab(tabs, dialog, node, False)
            output_qos_widgets = self._add_port_tab(tabs, dialog, node, True)
            if self._filter_has_multiple_inputs(node):
                sync = QWidget(dialog)
                sync_form = QFormLayout(sync)
                mode = QComboBox(dialog)
                mode.addItems(["receipt_time", "latest"])
                mode.setCurrentText(str(node.sync.get("mode", "receipt_time")))
                sync_widgets["mode"] = mode
                sync_form.addRow("Mode", mode)
                queue_size = QLineEdit(str(node.sync.get("queue_size", 10)), dialog)
                sync_widgets["queue_size"] = queue_size
                sync_form.addRow("Queue size", queue_size)
                max_interval = QLineEdit(str(node.sync.get("max_interval", 0.05)), dialog)
                sync_widgets["max_interval"] = max_interval
                sync_form.addRow("Max interval seconds", max_interval)
                tabs.addTab(sync, "Sync")
            layout.addWidget(tabs)
        else:
            tabs = QTabWidget(dialog)
            general = QWidget(dialog)
            form = QFormLayout(general)
            topic_edit = QLineEdit(node.topic, dialog)
            form.addRow("Topic", topic_edit)
            logical_type = node.output_type or node.input_type
            form.addRow("Type", self._readonly_field(logical_type or "unknown"))
            ros_type = self.message_type_by_logical.get(logical_type, "unknown") if logical_type else "unknown"
            form.addRow("ROS type", self._readonly_field(ros_type))
            type_adapter = self.type_adapter_by_logical.get(logical_type, "unknown") if logical_type else "unknown"
            form.addRow("Type adapter", self._readonly_field(type_adapter))
            type_package = self.type_package_by_logical.get(logical_type, "unknown") if logical_type else "unknown"
            form.addRow("Type package", self._readonly_field(type_package))
            tabs.addTab(general, "General")
            self._add_topic_connection_tab(tabs, dialog, node, True)
            self._add_topic_connection_tab(tabs, dialog, node, False)
            layout.addWidget(tabs)
        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel, dialog)
        buttons.accepted.connect(dialog.accept)
        buttons.rejected.connect(dialog.reject)
        layout.addWidget(buttons)
        self._set_dialog_default_size(dialog)
        while dialog.exec_() == QDialog.Accepted:
            if node.type == "filter" and name_edit is not None:
                try:
                    parameters = {
                        key: self._parameter_widget_value(key, widget, node.parameters[key])
                        for key, widget in parameter_widgets.items()
                    }
                    if collect_chain_parameters is not None:
                        node.parameters = parameters
                        parameters = collect_chain_parameters()
                    sync = {}
                    if sync_widgets:
                        sync = {
                            "mode": self._qos_widget_text(sync_widgets["mode"]),
                            "queue_size": self._parse_typed_scalar(
                                self._qos_widget_text(sync_widgets["queue_size"]),
                                10,
                                "Queue size",
                            ),
                            "max_interval": self._parse_typed_scalar(
                                self._qos_widget_text(sync_widgets["max_interval"]),
                                0.05,
                                "Max interval seconds",
                            ),
                        }
                    inputs = self._collect_port_qos(node.inputs, input_qos_widgets)
                    outputs = self._collect_port_qos(node.outputs, output_qos_widgets)
                except ValueError as error:
                    QMessageBox.critical(self.widget, "Invalid Value", str(error))
                    continue
                if not self._rename_node(node, name_edit.text().strip()):
                    return
                node.name = node.id
                node.parameters = parameters
                if sync_widgets:
                    node.sync = sync
                node.inputs = inputs
                node.outputs = outputs
            elif topic_edit is not None:
                if not self._rename_node(node, topic_edit.text().strip()):
                    return
                node.topic = node.id
            self._redraw_node(item)
            self._sync_live_pipeline()
            return

    def _readonly_field(self, text: str) -> QLineEdit:
        field = QLineEdit(text, self.widget)
        field.setReadOnly(True)
        return field

    def _topic_connection_rows(self, topic_node: Node, publishers: bool) -> list[tuple[str, str, str]]:
        nodes_by_id = {graph_node.id: graph_node for graph_node in self.graph.nodes}
        rows: list[tuple[str, str, str]] = []
        for edge in self.graph.edges:
            if publishers:
                if edge.target.node != topic_node.id:
                    continue
                filter_node = nodes_by_id.get(edge.source.node)
                if filter_node is None or filter_node.type != "filter":
                    continue
                port = self._canonical_output_port(filter_node, edge.source.port)
                description = self._parameter_description(filter_node, f"outputs.{port}.topic")
            else:
                if edge.source.node != topic_node.id:
                    continue
                filter_node = nodes_by_id.get(edge.target.node)
                if filter_node is None or filter_node.type != "filter":
                    continue
                port = self._canonical_input_port(filter_node, edge.target.port)
                description = self._parameter_description(filter_node, f"inputs.{port}.topic")
            rows.append((self._filter_display_name(filter_node), port, description or "No description."))
        return rows

    def _filter_display_name(self, node: Node) -> str:
        name = node.name or node.id
        if name == node.id:
            return node.id
        return f"{name} ({node.id})"

    def _add_topic_connection_tab(
        self,
        tabs: QTabWidget,
        dialog: QDialog,
        topic_node: Node,
        publishers: bool,
    ) -> None:
        scroll = QScrollArea(dialog)
        scroll.setWidgetResizable(True)
        page = QWidget(scroll)
        form = QFormLayout(page)
        rows = self._topic_connection_rows(topic_node, publishers)
        if not rows:
            form.addRow(QLabel("No publishers." if publishers else "No subscribers.", dialog))
        for index, (filter_name, port, description) in enumerate(rows):
            if index:
                line = QFrame(dialog)
                line.setFrameShape(QFrame.HLine)
                line.setFrameShadow(QFrame.Sunken)
                form.addRow(line)
            form.addRow("Filter", self._readonly_field(filter_name))
            form.addRow("Port", self._readonly_field(port or ("out" if publishers else "in")))
            form.addRow("Description", self._readonly_field(description))
        scroll.setWidget(page)
        tabs.addTab(scroll, "Publisher" if publishers else "Subscriber")

    def _add_parameters_tab(
        self,
        tabs: QTabWidget,
        dialog: QDialog,
        node: Node,
        refreshers=None,
    ) -> dict[str, QLineEdit | QCheckBox]:
        parameter_widgets: dict[str, QLineEdit | QCheckBox] = {}
        parameters_scroll = QScrollArea(dialog)
        parameters_scroll.setWidgetResizable(True)
        parameters = QWidget(parameters_scroll)
        parameters_layout = QVBoxLayout(parameters)
        parameters_tree = QTreeWidget(parameters)
        parameters_tree.setColumnCount(2)
        parameters_tree.setHeaderLabels(["Parameter", "Value"])
        parameters_layout.addWidget(parameters_tree)

        def clear() -> None:
            parameters_tree.clear()

        def rebuild() -> None:
            clear()
            parameter_widgets.clear()
            if node.parameters:
                parameter_widgets.update(self._populate_parameter_tree(
                    dialog,
                    parameters_tree,
                    node.parameters,
                    lambda key: self._parameter_description(node, key),
                ))
            else:
                parameters_tree.addTopLevelItem(QTreeWidgetItem(["No editable parameters.", ""]))

        rebuild()
        if refreshers is not None:
            refreshers.append(rebuild)
        parameters_scroll.setWidget(parameters)
        tabs.addTab(parameters_scroll, "Params")
        return parameter_widgets

    def _populate_parameter_tree(
        self,
        dialog: QDialog,
        tree: QTreeWidget,
        parameters: dict[str, object],
        description_for,
    ) -> dict[str, QLineEdit | QCheckBox]:
        widgets: dict[str, QLineEdit | QCheckBox] = {}
        groups: dict[tuple[str, ...], QTreeWidgetItem] = {}
        for key in sorted(parameters):
            value = parameters[key]
            parts = key.split(".")
            parent = None
            group_path: list[str] = []
            for part in parts[:-1]:
                group_path.append(part)
                path_key = tuple(group_path)
                if path_key in groups:
                    parent = groups[path_key]
                    continue
                group_item = QTreeWidgetItem([part, ""])
                group_item.setExpanded(True)
                if parent is None:
                    tree.addTopLevelItem(group_item)
                else:
                    parent.addChild(group_item)
                groups[path_key] = group_item
                parent = group_item
            leaf = QTreeWidgetItem([parts[-1], ""])
            if parent is None:
                tree.addTopLevelItem(leaf)
            else:
                parent.addChild(leaf)
            widget = self._parameter_editor_widget(dialog, value)
            description = description_for(key)
            if description:
                leaf.setToolTip(0, description)
                widget.setToolTip(description)
                if isinstance(widget, QLineEdit):
                    widget.setPlaceholderText(description)
            widgets[key] = widget
            tree.setItemWidget(leaf, 1, widget)
        tree.expandAll()
        tree.resizeColumnToContents(0)
        return widgets

    def _parameter_editor_widget(self, dialog: QDialog, value: object) -> QLineEdit | QCheckBox:
        if isinstance(value, bool):
            widget = QCheckBox(dialog)
            widget.setChecked(value)
            return widget
        return QLineEdit(str(value), dialog)

    def _parameter_label(
        self,
        dialog: QDialog,
        node: Node,
        key: str,
        widget: QLineEdit | QCheckBox,
    ) -> QLabel:
        description = self._parameter_description(node, key)
        label = QLabel(key, dialog)
        if description:
            label.setToolTip(description)
            widget.setToolTip(description)
            if isinstance(widget, QLineEdit):
                widget.setPlaceholderText(description)
        return label

    def _add_chain_tab(
        self,
        tabs: QTabWidget,
        dialog: QDialog,
        node: Node,
        refresh_parameters=None,
        collect_current_parameters=None,
    ):
        scroll = QScrollArea(dialog)
        scroll.setWidgetResizable(True)
        page = QWidget(scroll)
        layout = QVBoxLayout(page)
        controls = QHBoxLayout()
        add_button = QPushButton("Add", dialog)
        remove_button = QPushButton("Remove", dialog)
        up_button = QPushButton("Move Up", dialog)
        down_button = QPushButton("Move Down", dialog)
        for button in (add_button, remove_button, up_button, down_button):
            controls.addWidget(button)
        layout.addLayout(controls)
        entries = self._chain_entries(node)
        selected_index = {"value": 0}
        plugins = self._compatible_chain_plugins(node)
        chain_list = QListWidget(dialog)
        layout.addWidget(chain_list, 1)

        def list_label(index: int, entry: dict[str, object]) -> str:
            name = str(entry.get("name", "")).strip() or "(unnamed)"
            plugin_type = str(entry.get("type", "")).strip() or "(no plugin)"
            return f"{index + 1}. {name} ({plugin_type})"

        def refresh_entries_from_parameters() -> None:
            entries[:] = self._chain_entries(node)

        def rebuild_list() -> None:
            chain_list.clear()
            for index, entry in enumerate(entries):
                chain_list.addItem(list_label(index, entry))
            if entries:
                selected_index["value"] = max(0, min(selected_index["value"], len(entries) - 1))
                chain_list.setCurrentRow(selected_index["value"])

        def selection_changed() -> None:
            row = chain_list.currentRow()
            if row >= 0:
                selected_index["value"] = row

        def collect() -> dict[str, object]:
            refresh_entries_from_parameters()
            current_entries = self._chain_entries(node)
            for index, entry in enumerate(entries):
                if index < len(current_entries):
                    entry["params"] = current_entries[index].get("params", {})
            self._rewrite_chain_parameters(node, entries)
            return dict(node.parameters)

        def apply_chain_change(new_entries: list[dict[str, object]]) -> None:
            entries[:] = new_entries
            self._rewrite_chain_parameters(node, entries)
            try:
                self._refresh_filter_parameters_for_dialog(node)
            except Exception as error:
                QMessageBox.critical(self.widget, "Parameter Discovery Failed", str(error))
                return
            if refresh_parameters is not None:
                refresh_parameters()
            entries[:] = self._chain_entries(node)
            rebuild_list()
            self._sync_live_pipeline()

        def add_filter() -> None:
            if not plugins:
                QMessageBox.warning(self.widget, "No Plugins", "No compatible filters plugins were discovered.")
                return
            plugin = self._choose_chain_plugin(plugins)
            if plugin is None:
                return
            current = collect()
            node.parameters = current
            refresh_entries_from_parameters()
            entries.append({
                "name": self._unique_chain_filter_name(plugin.name, entries),
                "type": plugin.name,
                "params": dict(self._default_chain_filter_params(plugin.name)),
            })
            selected_index["value"] = len(entries) - 1
            apply_chain_change(entries)

        def remove_filter() -> None:
            collect()
            refresh_entries_from_parameters()
            if not entries:
                return
            index = selected_index["value"]
            apply_chain_change([entry for row, entry in enumerate(entries) if row != index])

        def move_filter(offset: int) -> None:
            collect()
            refresh_entries_from_parameters()
            index = selected_index["value"]
            target = index + offset
            if target < 0 or target >= len(entries):
                return
            entries[index], entries[target] = entries[target], entries[index]
            selected_index["value"] = target
            apply_chain_change(entries)

        def edit_filter_params(item=None) -> None:
            if collect_current_parameters is not None:
                node.parameters = collect_current_parameters()
            refresh_entries_from_parameters()
            row = chain_list.row(item) if item is not None else chain_list.currentRow()
            if row < 0 or row >= len(entries):
                return
            if self._edit_chain_filter_params(dialog, node, row):
                if refresh_parameters is not None:
                    refresh_parameters()
                refresh_entries_from_parameters()
                rebuild_list()

        add_button.clicked.connect(add_filter)
        remove_button.clicked.connect(remove_filter)
        up_button.clicked.connect(lambda: move_filter(-1))
        down_button.clicked.connect(lambda: move_filter(1))
        chain_list.itemSelectionChanged.connect(selection_changed)
        chain_list.itemDoubleClicked.connect(edit_filter_params)

        rebuild_list()
        scroll.setWidget(page)
        tabs.addTab(scroll, "Chain")
        return collect

    def _edit_chain_filter_params(self, parent: QDialog, node: Node, index: int) -> bool:
        entries = self._chain_entries(node)
        if index < 0 or index >= len(entries):
            return False
        entry = entries[index]
        prefix = self._chain_param_prefix(node)
        parameter_prefix = f"{prefix}.filter{index + 1}.params."
        scoped_parameters = {
            key.removeprefix(parameter_prefix): value
            for key, value in node.parameters.items()
            if key.startswith(parameter_prefix)
        }
        dialog = QDialog(parent)
        name = str(entry.get("name", "")).strip() or f"filter{index + 1}"
        dialog.setWindowTitle(f"{name} Parameters")
        layout = QVBoxLayout(dialog)
        tree = QTreeWidget(dialog)
        tree.setColumnCount(2)
        tree.setHeaderLabels(["Parameter", "Value"])
        layout.addWidget(tree)
        widgets: dict[str, QLineEdit | QCheckBox] = {}

        def rebuild_tree() -> None:
            tree.clear()
            widgets.clear()
            if scoped_parameters:
                widgets.update(self._populate_parameter_tree(
                    dialog,
                    tree,
                    scoped_parameters,
                    lambda key: self._parameter_description(node, f"{parameter_prefix}{key}"),
                ))
            else:
                tree.addTopLevelItem(QTreeWidgetItem(["No editable parameters.", ""]))

        def selected_parameter_name() -> str:
            item = tree.currentItem()
            if item is None:
                return ""
            parts = [item.text(0)]
            parent = item.parent()
            while parent is not None:
                parts.append(parent.text(0))
                parent = parent.parent()
            parts.reverse()
            name = ".".join(part for part in parts if part)
            return name if name in scoped_parameters else ""

        def add_parameter() -> None:
            name, accepted = QInputDialog.getText(dialog, "Add Parameter", "Parameter name")
            name = name.strip() if accepted else ""
            if not name:
                return
            if name in scoped_parameters:
                QMessageBox.warning(dialog, "Duplicate Parameter", f"Parameter {name} already exists.")
                return
            scoped_parameters[name] = ""
            rebuild_tree()

        def remove_parameter() -> None:
            name = selected_parameter_name()
            if not name:
                return
            scoped_parameters.pop(name, None)
            node.parameters.pop(f"{parameter_prefix}{name}", None)
            rebuild_tree()

        rebuild_tree()
        controls = QHBoxLayout()
        add_button = QPushButton("Add Parameter", dialog)
        remove_button = QPushButton("Remove Selected", dialog)
        add_button.clicked.connect(add_parameter)
        remove_button.clicked.connect(remove_parameter)
        controls.addWidget(add_button)
        controls.addWidget(remove_button)
        layout.addLayout(controls)
        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel, dialog)
        buttons.accepted.connect(dialog.accept)
        buttons.rejected.connect(dialog.reject)
        layout.addWidget(buttons)
        self._set_dialog_default_size(dialog, width=640, height=420)
        while dialog.exec_() == QDialog.Accepted:
            try:
                for key, widget in widgets.items():
                    node.parameters[f"{parameter_prefix}{key}"] = self._parameter_widget_value(
                        key,
                        widget,
                        scoped_parameters[key],
                    )
            except ValueError as error:
                QMessageBox.critical(self.widget, "Invalid Value", str(error))
                continue
            return True
        return False

    def _choose_chain_plugin(self, plugins: list[FilterPluginExport]) -> FilterPluginExport | None:
        dialog = QDialog(self.widget)
        dialog.setWindowTitle("Add Chain Filter")
        layout = QVBoxLayout(dialog)
        search = QLineEdit(dialog)
        search.setPlaceholderText("Search plugins")
        layout.addWidget(search)
        plugin_list = QListWidget(dialog)
        layout.addWidget(plugin_list, 1)
        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel, dialog)
        layout.addWidget(buttons)

        visible_plugins: list[FilterPluginExport] = []

        def label_for(plugin: FilterPluginExport) -> str:
            if plugin.description:
                return f"{plugin.name}\n{plugin.description}"
            return plugin.name

        def refresh() -> None:
            query = search.text().strip().lower()
            visible_plugins[:] = [
                plugin
                for plugin in plugins
                if not query
                or query in plugin.name.lower()
                or query in plugin.type.lower()
                or query in plugin.description.lower()
                or query in plugin.package.lower()
            ]
            plugin_list.clear()
            for index, plugin in enumerate(visible_plugins):
                plugin_list.addItem(label_for(plugin))
                item = plugin_list.item(plugin_list.count() - 1)
                if item is not None:
                    item.setData(Qt.UserRole, index)
            if visible_plugins:
                plugin_list.setCurrentRow(0)

        def selected_visible_plugin() -> FilterPluginExport | None:
            items = plugin_list.selectedItems()
            item = items[0] if items else plugin_list.currentItem()
            if item is None:
                return None
            index = item.data(Qt.UserRole)
            if not isinstance(index, int) or index < 0 or index >= len(visible_plugins):
                return None
            return visible_plugins[index]

        search.textChanged.connect(refresh)
        buttons.accepted.connect(dialog.accept)
        buttons.rejected.connect(dialog.reject)
        plugin_list.itemClicked.connect(lambda item: plugin_list.setCurrentItem(item))
        plugin_list.itemDoubleClicked.connect(lambda item: (plugin_list.setCurrentItem(item), dialog.accept()))
        refresh()
        self._set_dialog_default_size(dialog, width=760, height=460)
        if dialog.exec_() != QDialog.Accepted:
            return None
        return selected_visible_plugin()

    def _ensure_filter_port_configs(self, node: Node) -> None:
        for port, config in self._default_port_configs(node.input_ports or node.input_type, False).items():
            node.inputs.setdefault(port, config)
            node.inputs[port].setdefault("qos", self._default_qos())
        for port, config in self._default_port_configs(node.output_ports or node.output_type, True).items():
            node.outputs.setdefault(port, config)
            node.outputs[port].setdefault("qos", self._default_qos())

    def _add_port_tab(
        self,
        tabs: QTabWidget,
        dialog: QDialog,
        node: Node,
        outgoing: bool,
    ) -> dict[str, dict[str, QLineEdit | QComboBox]]:
        scroll = QScrollArea(dialog)
        scroll.setWidgetResizable(True)
        page = QWidget(scroll)
        form = QFormLayout(page)
        configs = node.outputs if outgoing else node.inputs
        widgets: dict[str, dict[str, QLineEdit | QComboBox]] = {}
        options = self._port_options(node, outgoing)
        for index, (port, stream_type, _label) in enumerate(options):
            if index:
                line = QFrame(dialog)
                line.setFrameShape(QFrame.HLine)
                line.setFrameShadow(QFrame.Sunken)
                form.addRow(line)
            topic = self._connected_topic(node, port, outgoing)
            topic_parameter = (
                f"outputs.{port}.topic" if outgoing else f"inputs.{port}.topic"
            )
            description = self._parameter_description(node, topic_parameter)
            form.addRow(
                f"{port} description",
                self._readonly_field(description or "No description."),
            )
            form.addRow(f"{port} type", self._readonly_field(stream_type or "unknown"))
            form.addRow(f"{port} topic", self._readonly_field(topic or "unconnected"))
            qos = dict(self._default_qos())
            qos.update((configs.get(port, {}) or {}).get("qos", {}) or {})
            port_widgets: dict[str, QLineEdit | QComboBox] = {}
            reliability = QComboBox(dialog)
            reliability.addItems(["best_effort", "reliable"])
            reliability.setCurrentText(str(qos.get("reliability", "best_effort")))
            port_widgets["reliability"] = reliability
            form.addRow(f"{port} reliability", reliability)
            history = QComboBox(dialog)
            history.addItems(["keep_last", "keep_all"])
            history.setCurrentText(str(qos.get("history", "keep_last")))
            port_widgets["history"] = history
            form.addRow(f"{port} history", history)
            depth = QLineEdit(str(qos.get("depth", 5)), dialog)
            port_widgets["depth"] = depth
            form.addRow(f"{port} depth", depth)
            durability = QComboBox(dialog)
            durability.addItems(["volatile", "transient_local"])
            durability.setCurrentText(str(qos.get("durability", "volatile")))
            port_widgets["durability"] = durability
            form.addRow(f"{port} durability", durability)
            widgets[port] = port_widgets
        scroll.setWidget(page)
        tabs.addTab(scroll, "Output" if outgoing else "Input")
        return widgets

    def _connected_topic(self, node: Node, port: str, outgoing: bool) -> str:
        nodes_by_id = {graph_node.id: graph_node for graph_node in self.graph.nodes}
        for edge in self.graph.edges:
            if outgoing and edge.source.node == node.id and self._canonical_output_port(node, edge.source.port) == port:
                target = nodes_by_id.get(edge.target.node)
                if target is not None and target.type == "topic":
                    return self._ros_topic_name(target.topic)
                continue
            if not outgoing and edge.target.node == node.id and self._canonical_input_port(node, edge.target.port) == port:
                source = nodes_by_id.get(edge.source.node)
                if source is not None and source.type == "topic":
                    return self._ros_topic_name(source.topic)
                continue
        return ""

    def _ros_topic_name(self, topic: str) -> str:
        if topic.startswith("~/"):
            return "/" + topic[2:].strip("/").replace("-", "_")
        return topic.replace("-", "_")

    def _collect_port_qos(
        self,
        current: dict[str, object],
        widgets: dict[str, dict[str, QLineEdit | QComboBox]],
    ) -> dict[str, object]:
        collected: dict[str, object] = {}
        for port, port_widgets in widgets.items():
            collected[port] = {
                "qos": {
                    "reliability": self._qos_widget_text(port_widgets["reliability"]),
                    "history": self._qos_widget_text(port_widgets["history"]),
                    "depth": self._parse_typed_scalar(self._qos_widget_text(port_widgets["depth"]), 5, f"{port} depth"),
                    "durability": self._qos_widget_text(port_widgets["durability"]),
                }
            }
        for port, config in current.items():
            collected.setdefault(port, config)
        return collected

    def _port_summary(self, values: list[str]) -> str:
        ports: list[str] = []
        for value in values:
            for item in value.replace(";", ",").split(","):
                item = item.strip()
                if item:
                    ports.append(item)
        return ", ".join(dict.fromkeys(ports)) or "unknown"

    def _filter_has_multiple_inputs(self, node: Node) -> bool:
        return self._has_multiple_input_types(node.input_ports or node.input_type)

    def _has_multiple_input_types(self, input_type: str) -> bool:
        return len(self._stream_types(input_type)) > 1

    def _default_sync(self) -> dict[str, object]:
        return {"mode": "receipt_time", "queue_size": 10, "max_interval": 0.05}

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

    def _parameter_widget_value(self, name: str, widget: QLineEdit | QCheckBox, current):
        if isinstance(widget, QCheckBox):
            return widget.isChecked()
        return self._parse_typed_scalar(widget.text(), current, name)

    def _qos_widget_text(self, widget: QLineEdit | QComboBox) -> str:
        if isinstance(widget, QComboBox):
            return widget.currentText()
        return widget.text()

    def _parse_typed_scalar(self, text: str, current, name: str = "Value"):
        if isinstance(current, bool):
            return text.strip().lower() in {"1", "true", "yes", "on"}
        try:
            if isinstance(current, int) and not isinstance(current, bool):
                return int(text)
            if isinstance(current, float):
                return float(text)
        except ValueError as error:
            expected = "integer" if isinstance(current, int) and not isinstance(current, bool) else "float"
            raise ValueError(f"{name} must be a valid {expected}.") from error
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
            return data
        data["package"] = node.package
        data["filter"] = node.filter
        data["parameters"] = node.parameters
        data["sync"] = node.sync
        return data

    def _sync_live_pipeline(self) -> None:
        try:
            desired = self._live_component_specs()
            self.graph.validate(message_type_by_logical=self.message_type_by_logical)
            self.live_runtime.sync(desired)
            self._verify_live_pipeline_loaded(desired)
            if desired:
                self.status.setText(f"Live pipeline running with {len(desired)} component(s).")
            self.last_live_runtime_error = ""
        except Exception as error:
            message = str(error)
            self.status.setText(f"Live pipeline error: {message}")
            if message != self.last_live_runtime_error:
                self.last_live_runtime_error = message
                QMessageBox.critical(self.widget, "Live Pipeline Error", message)

    def _verify_live_pipeline_loaded(self, desired: dict[str, dict[str, object]]) -> None:
        actual = {name.strip("/").rsplit("/", 1)[-1] for name in self.live_runtime.loaded_components()}
        expected = set(desired)
        missing = sorted(expected - actual)
        extra = sorted(actual - expected)
        if missing or extra:
            details = []
            if missing:
                details.append("Missing: " + ", ".join(missing))
            if extra:
                details.append("Extra: " + ", ".join(extra))
            raise RuntimeError("Live container does not match the editor graph. " + "; ".join(details))

    def _live_component_specs(self) -> dict[str, dict[str, object]]:
        specs: dict[str, dict[str, object]] = {}
        for node in self.graph.nodes:
            if node.type != "filter":
                continue
            self._ensure_filter_port_configs(node)
            specs[node.id] = {
                "package": node.package,
                "component_class": node.component_class or f"{node.package}::{node.filter}Component",
                "parameters": self._live_parameters_for_node(node),
            }
            if self._is_filter_chain(node):
                specs[node.id]["reload_on_parameter_change"] = True
        return specs

    def _live_parameters_for_node(self, node: Node) -> dict[str, object]:
        self._sanitize_filter_parameters(node)
        parameters = dict(node.parameters)
        for port, _stream_type, _label in self._port_options(node, False):
            topic = self._connected_topic(node, port, False)
            if topic:
                parameters[f"inputs.{port}.topic"] = topic
            qos = (node.inputs.get(port, {}) or {}).get("qos", {}) or {}
            for key, value in qos.items():
                parameters[f"inputs.{port}.qos.{key}"] = value
        for port, _stream_type, _label in self._port_options(node, True):
            topic = self._connected_topic(node, port, True)
            if topic:
                parameters[f"outputs.{port}.topic"] = topic
            qos = (node.outputs.get(port, {}) or {}).get("qos", {}) or {}
            for key, value in qos.items():
                parameters[f"outputs.{port}.qos.{key}"] = value
        for key, value in node.sync.items():
            if self._filter_has_multiple_inputs(node):
                parameters[f"sync.{key}"] = value
        return parameters

    def _sanitize_filter_parameters(self, node: Node) -> None:
        if node.type != "filter":
            return
        self._migrate_legacy_sync_parameters(node)
        if not self._filter_has_multiple_inputs(node):
            node.sync = {}
        defaults = self._declared_filter_parameter_defaults(node)
        prefix = self._chain_param_prefix(node) if self._is_filter_chain(node) else ""
        node.parameters = {
            key: value
            for key, value in node.parameters.items()
            if key in defaults or (prefix and self._is_chain_parameter_key(key, prefix))
        }

    def _migrate_legacy_sync_parameters(self, node: Node) -> None:
        if not self._filter_has_multiple_inputs(node):
            return
        for key in ("policy", "queue_size", "slop", "max_interval"):
            if key in node.parameters:
                node.sync.setdefault(key, node.parameters.pop(key))

    def _declared_filter_parameter_defaults(self, node: Node) -> dict[str, object]:
        cache_key = self._component_cache_key(node.package, node.filter, node.component_class)
        if cache_key in self.parameter_defaults_by_component:
            return self.parameter_defaults_by_component[cache_key]
        export = FilterExport(
            package=node.package,
            filter=node.filter,
            component_class=self._component_class_for_node(node),
            input_type=node.input_type,
            output_type=node.output_type,
            input_ports=node.input_ports,
            output_ports=node.output_ports,
        )
        try:
            return self._component_parameter_metadata(export).defaults
        except Exception:
            return dict(node.parameters)

    def _component_class_for_node(self, node: Node) -> str:
        return node.component_class or f"{node.package}::{node.filter}Component"

    def _component_cache_key(self, package: str, filter_name: str, component_class: str) -> str:
        return component_class or f"{package}::{filter_name}Component"

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
        self.graph.editor = {
            "orientation": "top_down" if self.top_down_mode else "left_right",
            "show_topics": self.show_topics,
            "show_filters": self.show_filters,
        }

    def _refresh_live_filter_parameters_for_save(self) -> None:
        for node in self.graph.nodes:
            if node.type != "filter":
                continue
            loaded = self.live_runtime.loaded.get(node.id)
            if loaded is None:
                continue
            old_parameters = dict(node.parameters)
            metadata = self.parameter_discovery.parameters_for_loaded_node(
                self.live_runtime.node,
                loaded.full_node_name,
            )
            node.parameters = metadata.defaults
            self._restore_filter_chain_parameters(node, old_parameters)
            self.parameter_descriptions[node.component_class] = metadata.descriptions
            self.parameter_defaults_by_component[self._component_cache_key(node.package, node.filter, node.component_class)] = metadata.defaults

    def _save(self) -> None:
        self._sync_positions()
        path, _ = QFileDialog.getSaveFileName(self.widget, "Save Pipeline", "", "YAML (*.yaml *.yml)")
        if path:
            if not path.endswith((".yaml", ".yml")):
                path = f"{path}.yaml"
            try:
                self._refresh_live_filter_parameters_for_save()
                self.graph.validate(message_type_by_logical=self.message_type_by_logical)
                save_graph(self.graph, path)
            except ValueError as error:
                QMessageBox.critical(self.widget, "Invalid Graph", str(error))
            except Exception as error:
                QMessageBox.critical(self.widget, "Parameter Refresh Failed", str(error))

    def _load(self) -> None:
        path, _ = QFileDialog.getOpenFileName(self.widget, "Load Pipeline", "", "YAML (*.yaml *.yml)")
        if not path:
            return
        try:
            self.graph = load_graph(path)
        except Exception as error:
            QMessageBox.critical(self.widget, "Load Failed", str(error))
            return
        orientation = self.graph.editor.get("orientation", "top_down")
        self.top_down_mode = orientation == "top_down"
        self.show_topics = bool(self.graph.editor.get("show_topics", True))
        self.show_filters = bool(self.graph.editor.get("show_filters", True))
        self.top_down_toggle.blockSignals(True)
        self.top_down_toggle.setChecked(self.top_down_mode)
        self.top_down_toggle.blockSignals(False)
        self.topic_visibility_toggle.blockSignals(True)
        self.topic_visibility_toggle.setChecked(self.show_topics)
        self.topic_visibility_toggle.blockSignals(False)
        self.filter_visibility_toggle.blockSignals(True)
        self.filter_visibility_toggle.setChecked(self.show_filters)
        self.filter_visibility_toggle.blockSignals(False)
        self.items_by_id.clear()
        self.edge_items.clear()
        self.scene.clear()
        for node in self.graph.nodes:
            item = NodeItem(node, self)
            item.setPos(node.position["x"], node.position["y"])
            self.scene.addItem(item)
            self.items_by_id[node.id] = item
            self.expand_scene_for_item(item)
        self._refresh_topic_visibility()
        self._rebuild_edges()
        self.fit_graph_view()
        self._sync_live_pipeline()

    def _rebuild_edges(self) -> None:
        for item in self.edge_items:
            self.scene.removeItem(item)
        self.edge_items.clear()
        self._refresh_topic_visibility()
        if not self.show_topics:
            self._rebuild_collapsed_topic_edges()
            self._refresh_port_visibility()
            return
        for edge in self.graph.edges:
            source = self.items_by_id.get(edge.source.node)
            target = self.items_by_id.get(edge.target.node)
            if source and target:
                self._add_edge_item(edge, source, target)
        self._refresh_port_visibility()

    def _rebuild_collapsed_topic_edges(self) -> None:
        nodes_by_id = {node.id: node for node in self.graph.nodes}
        incoming_by_topic: dict[str, list[Edge]] = {}
        outgoing_by_topic: dict[str, list[Edge]] = {}
        for edge in self.graph.edges:
            source_node = nodes_by_id.get(edge.source.node)
            target_node = nodes_by_id.get(edge.target.node)
            if source_node is None or target_node is None:
                continue
            if source_node.type == "filter" and target_node.type == "topic":
                incoming_by_topic.setdefault(target_node.id, []).append(edge)
            elif source_node.type == "topic" and target_node.type == "filter":
                outgoing_by_topic.setdefault(source_node.id, []).append(edge)
            elif source_node.type != "topic" and target_node.type != "topic":
                source = self.items_by_id.get(edge.source.node)
                target = self.items_by_id.get(edge.target.node)
                if source is not None and target is not None:
                    self._add_edge_item(edge, source, target)

        rendered_pairs: set[tuple[str, str, str, str]] = set()
        for topic_id, incoming_edges in incoming_by_topic.items():
            for incoming in incoming_edges:
                source = self.items_by_id.get(incoming.source.node)
                if source is None:
                    continue
                for outgoing in outgoing_by_topic.get(topic_id, []):
                    target = self.items_by_id.get(outgoing.target.node)
                    if target is None:
                        continue
                    pair = (
                        incoming.source.node,
                        incoming.source.port,
                        outgoing.target.node,
                        outgoing.target.port,
                    )
                    if pair in rendered_pairs:
                        continue
                    rendered_pairs.add(pair)
                    compatibility = incoming.compatibility or outgoing.compatibility
                    visual_edge = Edge(
                        PortRef(incoming.source.node, incoming.source.port, "output"),
                        PortRef(outgoing.target.node, outgoing.target.port, "input"),
                        compatibility=compatibility,
                    )
                    self._add_visual_edge_item(visual_edge, source, target, [outgoing], topic_id)

    def _remove_orphan_collapsed_topic_publishers(self, topic_ids: set[str]) -> None:
        if not topic_ids:
            return
        topics_with_subscribers = {
            edge.source.node
            for edge in self.graph.edges
            if edge.source.node in topic_ids
        }
        orphan_topic_ids = topic_ids - topics_with_subscribers
        if not orphan_topic_ids:
            return
        self.graph.edges = [
            edge
            for edge in self.graph.edges
            if edge.target.node not in orphan_topic_ids
        ]
