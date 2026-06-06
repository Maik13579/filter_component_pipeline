# pcl_filter_components

`pcl_filter_components` contains the generic PCL algorithms and reusable ROS 2
component templates used by the concrete point-type packages. It does not
register loadable components by itself; packages such as `pcl_filter_xyzi` and
`pcl_filter_xyzrgb` instantiate these templates for specific PCL point types.

Namespaces:

- `pcl_filter_components::filters`: algorithm templates that operate on PCL
  clouds and indices.
- `pcl_filter_components::ros`: lifecycle component templates that declare
  ports, parameters, QoS settings, and typed adapters.

## Filter Templates

`VoxelGrid` downsamples a cloud into leaf-size voxels. Its key parameters are
`filter.leaf_size_x`, `filter.leaf_size_y`, `filter.leaf_size_z`, and
`filter.output_indices`.

`PassThrough` keeps or rejects points along one field and limit range. Its
parameters include the selected field, limits, invert behavior, and
`filter.output_indices`.

`CropBox` keeps or rejects points inside an axis-aligned box. Its parameters
describe minimum and maximum bounds, invert behavior, and
`filter.output_indices`.

`PointCloudMerger` combines two cloud inputs of the same logical point type into
one cloud output. It is a multi-input component and therefore uses the sync
settings declared by `pcl_filter_base`.

## Ports

| Port | Direction | Meaning |
| --- | --- | --- |
| `cloud` | input | Point cloud consumed by single-cloud filters. |
| `cloud` | output | Filtered or merged point cloud. |
| `indices` | output | Point indices for filters that can publish selected indices. |
| `input_1` | input | First merger cloud input. |
| `input_2` | input | Second merger cloud input. |

Single-cloud filters expose one `cloud` input and two possible outputs:
`cloud` and `indices`. Merger filters expose `input_1`, `input_2`, and one
`cloud` output.

`filter.output_indices` controls which result a supporting single-cloud filter
publishes from its processing step. When it is `false`, the component publishes
the filtered cloud on the `cloud` output. When it is `true`, the component
publishes selected-point `PointIndices` data on the `indices` output instead of
publishing a filtered cloud.

## Conceptual Node Example

A `VoxelGridXYZI` filter node has one `PointXYZI` cloud input, one `PointXYZI`
cloud output, and one `PointIndices` output:

```yaml
type: filter
name: VoxelGridXYZI_1
package: pcl_filter_xyzi
filter: VoxelGridXYZI
component_class: pcl_filter_xyzi::VoxelGridXYZIComponent
input_type: PointXYZI
output_type: PointXYZI,PointIndices
parameters:
  filter.leaf_size_x: 0.05
  filter.leaf_size_y: 0.05
  filter.leaf_size_z: 0.05
  filter.output_indices: false
inputs:
  cloud:
    reliability: best_effort
outputs:
  cloud:
    reliability: best_effort
  indices:
    reliability: reliable
```

Edges in a graph connect topic nodes or other filters to these named ports. The
factory later turns those edges into `inputs.cloud.topic`,
`outputs.cloud.topic`, and `outputs.indices.topic` parameters.
