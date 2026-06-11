# filter_component_editor

![](doc/editor.png)

`filter_component_editor` provides the `Filter Component Pipeline Editor` rqt
plugin. It is a graph editor for YAML pipeline files. A background pipeline is
used for component parameter discovery and graph validation.

## Editing Model

The editor discovers logical stream types from package metadata and discovers
filter components from packages that export `<filter_component>` metadata. Users
build a graph by adding filter nodes, creating topic nodes, and connecting named
ports.

When a filter is added, the editor records its package, filter name, component
class, logical input type, logical output types, declared ports, and discovered
parameters. When a topic node is created, it becomes a graph endpoint or
intermediate binding for a ROS topic. Topic nodes are saved in YAML, but they
are not loaded as lifecycle components.

Connections are made between ports. For example, an input topic node connects
from `out` to a filter's `cloud` input, and the filter's `cloud` output connects
to an output topic node's `in` port. Port QoS, filter parameters, and multi-input
sync settings can be edited on the graph node and saved with the YAML.

Filters that declare `shm_keys` metadata also get a Shm tab. The tab shows each
key name, type, access mode, description, and editable remap value. Remaps
default to the key name and are saved as:

```yaml
shm:
  remappings:
    global_map: slam/global_map
```

The right side of the editor shows a Shared Memory Keys inventory grouped by the
effective remapped key. Dotted key names are shown as a tree, like parameter
groups, with only real shared-memory keys carrying a type. Entries turn red when
the same effective key is used with incompatible declared types. Clicking an
entry selects the filters that use that key.

The background pipeline is used for parameter discovery and graph validation.
Refreshing it applies the current graph to the background runtime. Saving YAML
produces the graph consumed by the factory.

The Fit control adjusts the view to the current graph layout.

## Visual Semantics

Blue edges are normal compatible connections. The logical point type and ROS
message type both match the connected ports.

Red edges are ROS-message-compatible logical mismatches. The editor allows these
only when both sides use the same ROS message type, but it warns that zero-copy
will not work. For example, a user can connect a `PointXYZ` topic to a
`PointXYZI` filter because both export `sensor_msgs/msg/PointCloud2`; the edge
is red because the logical point types differ.

## Conceptual Editing Example

A typical downsampling graph can be created as:

1. Add a `VoxelGridXYZI` filter node.
2. Create an input topic node named `/points/input` with logical type
   `PointXYZI`.
3. Create an output topic node named `/points/output` with logical type
   `PointXYZI`.
4. Connect `/points/input` port `out` to `VoxelGridXYZI_1` port `cloud`.
5. Connect `VoxelGridXYZI_1` port `cloud` to `/points/output` port `in`.
6. Set `filter.leaf_size_x`, `filter.leaf_size_y`, and `filter.leaf_size_z`.
7. Save the graph as YAML.

The resulting YAML describes topic nodes, the `VoxelGridXYZI` filter node, the
two `cloud` edges, the edited leaf-size parameters, and any port QoS settings.
