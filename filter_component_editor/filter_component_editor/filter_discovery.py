# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from dataclasses import dataclass, field
from pathlib import Path
import xml.etree.ElementTree as ET

from ament_index_python.resources import get_resources
from ament_index_python.packages import get_packages_with_prefixes


@dataclass(frozen=True)
class FilterExport:
    package: str
    filter: str
    component_class: str
    input_type: str = ""
    output_type: str = ""
    input_ports: str = ""
    output_ports: str = ""
    kind: str = "filter"
    chain_data_type: str = ""
    chain_param_prefix: str = "filters"


@dataclass(frozen=True)
class FilterPluginExport:
    package: str
    name: str
    type: str
    base_class_type: str
    description: str = ""


@dataclass(frozen=True)
class TypeExport:
    package: str
    point_type: str
    type_adapter: str = ""
    message_type: str = ""


@dataclass
class DiscoveryResult:
    filters: list[FilterExport] = field(default_factory=list)
    types: list[TypeExport] = field(default_factory=list)
    filter_plugins: list[FilterPluginExport] = field(default_factory=list)


def _component_classes(package: str, prefix: str) -> set[str]:
    resource = Path(prefix) / "share" / "ament_index" / "resource_index" / "rclcpp_components" / package
    if not resource.exists():
        return set()
    classes: set[str] = set()
    for line in resource.read_text(encoding="utf-8").splitlines():
        if line.strip():
            classes.add(line.split(";", 1)[0])
    return classes


def discover_filters() -> DiscoveryResult:
    result = DiscoveryResult()
    result.filter_plugins = discover_filter_plugins()
    for package, prefix in sorted(get_packages_with_prefixes().items()):
        package_xml = Path(prefix) / "share" / package / "package.xml"
        if not package_xml.exists():
            continue
        try:
            root = ET.parse(package_xml).getroot()
        except ET.ParseError:
            continue

        registered_components = _component_classes(package, prefix)
        package_filters: list[str] = []
        filter_types: dict[str, tuple[str, str, str, str, str, str, str]] = {}
        for export in root.findall("export"):
            for item in export.findall("filter_component"):
                point_type = item.attrib.get("type", "")
                if point_type:
                    result.types.append(
                        TypeExport(
                            package=package,
                            point_type=point_type,
                            type_adapter=item.attrib.get("type_adapter", ""),
                            message_type=item.attrib.get("message_type", ""),
                        )
                    )
                filter_name = item.attrib.get("filter", "")
                if filter_name:
                    package_filters.append(filter_name)
                    filter_types[filter_name] = (
                        item.attrib.get("input", ""),
                        item.attrib.get("output", ""),
                        item.attrib.get("input_ports", ""),
                        item.attrib.get("output_ports", ""),
                        item.attrib.get("kind", "filter") or "filter",
                        item.attrib.get("chain_data_type", ""),
                        item.attrib.get("chain_param_prefix", "filters") or "filters",
                    )

        for filter_name in package_filters:
            component_class = f"{package}::{filter_name}Component"
            if component_class not in registered_components:
                continue
            input_type, output_type, input_ports, output_ports, kind, chain_data_type, chain_param_prefix = filter_types[filter_name]
            result.filters.append(
                FilterExport(
                    package=package,
                    filter=filter_name,
                    component_class=component_class,
                    input_type=input_type,
                    output_type=output_type,
                    input_ports=input_ports,
                    output_ports=output_ports,
                    kind=kind,
                    chain_data_type=chain_data_type,
                    chain_param_prefix=chain_param_prefix,
                )
            )
    return result


def discover_filter_plugins() -> list[FilterPluginExport]:
    plugins_by_name: dict[str, FilterPluginExport] = {}
    for package, prefix in sorted(get_resources("filters__pluginlib__plugin").items()):
        resource = (
            Path(prefix)
            / "share"
            / "ament_index"
            / "resource_index"
            / "filters__pluginlib__plugin"
            / package
        )
        if not resource.exists():
            continue
        for line in resource.read_text(encoding="utf-8").splitlines():
            relative_path = line.strip()
            if not relative_path:
                continue
            plugin_xml = Path(prefix) / relative_path
            for plugin in parse_filter_plugin_xml(package, plugin_xml):
                plugins_by_name.setdefault(plugin.name, plugin)
    return sorted(plugins_by_name.values(), key=lambda item: item.name)


def parse_filter_plugin_xml(package: str, plugin_xml: Path) -> list[FilterPluginExport]:
    try:
        root = ET.parse(plugin_xml).getroot()
    except (ET.ParseError, OSError):
        return []
    plugins: list[FilterPluginExport] = []
    for item in root.iter("class"):
        name = item.attrib.get("name", "").strip()
        plugin_type = item.attrib.get("type", "").strip()
        base_class_type = item.attrib.get("base_class_type", "").strip()
        if not name or not plugin_type or not base_class_type:
            continue
        description = " ".join((item.findtext("description") or "").split())
        plugins.append(
            FilterPluginExport(
                package=package,
                name=name,
                type=plugin_type,
                base_class_type=base_class_type,
                description=description,
            )
        )
    return plugins
