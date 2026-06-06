# PCL Filter Pipeline

This repository contains ROS 2 packages for building point cloud filter
pipelines from reusable PCL operations. A pipeline is a graph of topic nodes and
filter nodes connected by typed ports.

A filter node is a loadable ROS 2 lifecycle component. Each component wraps one
PCL operation, declares its input and output ports, exposes filter parameters,
and uses typed adapters to move data between ROS messages and PCL data
structures. Topic nodes are graph bindings: they name ROS topics that enter,
leave, or connect parts of the graph. Topic nodes are not loaded as filter
components.

## Pipeline Model

The graph has three main concepts:

- Filter components: lifecycle components such as `VoxelGridXYZI` or
  `PassThroughXYZ` that process point cloud data.
- Ports: named inputs and outputs on a filter component. Single-cloud filters
  use the `cloud` input and the `cloud` and `indices` outputs. Merger filters
  use `input_1`, `input_2`, and `cloud`.
- Topic nodes: endpoints and intermediate bindings that assign ROS topic names
  to graph edges.

Logical point types describe the data type flowing through a port. Cloud types
such as `PointXYZ`, `PointXYZI`, `PointXYZRGB`, and `PointXYZRGBA` all map to
`sensor_msgs/msg/PointCloud2`. The `PointIndices` logical type maps to
`pcl_msgs/msg/PointIndices`.

Because several logical cloud types share the same ROS message type, the editor
can show both strict logical compatibility and looser ROS-message compatibility.
For example, a `PointXYZ` topic and a `PointXYZI` filter both use
`sensor_msgs/msg/PointCloud2`, but their logical point types differ.

## Packages

- [pcl_filter_type_adapters](pcl_filter_type_adapters/README.md): PCL cloud and
  point-indices type adapters plus logical type discovery metadata.
- [pcl_filter_synchronizer](pcl_filter_synchronizer/README.md): header-only
  unique-pointer synchronizer for filters with named input ports.
- [pcl_filter_base](pcl_filter_base/README.md): lifecycle component base classes
  and descriptor helpers for declaring ports, parameters, QoS, and sync.
- [pcl_filter_components](pcl_filter_components/README.md): generic PCL filter
  algorithms and reusable component templates.
- [pcl_filter_xyz](pcl_filter_xyz/README.md): `PointXYZ` aliases, filter
  metadata, and registered components.
- [pcl_filter_xyzi](pcl_filter_xyzi/README.md): `PointXYZI` aliases, filter
  metadata, and registered components.
- [pcl_filter_xyzrgb](pcl_filter_xyzrgb/README.md): `PointXYZRGB` aliases,
  filter metadata, and registered components.
- [pcl_filter_xyzrgba](pcl_filter_xyzrgba/README.md): `PointXYZRGBA` aliases,
  filter metadata, and registered components.
- [pcl_filter_factory](pcl_filter_factory/README.md): saved graph interpreter
  that loads filter nodes and binds their ports to topics.
- [pcl_filter_editor](pcl_filter_editor/README.md): rqt graph editor with live
  background pipeline validation.
- [pcl_filter_tests](pcl_filter_tests/README.md): repository validation coverage
  for discovery, registration, and example graph parsing.

## Architecture

The generic packages define common infrastructure. Concrete point-type packages
instantiate those templates for one PCL point type and export metadata for
factory and editor discovery.

```text
pcl_filter_type_adapters
  -> pcl_filter_synchronizer
    -> pcl_filter_base
    -> pcl_filter_components
      -> pcl_filter_xyz / pcl_filter_xyzi / pcl_filter_xyzrgb / pcl_filter_xyzrgba
        -> pcl_filter_factory
          -> pcl_filter_tests

pcl_filter_editor -> pcl_filter_tests
```

Concrete packages export filters with package metadata similar to:

```xml
<pcl_filter_component
  filter="VoxelGridXYZI"
  input="PointXYZI"
  output="PointXYZI,PointIndices"/>
```

They also export logical cloud aliases:

```xml
<pcl_filter_component
  type="PointXYZI"
  type_adapter="pcl_filter_xyzi::ros::PclCloudAdapterPointXYZI"
  message_type="sensor_msgs/msg/PointCloud2"/>
```

## Graph YAML

Saved YAML is the portable pipeline description. Filter nodes record component
identity, filter parameters, port QoS, optional synchronization settings, and
editor layout. Edges connect named ports. Topic nodes provide the ROS topic
bindings used by those edges.

This minimal graph represents `/points/input -> VoxelGridXYZI_1 ->
/points/output`:

```yaml
nodes:
  - type: topic
    name: /points/input
    topic_type: PointXYZI
  - type: filter
    name: VoxelGridXYZI_1
    package: pcl_filter_xyzi
    filter: VoxelGridXYZI
    component_class: pcl_filter_xyzi::VoxelGridXYZIComponent
    input_type: PointXYZI
    output_type: PointXYZI,PointIndices
    parameters:
      filter.leaf_size_x: 0.1
      filter.leaf_size_y: 0.1
      filter.leaf_size_z: 0.1
      filter.output_indices: false
  - type: topic
    name: /points/output
    topic_type: PointXYZI
edges:
  - from: {node: /points/input, port: out}
    to: {node: VoxelGridXYZI_1, port: cloud}
  - from: {node: VoxelGridXYZI_1, port: cloud}
    to: {node: /points/output, port: in}
```

When the factory reads this graph, it loads only `VoxelGridXYZI_1` as a
component. The topic nodes become the values for component port parameters such
as `inputs.cloud.topic` and `outputs.cloud.topic`.
