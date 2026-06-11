# Filter Component Pipeline

This repository contains ROS 2 packages for building filter pipelines from
loadable components. A pipeline is a graph of filter nodes connected to ROS
topics through named, typed ports.

A filter node is a ROS 2 lifecycle component that performs one processing step.
It declares the streams it consumes and produces, exposes parameters for its
operation, and uses [type adapters](https://ros.org/reps/rep-2007.html) to
convert between ROS messages and the data representation used by the
implementation. Topic nodes are graph bindings: they name ROS topics that enter,
leave, or connect parts of the graph. Topic nodes are not loaded as components.

![Filter Component Pipeline Editor](filter_component_editor/doc/editor.png)

## Table of Contents

- [Pipeline Model](#pipeline-model)
- [Packages](#packages)
- [ROS Filter Chains](#ros-filter-chains)
- [Architecture](#architecture)
- [C++ In-Process Shared Memory](#c-in-process-shared-memory)
- [Python Support](#python-support)
  - [Python Component Container](#python-component-container)
- [Editor](#editor)
- [Graph YAML](#graph-yaml)

## Pipeline Model

The graph has three main concepts:

- Filter components: loadable lifecycle components that process one or more
  typed streams and may publish one or more typed results.
- Ports: named inputs and outputs declared by each component. Port names are
  part of the component contract and are used by graph edges, the editor, and
  the factory.
- Topic nodes: endpoints and intermediate bindings that assign ROS topic names
  to graph edges.

Logical types describe what kind of data flows through a port. Component
packages export those logical types and map them to ROS message types through
type adapters. The editor and factory use that metadata to discover compatible
filters, topic types, and component classes.

## Packages

- [filter_component_synchronizer](filter_component_synchronizer/README.md): header-only
  unique-pointer synchronizer for filters with named input ports.
- [filter_component_base](filter_component_base/README.md): lifecycle component base classes,
  descriptor types for declaring ports and shared-memory keys, and helpers for
  component parameters, QoS, and sync.
- [component_shm](https://github.com/Maik13579/component_shm): in-process shared-memory registry and
  per-component remapping views for C++ components.
- [filter_component_factory](filter_component_factory/README.md): saved graph interpreter
  that loads filter nodes and binds their ports to topics.
- [filter_component_synchronizer_py](filter_component_synchronizer_py/README.md): Python
  synchronizer for filters with named input ports.
- [filter_component_base_py](filter_component_base_py/README.md): Python lifecycle base
  classes, port helpers, adapters, and Python intra-process transport.
- [filter_component_factory_py](filter_component_factory_py/README.md): Python runtime
  factory that loads Python filter nodes from saved graph YAML.
- [filter_component_editor](filter_component_editor/README.md): rqt graph editor for
  pipeline authoring and validation.
- [pcl](pcl/README.md): PCL-specific algorithms, type adapters, point-type
  component packages, and PCL validation tests.
- [grid_map](grid_map/README.md): Grid Map type adapter and filter-chain
  wrapper packages for upstream `grid_map` filters.

## ROS Filter Chains

The framework can also host upstream [`ros/filters`](https://github.com/ros/filters)
`filters::FilterChain<T>` pipelines as regular lifecycle filter components.
Use `pcl_filter_components_filter_chain` for the PCL chain components or
`grid_map_components_filter_chain` for Grid Map chains.

![Filter Chain Editor Nodes](filter_component_editor/doc/filters.png)

## Architecture

The root packages define the generic framework. Concrete component families live
in their own package groups and export discovery metadata through the generic
`<filter_component>` package tag. The current repository includes component
families under [`pcl/`](pcl/README.md) and [`grid_map/`](grid_map/README.md).

```text
filter_component_synchronizer
  -> filter_component_base
    -> component family packages
      -> filter_component_factory
      -> filter_component_editor
```

Python packages mirror the same structure:

```text
filter_component_synchronizer_py
  -> filter_component_base_py
    -> Python filter packages
      -> filter_component_factory_py
      -> filter_component_editor
```

Component packages export filters with package metadata similar to:

```xml
<filter_component
  filter="ExampleFilter"
  input="ExampleType"
  output="ExampleType"/>
```

They can also export logical type aliases:

```xml
<filter_component
  type="ExampleType"
  type_adapter="example_package::ros::ExampleAdapter"
  message_type="example_interfaces/msg/Example"/>
```

## C++ In-Process Shared Memory

C++ filters can use in-process shared memory through the
[`component_shm`](https://github.com/Maik13579/component_shm) package. `filter_component_base`
lets filters declare typed shared-memory keys, exposes checked helper functions
for const and mutable access, and supports per-filter remapping from local key
names to process-wide shared keys.

Pipeline metadata and YAML can remap local filter keys such as `global_map` to
effective process keys such as `slam/global_map`. The editor shows declared
keys in a Shm tab and maintains a shared-memory key inventory for C++ filters.
Python runtime support does not use `component_shm` in this version.

## Python Support

Python support is intended for fast prototyping while preserving the same graph,
port, QoS, and lifecycle model as C++ filter components.

Python filters derive from `filter_component_base_py.FilterComponentBasePy`.
They declare named input and output ports, receive synchronized adapter objects,
and publish through the same port names used by graph edges. Python support includes a
`PointCloud2NumpyAdapter` backed by `sensor_msgs_py.point_cloud2`.

Python type adapters mimic the role of C++ `rclcpp` type adapters. Each adapter
declares a ROS message type, a Python custom type, and `from_ros()` / `to_ros()`
conversion functions. The Python framework uses the ROS type for normal
publishers and subscriptions, and uses the custom type inside filter code,
synchronization, and Python intra-process delivery. The current adapter set provides a
PointCloud2-to-structured-NumPy adapter.

Python filter nodes are identified in graph YAML by `python_module` and
`python_class`. No separate `implementation: python` field is required:

```yaml
- type: filter
  name: PythonCloudFilter_1
  python_module: my_filters.cloud_filters
  python_class: PythonCloudFilter
  input_ports: cloud:PointCloud2
  output_ports: cloud:PointCloud2
```

Run mixed pipelines by starting the C++ factory for C++ components and the
Python component container for Python components:

```bash
ros2 run filter_component_factory filter_pipeline_factory \
  --ros-args -p pipeline_file:=/path/to/pipeline.yaml

ros2 run filter_component_factory_py filter_pipeline_factory_py
```

The C++ factory loads C++ component nodes and ignores Python nodes. The Python
runtime starts a component-container-style node for Python filters.
Python-to-Python edges can use the context-scoped Python intra-process manager
when topic, adapter type, and QoS match exactly. C++-to-Python and
Python-to-C++ edges currently use normal ROS topics.

### Python Component Container

The Python runtime deliberately mimics the C++ component container interface. It
provides the same `composition_interfaces` services:
`_container/load_node`, `_container/unload_node`, and `_container/list_nodes`.
The rqt editor uses those services to load and unload Python filters in the
background the same way it handles C++ components.

For Python filters, `LoadNode.plugin_name` is the Python plugin identity in the
form `<module>:<class>`, for example
`my_filters.cloud_filters:PythonCloudFilter`. The loaded lifecycle node appears
in `ros2 node list`, and the container's `list_nodes` service reports the same
full node names and unique ids expected by the editor runtime.

The Python intra-process manager is designed to mimic the role of the C++
`rclcpp` intra-process manager for Python filters. Each Python filter registers
its ports with the manager for its `rclpy.Context`. The manager connects active
publishers and subscribers only when the resolved topic, adapter type, and exact
QoS settings match. Publishers still create normal ROS publishers; the manager
direct-delivers Python adapter objects to compatible Python subscribers and only
falls back to ROS publishing when additional ROS subscribers are present.

When direct delivery and ROS publishing both happen, the Python manager records
pending local ROS echoes and drops those echoes on the matching Python
subscription callbacks. This prevents Python subscribers from processing the
same local publication once through the direct path and once through ROS. The
C++ implementation can do this with publisher GID metadata; Jazzy `rclpy` does
not expose that GID in public callback metadata, so the Python version uses this
pending-echo strategy.

## Editor

`filter_component_editor` provides an rqt plugin for pipeline graph authoring.
It discovers installed filter components and logical stream types, supports
filter and topic node editing, connects named ports, edits parameters and QoS,
and writes the graph as YAML.

The editor uses a background pipeline for parameter discovery and graph
validation. `filter_component_factory` later reads the saved YAML to launch the
pipeline.

## Graph YAML

Saved YAML describes the pipeline. Filter nodes record component identity,
filter parameters, port QoS, optional synchronization settings, and editor
layout. Edges connect named ports. Topic nodes provide the ROS topic bindings
used by those edges.

```yaml
nodes:
  - type: topic
    name: /input
    topic_type: ExampleType
  - type: filter
    name: ExampleFilter_1
    package: example_filter_package
    filter: ExampleFilter
    component_class: example_filter_package::ExampleFilterComponent
    input_type: ExampleType
    output_type: ExampleType
    parameters:
      filter.example_parameter: 1.0
  - type: topic
    name: /output
    topic_type: ExampleType
edges:
  - from: {node: /input, port: out, direction: output}
    to: {node: ExampleFilter_1, port: input, direction: input}
  - from: {node: ExampleFilter_1, port: output, direction: output}
    to: {node: /output, port: in, direction: input}
```

When the factory reads a graph, it loads filter nodes as components and applies
the topic-node bindings to the corresponding component ports.
