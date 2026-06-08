# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

from filter_component_editor.filter_discovery import (
    FilterExport,
    discover_filter_plugins,
    parse_filter_plugin_xml,
)


def test_filter_export_defaults_to_normal_filter_kind() -> None:
    export = FilterExport(
        package="demo_filters",
        filter="Passthrough",
        component_class="demo_filters::PassthroughComponent",
    )

    assert export.kind == "filter"
    assert export.chain_data_type == ""
    assert export.chain_param_prefix == "filters"


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


def test_installed_filters_increment_plugin_is_discoverable() -> None:
    plugins = {plugin.name: plugin for plugin in discover_filter_plugins()}

    assert plugins["filters/IncrementFilterInt"].base_class_type == "filters::FilterBase<int>"
    assert plugins["filters/MultiChannelIncrementFilterInt"].base_class_type == (
        "filters::MultiChannelFilterBase<int>"
    )
