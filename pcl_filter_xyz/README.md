# pcl_filter_xyz

`pcl_filter_xyz` instantiates the reusable filters from
`pcl_filter_components` for the concrete `pcl::PointXYZ` point type. It exports
the `PointXYZ` logical cloud type, editor discovery metadata, and the loadable
component classes for the `PointXYZ` filter set.

Registered classes:

- `pcl_filter_xyz::VoxelGridXYZComponent`
- `pcl_filter_xyz::PassThroughXYZComponent`
- `pcl_filter_xyz::CropBoxXYZComponent`
- `pcl_filter_xyz::PointCloudMergerXYZComponent`

The filters in this package have the same conceptual ports and parameters as the
other concrete point-type packages. Single-cloud filters use `cloud` input,
`cloud` output, and optional `indices` output; `PointCloudMergerXYZ` uses
`input_1`, `input_2`, and `cloud`. Saved graph filter nodes identify this
package and one registered component class:

```yaml
package: pcl_filter_xyz
component_class: pcl_filter_xyz::VoxelGridXYZComponent
```
