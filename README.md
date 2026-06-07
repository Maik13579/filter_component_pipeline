# PCL Filter Pipeline

This repository contains ROS 2 packages for building point cloud filter
pipelines from reusable PCL operations. A pipeline is a graph of topic nodes and
filter nodes connected by typed ports.

A filter node is a loadable ROS 2 lifecycle component. Each component wraps one
PCL operation, declares its input and output ports, exposes filter parameters,
and uses typed adapters to move data between ROS messages and PCL data
structures. Topic nodes are graph bindings: they name ROS topics that enter,
leave, or connect parts of the graph. Topic nodes are not loaded as filter
components.

## Table of Contents

- [Pipeline Model](#pipeline-model)
- [Packages](#packages)
- [Architecture](#architecture)
- [Filter Reference](#filter-reference)
  - [Downsampling](#downsampling)
  - [Outlier Removal](#outlier-removal)
  - [Selection](#selection)
  - [Spatial](#spatial)
  - [Surface](#surface)
  - [Segmentation](#segmentation)
  - [Multi Input](#multi-input)
  - [Color](#color)
  - [Intensity](#intensity)
- [Graph YAML](#graph-yaml)

## Pipeline Model

The graph has three main concepts:

- Filter components: lifecycle components such as `VoxelGridXYZI` or
  `PassThroughXYZ` that process point cloud data.
- Ports: named inputs and outputs on a filter component. Single-cloud filters
  use the `cloud` input and `cloud`/`orig_cloud` outputs. Merger filters
  use `input_1`, `input_2`, `cloud`, `orig_input_1`, and `orig_input_2`.
- Topic nodes: endpoints and intermediate bindings that assign ROS topic names
  to graph edges.

Logical point types describe the data type flowing through a port. Cloud types
such as `PointXYZ`, `PointXYZI`, `PointXYZRGB`, and `PointXYZRGBA` all map to
`sensor_msgs/msg/PointCloud2`. The `PointIndices` logical type maps to
`pcl_msgs/msg/PointIndices`.

Because several logical cloud types share the same ROS message type, the editor
can show both strict logical compatibility and looser ROS-message compatibility.
For example, a `PointXYZ` topic and a `PointXYZI` filter both use
`sensor_msgs/msg/PointCloud2`, but their logical point types differ.

## Packages

- [pcl_filter_type_adapters](pcl_filter_type_adapters/README.md): PCL cloud and
  point-indices type adapters plus logical type discovery metadata.
- [pcl_filter_synchronizer](pcl_filter_synchronizer/README.md): header-only
  unique-pointer synchronizer for filters with named input ports.
- [pcl_filter_base](pcl_filter_base/README.md): lifecycle component base classes
  and descriptor helpers for declaring ports, parameters, QoS, and sync.
- [pcl_filter_components](pcl_filter_components/README.md): generic PCL filter
  algorithms and reusable component templates.
- [pcl_filter_xyz](pcl_filter_xyz/README.md): `PointXYZ` aliases, filter
  metadata, and registered components.
- [pcl_filter_xyzi](pcl_filter_xyzi/README.md): `PointXYZI` aliases, filter
  metadata, and registered components.
- [pcl_filter_xyzrgb](pcl_filter_xyzrgb/README.md): `PointXYZRGB` aliases,
  filter metadata, and registered components.
- [pcl_filter_xyzrgba](pcl_filter_xyzrgba/README.md): `PointXYZRGBA` aliases,
  filter metadata, and registered components.
- [pcl_filter_factory](pcl_filter_factory/README.md): saved graph interpreter
  that loads filter nodes and binds their ports to topics.
- [pcl_filter_editor](pcl_filter_editor/README.md): rqt graph editor with live
  background pipeline validation.
- [pcl_filter_tests](pcl_filter_tests/README.md): repository validation coverage
  for discovery, registration, and example graph parsing.

## Architecture

The generic packages define common infrastructure. Concrete point-type packages
instantiate those templates for one PCL point type and export metadata for
factory and editor discovery.

```text
pcl_filter_type_adapters
  -> pcl_filter_synchronizer
    -> pcl_filter_base
    -> pcl_filter_components
      -> pcl_filter_xyz / pcl_filter_xyzi / pcl_filter_xyzrgb / pcl_filter_xyzrgba
        -> pcl_filter_factory
          -> pcl_filter_tests

pcl_filter_editor -> pcl_filter_tests
```

Concrete packages export filters with package metadata similar to:

```xml
<pcl_filter_component
  filter="VoxelGridXYZI"
  input="PointXYZI"
  output="PointXYZI"/>
```

They also export logical cloud aliases:

```xml
<pcl_filter_component
  type="PointXYZI"
  type_adapter="pcl_filter_xyzi::ros::PclCloudAdapterPointXYZI"
  message_type="sensor_msgs/msg/PointCloud2"/>
```

## Filter Reference

Concrete components are exported by point-type package. Common filters are
available as `XYZ`, `XYZI`, `XYZRGB`, and `XYZRGBA` variants, such as
`VoxelGridXYZI` or `PlaneClipperXYZRGBA`. Color filters are exported only by
`pcl_filter_xyzrgb` and `pcl_filter_xyzrgba`; intensity filters are exported
only by `pcl_filter_xyzi`.

Single-cloud filters use one `cloud` input and publish `cloud` plus
`orig_cloud` outputs. Multi-input filters use `input_1`, `input_2`, and publish
`cloud`, `orig_input_1`, and `orig_input_2` outputs.

### Downsampling

| Filter | Description |
| --- | --- |
| `VoxelGrid` | PCL voxel grid downsampling with configurable x/y/z leaf sizes. |
| `ApproximateVoxelGrid` | Faster approximate voxel-grid downsampling for large clouds. |
| `UniformSampling` | Keeps representative points at a minimum spatial radius. |
| `RandomSample` | Selects a configured number of random points from the input cloud. |
| `GridMinimum` | Keeps the minimum z point per 2D grid cell at a configured resolution. |

### Outlier Removal

| Filter | Description |
| --- | --- |
| `StatisticalOutlierRemoval` | Removes points whose neighbor distance statistics exceed the configured threshold. |
| `RadiusOutlierRemoval` | Removes points without enough neighbors inside a configured radius. |
| `ConditionalRemoval` | Keeps or rejects points using a field range condition. |

### Selection

| Filter | Description |
| --- | --- |
| `PassThrough` | Keeps or rejects points inside a field range. |
| `ExtractIndices` | Selects a configured contiguous index range, with optional organized output behavior. |
| `RemoveNaN` | Drops points containing non-finite NaN coordinates. |
| `RemoveInfinite` | Drops points containing infinite coordinates. |
| `KeepOrganized` | Keeps the organized cloud shape while marking rejected points. |

### Spatial

| Filter | Description |
| --- | --- |
| `CropBox` | Keeps or rejects points inside an axis-aligned 3D box. |
| `FrustumCulling` | Keeps points inside a camera-style frustum. |
| `CropSphere` | Keeps or rejects points inside a configured sphere. |
| `ProjectInliers` | Projects points onto a configured plane model. |
| `PlaneClipper` | Keeps or rejects points by signed distance to a plane. |

### Surface

| Filter | Description |
| --- | --- |
| `MedianFilter` | Smooths organized cloud depth values with a median window. |
| `LocalMaximum` | Keeps local maximum points within a configured radius. |
| `BilateralFilter` | Applies edge-preserving smoothing using spatial and range parameters. |
| `VoxelGridCovariance` | Builds covariance voxels and emits representative filtered points. |
| `MorphologicalFilter` | Applies PCL morphological operations over a grid. |
| `MovingLeastSquares` | Smooths and resamples surfaces with moving least squares. |

### Segmentation

| Filter | Description |
| --- | --- |
| `PlaneModelFilter` | Filters points by distance to a configured plane equation. |
| `SACSegmentationExtract` | Fits a plane with SAC/RANSAC and extracts the selected inliers. |
| `EuclideanClusterExtract` | Extracts one Euclidean cluster selected by sorted cluster index. |

### Multi Input

| Filter | Description |
| --- | --- |
| `PointCloudMerger` | Concatenates two input clouds using the existing merger component. |
| `PointCloudConcatenate` | Concatenates two input clouds into one output cloud. |
| `PointCloudSubtract` | Removes points from the first cloud when a nearby point exists in the second cloud. |
| `PointCloudDifference` | Emits the symmetric point difference between two clouds. |

### Color

| Filter | Description |
| --- | --- |
| `ColorThreshold` | Keeps or rejects RGB points inside configured channel ranges. |
| `RGBRangeFilter` | Alias-style RGB range filter using the same channel-range behavior. |
| `RGBAAlphaFilter` | Keeps or rejects RGBA points by alpha range. |

### Intensity

| Filter | Description |
| --- | --- |
| `IntensityThreshold` | Keeps or rejects XYZI points inside an intensity range. |
| `IntensityRangeFilter` | Alias-style intensity range filter using the same intensity-range behavior. |

## Graph YAML

Saved YAML is the portable pipeline description. Filter nodes record component
identity, filter parameters, port QoS, optional synchronization settings, and
editor layout. Edges connect named ports. Topic nodes provide the ROS topic
bindings used by those edges.

This minimal graph represents `/points/input -> VoxelGridXYZI_1 ->
/points/output`:

```yaml
nodes:
  - type: topic
    name: /points/input
    topic_type: PointXYZI
  - type: filter
    name: VoxelGridXYZI_1
    package: pcl_filter_xyzi
    filter: VoxelGridXYZI
    component_class: pcl_filter_xyzi::VoxelGridXYZIComponent
    input_type: PointXYZI
    output_type: PointXYZI
    parameters:
      filter.leaf_size_x: 0.1
      filter.leaf_size_y: 0.1
      filter.leaf_size_z: 0.1
  - type: topic
    name: /points/output
    topic_type: PointXYZI
edges:
  - from: {node: /points/input, port: out}
    to: {node: VoxelGridXYZI_1, port: cloud}
  - from: {node: VoxelGridXYZI_1, port: cloud}
    to: {node: /points/output, port: in}
```

When the factory reads this graph, it loads only `VoxelGridXYZI_1` as a
component. The topic nodes become the values for component port parameters such
as `inputs.cloud.topic` and `outputs.cloud.topic`.
