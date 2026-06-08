# grid_map_components_filter_chain

ROS 2 lifecycle component that exposes upstream `filters::FilterChain<grid_map::GridMap>`
through the filter component framework.

The component is exported as `RosFilterChainGridMap`. It uses one `map` input
port and one `map` output port, both using the logical `GridMap` type. The ROS
boundary uses `grid_map_components_type_adapter::ros::GridMapAdapter`, while the
filter chain loads plugins compiled for `filters::FilterBase<grid_map::GridMap>`.

Example factory node entry:

```yaml
package: grid_map_components_filter_chain
filter: RosFilterChainGridMap
component_class: grid_map_components_filter_chain::RosFilterChainGridMapComponent
input_ports: map:GridMap
output_ports: map:GridMap
```

Chain parameters use the fixed `filters` prefix:

```yaml
filters:
  filter1:
    name: first_filter
    type: gridMapFilters/BufferNormalizerFilter
    params: filters.filter1.params
```
