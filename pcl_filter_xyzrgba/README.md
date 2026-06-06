# pcl_filter_xyzrgba

`pcl_filter_xyzrgba` instantiates the reusable filters from
`pcl_filter_components` for the concrete `pcl::PointXYZRGBA` point type. It
exports the `PointXYZRGBA` logical cloud type, editor discovery metadata, and
the loadable component classes for the `PointXYZRGBA` filter set.

Registered classes:

- `pcl_filter_xyzrgba::VoxelGridXYZRGBAComponent`
- `pcl_filter_xyzrgba::PassThroughXYZRGBAComponent`
- `pcl_filter_xyzrgba::CropBoxXYZRGBAComponent`
- `pcl_filter_xyzrgba::PointCloudMergerXYZRGBAComponent`

The filters in this package have the same conceptual ports and parameters as the
other concrete point-type packages. Single-cloud filters use `cloud` input,
`cloud` output, and optional `indices` output; `PointCloudMergerXYZRGBA` uses
`input_1`, `input_2`, and `cloud`. Saved graph filter nodes identify this
package and one registered component class:

```yaml
package: pcl_filter_xyzrgba
component_class: pcl_filter_xyzrgba::VoxelGridXYZRGBAComponent
```
