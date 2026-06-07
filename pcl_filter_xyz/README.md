# pcl_filter_xyz

`pcl_filter_xyz` instantiates the reusable filters from
`pcl_filter_components` for the concrete `pcl::PointXYZ` point type. It exports
the `PointXYZ` logical cloud type, editor discovery metadata, and the loadable
component classes for the `PointXYZ` filter set.

Registered components are exported in the same groups as the source tree:

- Downsampling: `VoxelGridXYZ`, `ApproximateVoxelGridXYZ`,
  `UniformSamplingXYZ`, `RandomSampleXYZ`, `GridMinimumXYZ`.
- Outlier removal: `StatisticalOutlierRemovalXYZ`,
  `RadiusOutlierRemovalXYZ`, `ConditionalRemovalXYZ`.
- Selection: `PassThroughXYZ`, `ExtractIndicesXYZ`, `RemoveNaNXYZ`,
  `RemoveInfiniteXYZ`, `KeepOrganizedXYZ`.
- Spatial: `CropBoxXYZ`, `FrustumCullingXYZ`, `CropSphereXYZ`,
  `ProjectInliersXYZ`, `PlaneClipperXYZ`.
- Surface: `MedianFilterXYZ`, `LocalMaximumXYZ`, `BilateralFilterXYZ`,
  `VoxelGridCovarianceXYZ`, `MorphologicalFilterXYZ`,
  `MovingLeastSquaresXYZ`.
- Segmentation: `PlaneModelFilterXYZ`, `SACSegmentationExtractXYZ`,
  `EuclideanClusterExtractXYZ`.
- Multi input: `PointCloudMergerXYZ`, `PointCloudConcatenateXYZ`,
  `PointCloudSubtractXYZ`, `PointCloudDifferenceXYZ`.

The filters in this package have the same conceptual ports and parameters as the
other concrete point-type packages. Single-cloud filters use `cloud` input,
`cloud` and `orig_cloud` outputs; `PointCloudMergerXYZ` uses
`input_1`, `input_2`, `cloud`, `orig_input_1`, and `orig_input_2`.
Saved graph filter nodes identify this package and one registered component class:

```yaml
package: pcl_filter_xyz
component_class: pcl_filter_xyz::VoxelGridXYZComponent
```
