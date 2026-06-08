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
- [ROS Filters Chains](#ros-filters-chains)
- [Architecture](#architecture)
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
- [filter_component_base](filter_component_base/README.md): lifecycle component base classes
  and descriptor helpers for declaring ports, parameters, QoS, and sync.
- [filter_component_factory](filter_component_factory/README.md): saved graph interpreter
  that loads filter nodes and binds their ports to topics.
- [filter_component_editor](filter_component_editor/README.md): rqt graph editor for
  pipeline authoring and validation.
- [pcl](pcl/README.md): PCL-specific algorithms, type adapters, point-type
  component packages, and PCL validation tests.

## ROS Filters Chains

The framework can also host upstream [`ros/filters`](https://github.com/ros/filters)
`filters::FilterChain<T>` pipelines as regular lifecycle filter components.
Use `pcl_filter_components_filter_chain` for the PCL chain components and
configure the chain parameters under the component's `filter_chain` prefix.

## Architecture

The root packages define the generic framework. Concrete component families live
in their own package groups and export discovery metadata through the generic
`<filter_component>` package tag. The current repository includes a PCL
component family under [`pcl/`](pcl/README.md).

```text
filter_component_synchronizer
  -> filter_component_base
    -> component family packages
      -> filter_component_factory
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
