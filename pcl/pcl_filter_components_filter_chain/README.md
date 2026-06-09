# pcl_filter_components_filter_chain

ROS 2 components that expose the upstream [`ros/filters`](https://github.com/ros/filters) `filters::FilterChain<T>` API through the filter component lifecycle framework.

Each component uses typed `std::unique_ptr` messages at the ROS subscription and publication boundary. The chain itself uses the official value-based `filters` API, so processing copies values internally.

## Using This In A Pipeline

Choose one of the exported `RosFilterChain...` components in the editor or factory, then connect it like any other single-input filter component.

Point cloud chains use a `cloud` input port, a filtered `cloud` output port, and an `orig_input` output port with the unchanged input. `PointIndices` chains use `indices` for the input and filtered output, plus `orig_input` for the unchanged input. The ROS boundary still uses the package type adapters, while the filter chain loads plugins compiled for the exact `filters::FilterBase<T>` data type listed by the component.

Example factory node entry:

```yaml
package: pcl_filter_components_filter_chain
filter: RosFilterChainXYZI
component_class: pcl_filter_components_filter_chain::RosFilterChainXYZIComponent
input_ports: cloud:PointXYZI
output_ports: cloud:PointXYZI,orig_input:PointXYZI
```

Example chain parameters:

```yaml
filters:
  filter1:
    name: first_filter
    type: package/FilterPlugin
    params: filters.filter1.params
```

Filter-specific parameters are read under the prefix named by `filterN.params`. Only plugins exported for the exact `filters::FilterBase<T>` base class type can be loaded.
