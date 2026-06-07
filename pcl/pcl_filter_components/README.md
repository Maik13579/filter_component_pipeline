# pcl_filter_components

`pcl_filter_components` contains the reusable PCL algorithms and ROS 2 component
templates used by the concrete point-type packages. It does not register
loadable components by itself; packages such as `pcl_filter_components_xyzi` and
`pcl_filter_components_xyzrgb` instantiate these templates for specific PCL
point types.

Namespaces:

- `pcl_filter_components::filters`: algorithm templates that operate on PCL
  clouds and indices.
- `pcl_filter_components::ros`: lifecycle component templates that declare
  ports, parameters, QoS settings, and typed adapters.

## Table of Contents

- [Filter Templates](#filter-templates)
  - [Downsampling](#downsampling)
  - [Outlier Removal](#outlier-removal)
  - [Selection](#selection)
  - [Spatial](#spatial)
  - [Surface](#surface)
  - [Segmentation](#segmentation)
  - [Multi Input](#multi-input)
  - [Color](#color)
  - [Intensity](#intensity)
- [Ports](#ports)
- [Conceptual Node Example](#conceptual-node-example)

## Filter Templates

The generic filter headers are grouped under
`include/pcl_filter_components/filters/<group>/`. ROS wrapper templates live
under `include/pcl_filter_components/ros/<group>/` or `ros/common/` when the
same wrapper pattern applies to many groups.

### Downsampling

| Template | Header | Description |
| --- | --- | --- |
| `VoxelGridFilter` | `filters/voxel_grid_filter.hpp` | PCL voxel grid downsampling with x/y/z leaf sizes. |
| `ApproximateVoxelGridFilter` | `filters/downsampling/approximate_voxel_grid_filter.hpp` | Approximate voxel-grid downsampling for faster large-cloud reduction. |
| `UniformSamplingFilter` | `filters/downsampling/uniform_sampling_filter.hpp` | Selects representative points separated by a configured radius. |
| `RandomSampleFilter` | `filters/downsampling/random_sample_filter.hpp` | Selects a configured number of random input points. |
| `GridMinimumFilter` | `filters/downsampling/grid_minimum_filter.hpp` | Keeps the minimum z point per 2D grid cell. |

### Outlier Removal

| Template | Header | Description |
| --- | --- | --- |
| `StatisticalOutlierRemovalFilter` | `filters/outlier_removal/statistical_outlier_removal_filter.hpp` | Removes points outside neighbor-distance statistics. |
| `RadiusOutlierRemovalFilter` | `filters/outlier_removal/radius_outlier_removal_filter.hpp` | Removes points without enough neighbors inside a radius. |
| `ConditionalRemovalFilter` | `filters/outlier_removal/conditional_removal_filter.hpp` | Keeps or rejects points using a configured field range. |

### Selection

| Template | Header | Description |
| --- | --- | --- |
| `PassThroughFilter` | `filters/passthrough_filter.hpp` | Keeps or rejects points inside a field range. |
| `ExtractIndicesFilter` | `filters/selection/extract_indices_filter.hpp` | Selects a configured index range. |
| `RemoveNaNFilter` | `filters/selection/remove_nan_filter.hpp` | Removes points with NaN coordinates. |
| `RemoveInfiniteFilter` | `filters/selection/remove_infinite_filter.hpp` | Removes points with infinite coordinates. |
| `KeepOrganizedFilter` | `filters/selection/keep_organized_filter.hpp` | Preserves organized-cloud shape while marking rejected points. |

### Spatial

| Template | Header | Description |
| --- | --- | --- |
| `CropBoxFilter` | `filters/crop_box_filter.hpp` | Keeps or rejects points inside an axis-aligned 3D box. |
| `FrustumCullingFilter` | `filters/spatial/frustum_culling_filter.hpp` | Keeps points inside a camera frustum. |
| `CropSphereFilter` | `filters/spatial/crop_sphere_filter.hpp` | Keeps or rejects points inside a sphere. |
| `ProjectInliersFilter` | `filters/spatial/project_inliers_filter.hpp` | Projects points onto a plane model. |
| `PlaneClipperFilter` | `filters/spatial/plane_clipper_filter.hpp` | Keeps or rejects points by signed distance to a plane. |

### Surface

| Template | Header | Description |
| --- | --- | --- |
| `MedianFilter` | `filters/surface/median_filter.hpp` | Smooths organized depth data with a median window. |
| `LocalMaximumFilter` | `filters/surface/local_maximum_filter.hpp` | Keeps local maximum points within a radius. |
| `BilateralFilter` | `filters/surface/bilateral_filter.hpp` | Applies edge-preserving smoothing. |
| `VoxelGridCovarianceFilter` | `filters/surface/voxel_grid_covariance_filter.hpp` | Builds covariance voxels and emits representative points. |
| `MorphologicalFilter` | `filters/surface/morphological_filter.hpp` | Applies PCL morphology operations over a grid. |
| `MovingLeastSquaresFilter` | `filters/surface/moving_least_squares_filter.hpp` | Smooths and resamples surfaces using MLS. |

### Segmentation

| Template | Header | Description |
| --- | --- | --- |
| `PlaneModelFilter` | `filters/segmentation/plane_model_filter.hpp` | Filters points by distance to a configured plane equation. |
| `SACSegmentationExtractFilter` | `filters/segmentation/sac_segmentation_extract_filter.hpp` | Fits a plane with SAC/RANSAC and extracts inliers. |
| `EuclideanClusterExtractFilter` | `filters/segmentation/euclidean_cluster_extract_filter.hpp` | Extracts one sorted Euclidean cluster. |

### Multi Input

| Template | Header | Description |
| --- | --- | --- |
| `PointCloudMerger` | `ros/point_cloud_merger_component.hpp` | Existing two-cloud concatenation component. |
| `PointCloudConcatenate` | `ros/point_cloud_merger_component.hpp` | Concatenation export using the merger component behavior. |
| `PointCloudSubtractFilter` | `filters/multi_input/point_cloud_subtract_filter.hpp` | Removes points from the first cloud when a nearby point exists in the second. |
| `PointCloudDifferenceFilter` | `filters/multi_input/point_cloud_difference_filter.hpp` | Emits the symmetric difference between two clouds. |

### Color

| Template | Header | Description |
| --- | --- | --- |
| `ColorThresholdFilter` | `filters/color/color_threshold_filter.hpp` | Keeps or rejects RGB points inside channel ranges. |
| `RGBRangeFilter` | `filters/color/rgb_range_filter.hpp` | RGB range filter alias using channel-range behavior. |
| `RGBAAlphaFilter` | `filters/color/rgba_alpha_filter.hpp` | Keeps or rejects RGBA points by alpha range. |

### Intensity

| Template | Header | Description |
| --- | --- | --- |
| `IntensityThresholdFilter` | `filters/intensity/intensity_threshold_filter.hpp` | Keeps or rejects XYZI points by intensity range. |
| `IntensityRangeFilter` | `filters/intensity/intensity_range_filter.hpp` | Intensity range filter alias using intensity-range behavior. |

## Ports

| Port | Direction | Meaning |
| --- | --- | --- |
| `cloud` | input | Point cloud consumed by single-cloud filters. |
| `cloud` | output | Filtered or merged point cloud. |
| `orig_cloud` | output | Original single-cloud filter input. |
| `input_1` | input | First merger cloud input. |
| `input_2` | input | Second merger cloud input. |
| `orig_input_1` | output | Original first multi-input cloud. |
| `orig_input_2` | output | Original second multi-input cloud. |

Single-cloud filters expose one `cloud` input and `cloud`/`orig_cloud` outputs.
Merger filters expose `input_1`, `input_2`, `cloud`, `orig_input_1`, and
`orig_input_2`.

## Conceptual Node Example

A `VoxelGridXYZI` filter node has one `PointXYZI` cloud input, one filtered
`PointXYZI` cloud output, and one original-input `PointXYZI` output:

```yaml
type: filter
name: VoxelGridXYZI_1
package: pcl_filter_components_xyzi
filter: VoxelGridXYZI
component_class: pcl_filter_components_xyzi::VoxelGridXYZIComponent
input_type: PointXYZI
output_type: PointXYZI
parameters:
  filter.leaf_size_x: 0.05
  filter.leaf_size_y: 0.05
  filter.leaf_size_z: 0.05
inputs:
  cloud:
    reliability: best_effort
outputs:
  cloud:
    reliability: best_effort
  orig_cloud:
    reliability: reliable
```

Edges in a graph connect topic nodes or other filters to these named ports. The
factory later turns those edges into parameters such as `inputs.cloud.topic` and
`outputs.cloud.topic`.
