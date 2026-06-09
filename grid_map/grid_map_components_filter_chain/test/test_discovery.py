# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

from ament_index_python.packages import get_package_prefix

from filter_component_editor.filter_discovery import discover_filters


def test_grid_map_type_adapter_export_is_discoverable() -> None:
    discovery = discover_filters()
    types = {(item.package, item.point_type): item for item in discovery.types}

    grid_map_type = types[("grid_map_components_type_adapter", "GridMap")]
    assert grid_map_type.message_type == "grid_map_msgs/msg/GridMap"
    assert (
        grid_map_type.type_adapter
        == "grid_map_components_type_adapter::ros::GridMapAdapter"
    )


def test_grid_map_filter_chain_component_is_discoverable_with_metadata() -> None:
    discovery = discover_filters()
    filters = {(item.package, item.filter): item for item in discovery.filters}

    item = filters[("grid_map_components_filter_chain", "RosFilterChainGridMap")]
    assert item.component_class == (
        "grid_map_components_filter_chain::RosFilterChainGridMapComponent"
    )
    assert item.kind == "filter_chain"
    assert item.chain_data_type == "grid_map::GridMap"
    assert item.input_type == "GridMap"
    assert item.input_ports == "map:GridMap"
    assert item.output_type == "GridMap"
    assert item.output_ports == "map:GridMap,orig_input:GridMap"


def test_grid_map_filter_chain_component_registers_in_rclcpp_components_index() -> None:
    prefix = Path(get_package_prefix("grid_map_components_filter_chain"))
    resource = (
        prefix
        / "share"
        / "ament_index"
        / "resource_index"
        / "rclcpp_components"
        / "grid_map_components_filter_chain"
    )
    registered = resource.read_text(encoding="utf-8")

    assert (
        "grid_map_components_filter_chain::RosFilterChainGridMapComponent"
        in registered
    )


def test_grid_map_filters_plugins_are_discoverable_and_chain_compatible() -> None:
    discovery = discover_filters()
    plugins = {
        item.name: item
        for item in discovery.filter_plugins
        if item.package == "grid_map_filters"
    }

    assert plugins
    for plugin_name in (
        "gridMapFilters/ThresholdFilter",
        "gridMapFilters/BufferNormalizerFilter",
        "gridMapFilters/SetBasicLayersFilter",
    ):
        assert plugins[plugin_name].base_class_type == (
            "filters::FilterBase<grid_map::GridMap>"
        )


def test_grid_map_cv_plugins_are_discoverable_and_chain_compatible() -> None:
    discovery = discover_filters()
    plugins = {
        item.name: item
        for item in discovery.filter_plugins
        if item.package == "grid_map_cv"
    }

    assert plugins["gridMapCv/InpaintFilter"].base_class_type == (
        "filters::FilterBase<grid_map::GridMap>"
    )


def test_grid_map_filter_chain_plugin_defaults_are_discoverable() -> None:
    discovery = discover_filters()
    grid_map_plugins = {
        item.name
        for item in discovery.filter_plugins
        if item.package == "grid_map_filters"
        and item.base_class_type == "filters::FilterBase<grid_map::GridMap>"
    }

    defaults = discovery.filter_plugin_defaults

    assert grid_map_plugins
    assert grid_map_plugins.issubset(defaults)
    assert defaults["gridMapFilters/ThresholdFilter"] == {
        "layer": "elevation",
        "lower_threshold": 0.0,
        "set_to": 0.0,
    }
    assert defaults["gridMapFilters/LightIntensityFilter"]["light_direction"] == [
        0.0,
        0.0,
        1.0,
    ]
    assert defaults["gridMapFilters/BufferNormalizerFilter"] == {}
    assert defaults["gridMapCv/InpaintFilter"] == {
        "input_layer": "elevation",
        "output_layer": "inpaint",
        "radius": 0.05,
    }
