# pcl_filter_components

`pcl_filter_components` provides reusable, templated, header-only PCL filters for
`pcl::PointCloud<PointT>` and exposes `pcl::PointXYZI` variants as ROS 2
lifecycle components.

The ROS wire type stays `sensor_msgs/msg/PointCloud2`. Internally, the
components use an `rclcpp::TypeAdapter` to receive and publish stamped, typed
PCL clouds while preserving the original message header.

The components use `std::unique_ptr` subscription callbacks and publishing for
efficient intra-process communication. True zero-copy behavior only applies when
the filters are composed into the same process with intra-process communication
enabled; inter-process communication still serializes as `sensor_msgs/msg/PointCloud2`.

## Provided Components

The package currently exports these ROS 2 components:

- `pcl_filter_components::VoxelGridXYZIComponent`
- `pcl_filter_components::PassThroughXYZIComponent`
- `pcl_filter_components::CropBoxXYZIComponent`
- `pcl_filter_components::PointCloudMergerXYZIComponent`

The filtering components consume `PointXYZI` and publish `PointXYZI` or
`PointIndices`. The merger component consumes two `PointXYZI` inputs and
publishes one merged `PointXYZI` cloud.

## Component Discovery Tags

The visual editor discovers filters and available point types from package
exports. A package that provides a filter should export one
`pcl_filter_component` tag per filter:

```xml
<export>
  <pcl_filter_component
    filter="VoxelGridXYZI"
    input="PointXYZI"
    output="PointXYZI,PointIndices"/>
  <pcl_filter_component
    filter="PointCloudMergerXYZI"
    input="PointXYZI,PointXYZI"
    output="PointXYZI"/>
</export>
```

The `filter` value is the public filter name. The editor derives the package from
the package that exports the tag and expects the component class to be registered
as:

```text
<package_name>::<filter>Component
```

The editor cross-checks this class against the normal `rclcpp_components` index,
so the component must also be registered with `RCLCPP_COMPONENTS_REGISTER_NODE`.

Point and adapter types are exported with the same tag name using the `type`
attribute:

```xml
<export>
  <pcl_filter_component
    type="PointXYZI"
    type_adapter="pcl_filter_components::ros::PclCloudAdapterPointXYZI"
    message_type="sensor_msgs/msg/PointCloud2"/>
  <pcl_filter_component
    type="PointIndices"
    type_adapter="pcl_filter_components::ros::PclIndicesAdapter"
    message_type="pcl_msgs/msg/PointIndices"/>
</export>
```

These type exports let the editor know which input, output, and topic types are
valid. Multiple PCL-side types may map to the same ROS wire message, so the
editor uses these explicit type names instead of only inspecting ROS message
types.

## rqt Pipeline Editor

The package installs an `rqt` plugin named `PCL Pipeline Editor`.

```bash
source install/setup.bash
rqt --force-discover
```

The editor can also be checked from the command line:

```bash
source install/setup.bash
rqt --force-discover --list-plugins | grep -i pipeline
```

![PCL pipeline editor](doc/editor.png)

The editor creates a graph from two visible element types:

- Filter nodes are discovered component filters.
- Topic nodes are diamond markers that represent real ROS topics and carry QoS.
  External input and output topics are represented by topic nodes at the ends of
  the graph.

Connections are made by dragging from an output sphere to an input sphere. Arrows
are blue by default and orange when selected. Topic nodes use a diamond marker so
they are visually distinct from filter nodes. Top-down ports are the default
layout, and the selected layout is saved in the YAML file.

You can wire the graph in three ways:

- Drag from one filter to another filter. The editor creates a topic node between
  them automatically.
- Drag from a filter to an existing compatible topic, or from a topic to a
  compatible filter.
- Double-click a filter input or output sphere. The editor creates a compatible
  topic node next to that port and connects it.

When a filter has more than one possible output type, the editor keeps one output
sphere. It chooses the correct output port from the target topic/filter type. If
the type is ambiguous, it prompts for the output to use. For the current filters,
the cloud output is `PointXYZI` and the indices output is `PointIndices`.

