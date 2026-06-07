# pcl_filter_xyzi

`pcl_filter_xyzi` instantiates the reusable filters from
`pcl_filter_components` for the concrete `pcl::PointXYZI` point type. It exports
the `PointXYZI` logical cloud type, editor discovery metadata, and the loadable
component classes for the `PointXYZI` filter set.

Registered components are exported in the same groups as the source tree:

- Downsampling: `VoxelGridXYZI`, `ApproximateVoxelGridXYZI`,
  `UniformSamplingXYZI`, `RandomSampleXYZI`, `GridMinimumXYZI`.
- Outlier removal: `StatisticalOutlierRemovalXYZI`,
  `RadiusOutlierRemovalXYZI`, `ConditionalRemovalXYZI`.
- Selection: `PassThroughXYZI`, `ExtractIndicesXYZI`, `RemoveNaNXYZI`,
  `RemoveInfiniteXYZI`, `KeepOrganizedXYZI`.
- Spatial: `CropBoxXYZI`, `FrustumCullingXYZI`, `CropSphereXYZI`,
  `ProjectInliersXYZI`, `PlaneClipperXYZI`.
- Surface: `MedianFilterXYZI`, `LocalMaximumXYZI`, `BilateralFilterXYZI`,
  `VoxelGridCovarianceXYZI`, `MorphologicalFilterXYZI`,
  `MovingLeastSquaresXYZI`.
- Segmentation: `PlaneModelFilterXYZI`, `SACSegmentationExtractXYZI`,
  `EuclideanClusterExtractXYZI`.
- Multi input: `PointCloudMergerXYZI`, `PointCloudConcatenateXYZI`,
  `PointCloudSubtractXYZI`, `PointCloudDifferenceXYZI`.
- Intensity: `IntensityThresholdXYZI`, `IntensityRangeFilterXYZI`.

The filters in this package have the same conceptual ports and parameters as the
other concrete point-type packages. Single-cloud filters use `cloud` input,
`cloud` and `orig_cloud` outputs; `PointCloudMergerXYZI` uses
`input_1`, `input_2`, `cloud`, `orig_input_1`, and `orig_input_2`.
Saved graph filter nodes identify this package and one registered component class:

```yaml
package: pcl_filter_xyzi
component_class: pcl_filter_xyzi::VoxelGridXYZIComponent
```
