# filter_component_editor

![](doc/editor.png)

`filter_component_editor` provides the `PCL Pipeline Editor` rqt plugin. It is a graph
editor for saved YAML pipelines, with a live pipeline running in the background
to discover component parameters and validate the graph while it is edited.

## Editing Model

The editor discovers logical point types from package metadata and discovers
filter components from the concrete point-type packages. Users build a graph by
adding filter nodes, creating topic nodes, and connecting named ports.

When a filter is added, the editor records its package, filter name, component
class, logical input type, logical output types, default ports, and discovered
parameters. When a topic node is created, it becomes a graph endpoint or
intermediate binding for a ROS topic. Topic nodes are saved in YAML, but they
are not loaded as lifecycle components.

Connections are made between ports. For example, an input topic node connects
from `out` to a filter's `cloud` input, and the filter's `cloud` output connects
to an output topic node's `in` port. Port QoS, filter parameters, and multi-input
sync settings can be edited on the graph node and saved with the YAML.

The live background pipeline is used for parameter discovery and immediate graph
validation. Refreshing the live pipeline applies the current graph to that
background runtime. Saving YAML still produces the portable pipeline
description; the saved graph is the artifact that the factory interprets later.

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
