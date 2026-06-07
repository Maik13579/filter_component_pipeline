# pcl_filter_components_xyzrgba

`pcl_filter_components_xyzrgba` instantiates the reusable filters from
`pcl_filter_components` for the concrete `pcl::PointXYZRGBA` point type. It
exports the `PointXYZRGBA` logical cloud type, editor discovery metadata, and
the loadable component classes for the `PointXYZRGBA` filter set.

Registered components are exported in the same groups as the source tree:

- Downsampling: `VoxelGridXYZRGBA`, `ApproximateVoxelGridXYZRGBA`,
  `UniformSamplingXYZRGBA`, `RandomSampleXYZRGBA`, `GridMinimumXYZRGBA`.
- Outlier removal: `StatisticalOutlierRemovalXYZRGBA`,
  `RadiusOutlierRemovalXYZRGBA`, `ConditionalRemovalXYZRGBA`.
- Selection: `PassThroughXYZRGBA`, `ExtractIndicesXYZRGBA`,
  `RemoveNaNXYZRGBA`, `RemoveInfiniteXYZRGBA`, `KeepOrganizedXYZRGBA`.
- Spatial: `CropBoxXYZRGBA`, `FrustumCullingXYZRGBA`,
  `CropSphereXYZRGBA`, `ProjectInliersXYZRGBA`, `PlaneClipperXYZRGBA`.
- Surface: `MedianFilterXYZRGBA`, `LocalMaximumXYZRGBA`,
  `BilateralFilterXYZRGBA`, `VoxelGridCovarianceXYZRGBA`,
  `MorphologicalFilterXYZRGBA`, `MovingLeastSquaresXYZRGBA`.
- Segmentation: `PlaneModelFilterXYZRGBA`, `SACSegmentationExtractXYZRGBA`,
  `EuclideanClusterExtractXYZRGBA`.
- Multi input: `PointCloudMergerXYZRGBA`, `PointCloudConcatenateXYZRGBA`,
  `PointCloudSubtractXYZRGBA`, `PointCloudDifferenceXYZRGBA`.
- Color: `ColorThresholdXYZRGBA`, `RGBRangeFilterXYZRGBA`,
  `RGBAAlphaFilterXYZRGBA`.

The filters in this package have the same conceptual ports and parameters as the
other concrete point-type packages. Single-cloud filters use `cloud` input,
`cloud` and `orig_cloud` outputs; `PointCloudMergerXYZRGBA` uses
`input_1`, `input_2`, `cloud`, `orig_input_1`, and `orig_input_2`.
Saved graph filter nodes identify this package and one registered component class:

```yaml
package: pcl_filter_components_xyzrgba
component_class: pcl_filter_components_xyzrgba::VoxelGridXYZRGBAComponent
```
