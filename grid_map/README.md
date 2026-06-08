# Grid Map Packages

This directory groups filter-component integration packages for the upstream
ANYbotics [`grid_map`](https://github.com/ANYbotics/grid_map/tree/jazzy)
Jazzy packages.

## Packages

- [grid_map_components_type_adapter](grid_map_components_type_adapter/README.md):
  `rclcpp::TypeAdapter` support and logical type discovery metadata for
  `grid_map::GridMap`.
- [grid_map_components_filter_chain](grid_map_components_filter_chain/README.md):
  lifecycle component wrapper for upstream `filters::FilterChain<grid_map::GridMap>`.

The grid map packages depend on the generic `filter_component_base` and
`filter_component_synchronizer` packages from the repository root. The editor
and factory discover grid map components through the generic `<filter_component>`
package metadata tag.
