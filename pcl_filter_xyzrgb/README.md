# pcl_filter_xyzrgb

`pcl_filter_xyzrgb` instantiates the reusable filters from
`pcl_filter_components` for the concrete `pcl::PointXYZRGB` point type. It
exports the `PointXYZRGB` logical cloud type, editor discovery metadata, and the
loadable component classes for the `PointXYZRGB` filter set.

Registered components are exported in the same groups as the source tree:

- Downsampling: `VoxelGridXYZRGB`, `ApproximateVoxelGridXYZRGB`,
  `UniformSamplingXYZRGB`, `RandomSampleXYZRGB`, `GridMinimumXYZRGB`.
- Outlier removal: `StatisticalOutlierRemovalXYZRGB`,
  `RadiusOutlierRemovalXYZRGB`, `ConditionalRemovalXYZRGB`.
- Selection: `PassThroughXYZRGB`, `ExtractIndicesXYZRGB`,
  `RemoveNaNXYZRGB`, `RemoveInfiniteXYZRGB`, `KeepOrganizedXYZRGB`.
- Spatial: `CropBoxXYZRGB`, `FrustumCullingXYZRGB`, `CropSphereXYZRGB`,
  `ProjectInliersXYZRGB`, `PlaneClipperXYZRGB`.
- Surface: `MedianFilterXYZRGB`, `LocalMaximumXYZRGB`,
  `BilateralFilterXYZRGB`, `VoxelGridCovarianceXYZRGB`,
  `MorphologicalFilterXYZRGB`, `MovingLeastSquaresXYZRGB`.
- Segmentation: `PlaneModelFilterXYZRGB`, `SACSegmentationExtractXYZRGB`,
  `EuclideanClusterExtractXYZRGB`.
- Multi input: `PointCloudMergerXYZRGB`, `PointCloudConcatenateXYZRGB`,
  `PointCloudSubtractXYZRGB`, `PointCloudDifferenceXYZRGB`.
- Color: `ColorThresholdXYZRGB`, `RGBRangeFilterXYZRGB`.

The filters in this package have the same conceptual ports and parameters as the
other concrete point-type packages. Single-cloud filters use `cloud` input,
`cloud` and `orig_cloud` outputs; `PointCloudMergerXYZRGB` uses
`input_1`, `input_2`, `cloud`, `orig_input_1`, and `orig_input_2`.
Saved graph filter nodes identify this package and one registered component class:

```yaml
package: pcl_filter_xyzrgb
component_class: pcl_filter_xyzrgb::VoxelGridXYZRGBComponent
```
