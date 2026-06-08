# filter_component_factory

`filter_component_factory` interprets saved YAML pipeline graphs. It turns filter
nodes into loaded ROS 2 lifecycle components and turns topic nodes into topic
bindings for the connected component ports.

Saved YAML contains two kinds of nodes:

- Filter nodes: component identity, filter parameters, port QoS, optional sync
  settings, and editor layout.
- Topic nodes: named ROS topic endpoints or intermediate graph bindings.

Only filter nodes are loaded with `rclcpp_components`. Topic nodes are graph
structure; they provide the concrete topic names assigned to component ports.

## Edge Mapping

Edges describe how named ports are connected. For each incoming edge to a filter
node, the factory writes an `inputs.<port>.topic` parameter. For each outgoing
edge from a filter node, it writes an `outputs.<port>.topic` parameter.

For example:

```yaml
edges:
  - from: {node: /points/input, port: out, direction: output}
    to: {node: VoxelGridXYZI_1, port: cloud, direction: input}
  - from: {node: VoxelGridXYZI_1, port: cloud, direction: output}
    to: {node: /points/output, port: in, direction: input}
```

maps to these component parameters on `VoxelGridXYZI_1`:

```yaml
inputs.cloud.topic: /points/input
outputs.cloud.topic: /points/output
```

The saved graph may also contain per-port QoS maps and a filter node `sync` map.
The factory passes sync entries through as `sync.mode`, `sync.queue_size`, and
`sync.max_interval` parameters for components that declare more than one input
port. Removed `sync.policy` graphs are rejected during validation.

Legacy graph ports named `in` and `out` are normalized to the modern `cloud`
port for filter components. Topic-node ports such as `in` and `out` remain graph
endpoint labels.

Factory headers use the `filter_component_factory/...` include root and the
`filter_component_factory::pipeline` namespace.
