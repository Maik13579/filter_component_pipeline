# grid_map_components_filter_chain

ROS 2 lifecycle component that exposes upstream `filters::FilterChain<grid_map::GridMap>`
through the filter component framework.

The component is exported as `RosFilterChainGridMap`. It uses one `map` input
port, one filtered `map` output port, and one `orig_input` output port with the
unchanged input. All ports use the logical `GridMap` type. The ROS boundary uses
`grid_map_components_type_adapter::ros::GridMapAdapter`, while the filter chain
loads plugins compiled for `filters::FilterBase<grid_map::GridMap>`.

Example factory node entry:

```yaml
package: grid_map_components_filter_chain
filter: RosFilterChainGridMap
component_class: grid_map_components_filter_chain::RosFilterChainGridMapComponent
input_ports: map:GridMap
output_ports: map:GridMap,orig_input:GridMap
```

Example chain parameters:

```yaml
filters:
  filter1:
    name: first_filter
    type: gridMapFilters/BufferNormalizerFilter
    params: filters.filter1.params
```

## Grid Map Filter Defaults

The editor has to create and configure filter-chain plugins before live map
input is available so it can build a runnable graph and expose editable
parameters. That means every filter needs startup-safe defaults for all
configure-time parameters.

Upstream `grid_map_filters` plugins require parameters such as `input_layer`,
`output_layer`, `radius`, and color values, but they do not provide discoverable
defaults through pluginlib metadata. This package therefore installs
`config/filter_plugin_defaults.yaml` and links it from the `RosFilterChainGridMap`
export with `chain_plugin_defaults`. The editor reads that YAML file and seeds
the defaults when a grid-map filter is added. Users should then edit the seeded
values to match their map layers and intended processing chain.
