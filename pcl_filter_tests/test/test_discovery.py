# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

from ament_index_python.packages import get_package_prefix, get_package_share_directory

from pcl_filter_editor.filter_discovery import discover_filters


COMMON_SINGLE_INPUT_FILTERS = {
    "VoxelGrid",
    "PassThrough",
    "CropBox",
    "ApproximateVoxelGrid",
    "UniformSampling",
    "RandomSample",
    "GridMinimum",
    "StatisticalOutlierRemoval",
    "RadiusOutlierRemoval",
    "ConditionalRemoval",
    "ExtractIndices",
    "RemoveNaN",
    "RemoveInfinite",
    "KeepOrganized",
    "FrustumCulling",
    "CropSphere",
    "ProjectInliers",
    "PlaneClipper",
    "MedianFilter",
    "LocalMaximum",
    "BilateralFilter",
    "VoxelGridCovariance",
    "MorphologicalFilter",
    "MovingLeastSquares",
    "PlaneModelFilter",
    "SACSegmentationExtract",
    "EuclideanClusterExtract",
}

COMMON_MULTI_INPUT_FILTERS = {
    "PointCloudMerger",
    "PointCloudConcatenate",
    "PointCloudSubtract",
    "PointCloudDifference",
}

PACKAGE_FILTERS = {
    "pcl_filter_xyzi": (
        "XYZI",
        "PointXYZI",
        {
            "IntensityThreshold",
            "IntensityRangeFilter",
        },
    ),
}


def test_discovery_reads_filter_and_type_adapter_exports() -> None:
    discovery = discover_filters()

    filters = {(item.package, item.filter): item for item in discovery.filters}
    for package, (suffix, point_type, _) in PACKAGE_FILTERS.items():
        assert (package, f"VoxelGrid{suffix}") in filters
        assert (package, f"PointCloudMerger{suffix}") in filters
        voxel = filters[(package, f"VoxelGrid{suffix}")]
        assert voxel.component_class == f"{package}::VoxelGrid{suffix}Component"
        assert voxel.input_type == point_type
        assert voxel.output_type == point_type
        assert voxel.input_ports == f"cloud:{point_type}"
        assert voxel.output_ports == f"cloud:{point_type},orig_cloud:{point_type}"
        merger = filters[(package, f"PointCloudMerger{suffix}")]
        assert merger.input_type == f"{point_type},{point_type}"
        assert merger.output_type == point_type
        assert merger.input_ports == f"input_1:{point_type},input_2:{point_type}"
        assert merger.output_ports == f"cloud:{point_type},orig_input_1:{point_type},orig_input_2:{point_type}"

    voxel = filters[("pcl_filter_xyzi", "VoxelGridXYZI")]
    assert voxel.component_class == "pcl_filter_xyzi::VoxelGridXYZIComponent"

    types = {(item.package, item.point_type): item for item in discovery.types}
    assert types[("pcl_filter_xyzi", "PointXYZI")].message_type == "sensor_msgs/msg/PointCloud2"
    assert (
        types[("pcl_filter_xyzi", "PointXYZI")].type_adapter
        == "pcl_filter_xyzi::ros::PclCloudAdapterPointXYZI"
    )
    assert (
        types[("pcl_filter_type_adapters", "PointNormal")].type_adapter
        == "pcl_filter_type_adapters::ros::PclNormalCloudAdapter"
    )
    assert types[("pcl_filter_type_adapters", "PointIndices")].message_type == "pcl_msgs/msg/PointIndices"


def test_discovery_exports_every_registered_filter_component() -> None:
    discovery = discover_filters()
    filters = {(item.package, item.filter): item for item in discovery.filters}

    for package, (suffix, point_type, extra_filters) in PACKAGE_FILTERS.items():
        expected_single_input = COMMON_SINGLE_INPUT_FILTERS | extra_filters
        expected_multi_input = COMMON_MULTI_INPUT_FILTERS
        expected_names = {
            *(f"{name}{suffix}" for name in expected_single_input),
            *(f"{name}{suffix}" for name in expected_multi_input),
        }
        discovered_names = {name for discovered_package, name in filters if discovered_package == package}

        assert discovered_names == expected_names

        for name in expected_single_input:
            item = filters[(package, f"{name}{suffix}")]
            assert item.component_class == f"{package}::{name}{suffix}Component"
            assert item.input_type == point_type
            assert item.output_type == point_type
            assert item.input_ports == f"cloud:{point_type}"
            assert item.output_ports == f"cloud:{point_type},orig_cloud:{point_type}"

        for name in expected_multi_input:
            item = filters[(package, f"{name}{suffix}")]
            assert item.component_class == f"{package}::{name}{suffix}Component"
            assert item.input_type == f"{point_type},{point_type}"
            assert item.output_type == point_type
            assert item.input_ports == f"input_1:{point_type},input_2:{point_type}"
            assert item.output_ports == f"cloud:{point_type},orig_input_1:{point_type},orig_input_2:{point_type}"


def test_components_register_in_rclcpp_components_index() -> None:
    prefix = Path(get_package_prefix("pcl_filter_xyzi"))
    resource = (
        prefix
        / "share"
        / "ament_index"
        / "resource_index"
        / "rclcpp_components"
        / "pcl_filter_xyzi"
    )
    registered = resource.read_text(encoding="utf-8")

    assert "pcl_filter_xyzi::VoxelGridXYZIComponent" in registered
    assert "pcl_filter_xyzi::PassThroughXYZIComponent" in registered
    assert "pcl_filter_xyzi::CropBoxXYZIComponent" in registered
    for filter_name in COMMON_SINGLE_INPUT_FILTERS | COMMON_MULTI_INPUT_FILTERS | {
        "IntensityThreshold",
        "IntensityRangeFilter",
    }:
        assert f"pcl_filter_xyzi::{filter_name}XYZIComponent" in registered


def test_factory_installs_example_pipeline() -> None:
    example = Path(get_package_share_directory("pcl_filter_factory")) / "config" / "example_pipeline.yaml"
    text = example.read_text(encoding="utf-8")

    assert "package: pcl_filter_xyzi" in text
    assert "component_class: pcl_filter_xyzi::VoxelGridXYZIComponent" in text
