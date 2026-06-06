# pcl_filter_xyzi

`pcl_filter_xyzi` instantiates the reusable filters from
`pcl_filter_components` for the concrete `pcl::PointXYZI` point type. It exports
the `PointXYZI` logical cloud type, editor discovery metadata, and the loadable
component classes for the `PointXYZI` filter set.

Registered classes:

- `pcl_filter_xyzi::VoxelGridXYZIComponent`
- `pcl_filter_xyzi::PassThroughXYZIComponent`
- `pcl_filter_xyzi::CropBoxXYZIComponent`
- `pcl_filter_xyzi::PointCloudMergerXYZIComponent`

The filters in this package have the same conceptual ports and parameters as the
other concrete point-type packages. Single-cloud filters use `cloud` input,
`cloud` output, and optional `indices` output; `PointCloudMergerXYZI` uses
`input_1`, `input_2`, and `cloud`. Saved graph filter nodes identify this
package and one registered component class:

```yaml
package: pcl_filter_xyzi
component_class: pcl_filter_xyzi::VoxelGridXYZIComponent
```
