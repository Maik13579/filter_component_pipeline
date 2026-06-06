# pcl_filter_xyzrgb

`pcl_filter_xyzrgb` instantiates the reusable filters from
`pcl_filter_components` for the concrete `pcl::PointXYZRGB` point type. It
exports the `PointXYZRGB` logical cloud type, editor discovery metadata, and the
loadable component classes for the `PointXYZRGB` filter set.

Registered classes:

- `pcl_filter_xyzrgb::VoxelGridXYZRGBComponent`
- `pcl_filter_xyzrgb::PassThroughXYZRGBComponent`
- `pcl_filter_xyzrgb::CropBoxXYZRGBComponent`
- `pcl_filter_xyzrgb::PointCloudMergerXYZRGBComponent`

The filters in this package have the same conceptual ports and parameters as the
other concrete point-type packages. Single-cloud filters use `cloud` input,
`cloud` output, and optional `indices` output; `PointCloudMergerXYZRGB` uses
`input_1`, `input_2`, and `cloud`. Saved graph filter nodes identify this
package and one registered component class:

```yaml
package: pcl_filter_xyzrgb
component_class: pcl_filter_xyzrgb::VoxelGridXYZRGBComponent
```