Filters with repeated input types, such as `PointCloudMergerXYZI`, also keep one
input sphere. When a connection is ambiguous, the editor asks which input port
to use.

Double-click behavior:

- Filter node: opens a tabbed editor with General and Parameters tabs. A Sync tab
  is shown only when the filter metadata declares multiple inputs.
- Topic node: opens topic and QoS settings. `reliability` and `history` use drop
  downs; `depth` remains a numeric value.
- Arrow: starts rewiring mode so the arrow can be moved to another compatible
  topic.

The parameter tab uses the same parameter names as the ROS components and shows
short hints for the known filter parameters. The topic/type fields are not used
to override type compatibility; type compatibility comes from filter exports and
graph connections.

## Graph YAML

Pipelines are saved as YAML. The saved graph contains editor layout data,
component identities, parameters, QoS, sync settings, and graph edges.

Important identity rules:

- Filter nodes use `name` as the graph identity.
- Topic nodes use the ROS topic name as the graph identity.
- Topic nodes do not have a separate `id`; the topic name is unique.
- Edges reference the visible filter name or topic name.

Example:

```yaml
version: 1
editor:
  orientation: top_down
nodes:
  - type: topic
    topic: /points/input
    input_type: PointXYZI
    output_type: PointXYZI
    position: {x: 280.0, y: 0.0}
  - type: filter
    name: VoxelGridXYZI_1
    package: pcl_filter_components
    filter: VoxelGridXYZI
    input_type: PointXYZI
    output_type: PointXYZI,PointIndices
    parameters:
      filter.leaf_size_x: 0.1
      filter.leaf_size_y: 0.1
      filter.leaf_size_z: 0.1
      queue_size: 5
    position: {x: 280.0, y: 160.0}
  - type: topic
    topic: ~/VoxelGridXYZI_1-CropBoxXYZI_1
    input_type: PointXYZI
    output_type: PointXYZI
    qos:
      reliability: best_effort
      history: keep_last
      depth: 5
    position: {x: 280.0, y: 320.0}
edges:
  - from: {node: /points/input, port: out}
    to: {node: VoxelGridXYZI_1, port: in}
  - from: {node: VoxelGridXYZI_1, port: out}
    to: {node: ~/VoxelGridXYZI_1-CropBoxXYZI_1, port: in}
```

When the editor auto-creates an intermediate topic for a direct filter-to-filter
connection, the default topic name is:

```text
~/[from_filter_name]-[to_filter_name]
```

The editor validates graph edges by point type. A `PointIndices` edge cannot be
connected to a `PointXYZI` topic unless the types match through the selected
port.

An installed example pipeline is available at:

```text
share/pcl_filter_components/config/example_pipeline.yaml
```

## Factory Runtime

`pcl_pipeline_factory` loads a saved YAML pipeline and composes the filter
components in-process with intra-process communication enabled.

```bash
source install/setup.bash
ros2 run pcl_filter_components pcl_pipeline_factory \
  --ros-args -p pipeline_file:=/path/to/pipeline.yaml
```

By default, the factory uses a single-threaded executor. Set
`executor_threads` to a value greater than `1` to run the composed pipeline with
a multi-threaded executor:

```bash
ros2 run pcl_filter_components pcl_pipeline_factory \
  --ros-args \
  -p pipeline_file:=/path/to/pipeline.yaml \
  -p executor_threads:=4
```

Topic graph nodes are not runtime components. They become topic bindings and
remaps for the composed filter components. Filter nodes are loaded through
`rclcpp_components`, configured, activated, deactivated, and cleaned up through
the lifecycle interface.

The factory applies:

- filter parameters from the YAML graph
- input and output topic bindings from graph edges
- topic nodes as shared intermediate topics
- component loading in the same process with intra-process communication enabled

## Development Notes

Parameter descriptors are declared by the filter components and tested so the
editor has stable parameter names, defaults, descriptions, and numeric ranges to
build parameter forms from. Current tests cover:

- parameter descriptors for the PointXYZI filters
- filter and type-adapter discovery from `package.xml`
- Python and C++ graph save/load validation
- multi-port outputs such as `indices`
- factory lifecycle loading of generated and example YAML pipelines
