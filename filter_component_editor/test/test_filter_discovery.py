# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

from filter_component_editor.filter_discovery import (
    FilterExport,
    discover_filters,
    discover_filter_plugins,
    load_filter_plugin_defaults,
    parse_filter_plugin_xml,
)


def test_filter_export_defaults_to_normal_filter_kind() -> None:
    export = FilterExport(
        package="test_filters",
        filter="Passthrough",
        component_class="test_filters::PassthroughComponent",
    )

    assert export.kind == "filter"
    assert export.chain_data_type == ""
    assert export.implementation == "cpp"


def test_filter_export_supports_python_filters() -> None:
    export = FilterExport(
        package="test_filters",
        filter="NumpyCloud",
        component_class="",
        implementation="python",
        python_module="test_filters.numpy_cloud",
        python_class="NumpyCloudFilter",
    )

    assert export.implementation == "python"
    assert export.python_module == "test_filters.numpy_cloud"
    assert export.python_class == "NumpyCloudFilter"


def test_parse_filter_plugin_xml_reads_filter_base_plugins(tmp_path: Path) -> None:
    plugin_xml = tmp_path / "plugins.xml"
    plugin_xml.write_text(
        """<class_libraries>
  <library path="increment">
    <class name="filters/IncrementFilterInt"
           type="filters::IncrementFilter&lt;int&gt;"
           base_class_type="filters::FilterBase&lt;int&gt;">
      <description>
        Increment an int.
      </description>
    </class>
    <class name="filters/MultiChannelIncrementFilterInt"
           type="filters::MultiChannelIncrementFilter&lt;int&gt;"
           base_class_type="filters::MultiChannelFilterBase&lt;int&gt;" />
  </library>
</class_libraries>
""",
        encoding="utf-8",
    )

    plugins = parse_filter_plugin_xml("filters", plugin_xml)

    assert [plugin.name for plugin in plugins] == [
        "filters/IncrementFilterInt",
        "filters/MultiChannelIncrementFilterInt",
    ]
    assert plugins[0].type == "filters::IncrementFilter<int>"
    assert plugins[0].base_class_type == "filters::FilterBase<int>"
    assert plugins[0].description == "Increment an int."


def test_load_filter_plugin_defaults_reads_yaml_mapping(tmp_path: Path) -> None:
    defaults_yaml = tmp_path / "defaults.yaml"
    defaults_yaml.write_text(
        """gridMapFilters/ThresholdFilter:
  layer: elevation
  lower_threshold: 0.0
  set_to: 0.0
gridMapFilters/BufferNormalizerFilter: {}
""",
        encoding="utf-8",
    )

    defaults = load_filter_plugin_defaults(defaults_yaml)

    assert defaults["gridMapFilters/ThresholdFilter"] == {
        "layer": "elevation",
        "lower_threshold": 0.0,
        "set_to": 0.0,
    }
    assert defaults["gridMapFilters/BufferNormalizerFilter"] == {}


def test_installed_filters_increment_plugin_is_discoverable() -> None:
    plugins = {plugin.name: plugin for plugin in discover_filter_plugins()}

    assert plugins["filters/IncrementFilterInt"].base_class_type == "filters::FilterBase<int>"
    assert plugins["filters/MultiChannelIncrementFilterInt"].base_class_type == (
        "filters::MultiChannelFilterBase<int>"
    )


def test_python_passthrough_filter_is_discoverable_when_installed() -> None:
    filters = {item.filter: item for item in discover_filters().filters}

    if "PythonPointCloudPassthrough" not in filters:
        return
    export = filters["PythonPointCloudPassthrough"]
    assert export.implementation == "python"
    assert export.python_module == "filter_component_python_examples.passthrough_cloud"
    assert export.python_class == "PythonPointCloudPassthrough"


def test_pointcloud2_numpy_type_is_discoverable_when_installed() -> None:
    types = {item.point_type: item for item in discover_filters().types}

    if "PointCloud2" not in types:
        return
    export = types["PointCloud2"]
    assert export.message_type == "sensor_msgs/msg/PointCloud2"
    assert export.type_adapter == "filter_component_base_py.adapters.PointCloud2NumpyAdapter"
