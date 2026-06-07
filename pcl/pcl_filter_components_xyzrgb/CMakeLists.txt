cmake_minimum_required(VERSION 3.8)
project(pcl_filter_components_xyzrgb)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(pcl_filter_components REQUIRED)
find_package(filter_component_base REQUIRED)
find_package(filter_component_synchronizer REQUIRED)
find_package(pcl_filter_components_type_adapters REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(pcl_msgs REQUIRED)
find_package(pcl_conversions REQUIRED)
find_package(pcl_ros REQUIRED)
find_package(PCL REQUIRED COMPONENTS common filters segmentation surface search kdtree sample_consensus)

add_library(${PROJECT_NAME}_components SHARED
  src/downsampling/voxel_grid_xyzrgb_component.cpp
  src/selection/passthrough_xyzrgb_component.cpp
  src/spatial/crop_box_xyzrgb_component.cpp
  src/multi_input/point_cloud_merger_xyzrgb_component.cpp
  src/downsampling/approximate_voxel_grid_xyzrgb_component.cpp
  src/downsampling/uniform_sampling_xyzrgb_component.cpp
  src/downsampling/random_sample_xyzrgb_component.cpp
  src/downsampling/grid_minimum_xyzrgb_component.cpp
  src/outlier_removal/statistical_outlier_removal_xyzrgb_component.cpp
  src/outlier_removal/radius_outlier_removal_xyzrgb_component.cpp
  src/outlier_removal/conditional_removal_xyzrgb_component.cpp
  src/selection/extract_indices_xyzrgb_component.cpp
  src/selection/remove_nan_xyzrgb_component.cpp
  src/selection/remove_infinite_xyzrgb_component.cpp
  src/selection/keep_organized_xyzrgb_component.cpp
  src/spatial/frustum_culling_xyzrgb_component.cpp
  src/spatial/crop_sphere_xyzrgb_component.cpp
  src/spatial/project_inliers_xyzrgb_component.cpp
  src/spatial/plane_clipper_xyzrgb_component.cpp
  src/surface/median_filter_xyzrgb_component.cpp
  src/surface/local_maximum_xyzrgb_component.cpp
  src/surface/bilateral_filter_xyzrgb_component.cpp
  src/surface/voxel_grid_covariance_xyzrgb_component.cpp
  src/surface/morphological_filter_xyzrgb_component.cpp
  src/surface/moving_least_squares_xyzrgb_component.cpp
  src/segmentation/plane_model_filter_xyzrgb_component.cpp
  src/segmentation/sac_segmentation_extract_xyzrgb_component.cpp
  src/segmentation/euclidean_cluster_extract_xyzrgb_component.cpp
  src/multi_input/point_cloud_concatenate_xyzrgb_component.cpp
  src/multi_input/point_cloud_subtract_xyzrgb_component.cpp
  src/multi_input/point_cloud_difference_xyzrgb_component.cpp
  src/color/color_threshold_xyzrgb_component.cpp
  src/color/rgb_range_xyzrgb_component.cpp
)
target_include_directories(${PROJECT_NAME}_components PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${PCL_INCLUDE_DIRS}
)
target_link_libraries(${PROJECT_NAME}_components ${PCL_LIBRARIES})
ament_target_dependencies(${PROJECT_NAME}_components
  pcl_filter_components
  filter_component_base
  filter_component_synchronizer
  pcl_filter_components_type_adapters
  rclcpp
  rclcpp_components
  rclcpp_lifecycle
  sensor_msgs
  std_msgs
  pcl_msgs
  pcl_conversions
  pcl_ros
)

rclcpp_components_register_nodes(${PROJECT_NAME}_components
  "pcl_filter_components_xyzrgb::VoxelGridXYZRGBComponent"
  "pcl_filter_components_xyzrgb::PassThroughXYZRGBComponent"
  "pcl_filter_components_xyzrgb::CropBoxXYZRGBComponent"
  "pcl_filter_components_xyzrgb::PointCloudMergerXYZRGBComponent"
  "pcl_filter_components_xyzrgb::ApproximateVoxelGridXYZRGBComponent"
  "pcl_filter_components_xyzrgb::UniformSamplingXYZRGBComponent"
  "pcl_filter_components_xyzrgb::RandomSampleXYZRGBComponent"
  "pcl_filter_components_xyzrgb::GridMinimumXYZRGBComponent"
  "pcl_filter_components_xyzrgb::StatisticalOutlierRemovalXYZRGBComponent"
  "pcl_filter_components_xyzrgb::RadiusOutlierRemovalXYZRGBComponent"
  "pcl_filter_components_xyzrgb::ConditionalRemovalXYZRGBComponent"
  "pcl_filter_components_xyzrgb::ExtractIndicesXYZRGBComponent"
  "pcl_filter_components_xyzrgb::RemoveNaNXYZRGBComponent"
  "pcl_filter_components_xyzrgb::RemoveInfiniteXYZRGBComponent"
  "pcl_filter_components_xyzrgb::KeepOrganizedXYZRGBComponent"
  "pcl_filter_components_xyzrgb::FrustumCullingXYZRGBComponent"
  "pcl_filter_components_xyzrgb::CropSphereXYZRGBComponent"
  "pcl_filter_components_xyzrgb::ProjectInliersXYZRGBComponent"
  "pcl_filter_components_xyzrgb::PlaneClipperXYZRGBComponent"
  "pcl_filter_components_xyzrgb::MedianFilterXYZRGBComponent"
  "pcl_filter_components_xyzrgb::LocalMaximumXYZRGBComponent"
  "pcl_filter_components_xyzrgb::BilateralFilterXYZRGBComponent"
  "pcl_filter_components_xyzrgb::VoxelGridCovarianceXYZRGBComponent"
  "pcl_filter_components_xyzrgb::MorphologicalFilterXYZRGBComponent"
  "pcl_filter_components_xyzrgb::MovingLeastSquaresXYZRGBComponent"
  "pcl_filter_components_xyzrgb::PlaneModelFilterXYZRGBComponent"
  "pcl_filter_components_xyzrgb::SACSegmentationExtractXYZRGBComponent"
  "pcl_filter_components_xyzrgb::EuclideanClusterExtractXYZRGBComponent"
  "pcl_filter_components_xyzrgb::PointCloudConcatenateXYZRGBComponent"
  "pcl_filter_components_xyzrgb::PointCloudSubtractXYZRGBComponent"
  "pcl_filter_components_xyzrgb::PointCloudDifferenceXYZRGBComponent"
  "pcl_filter_components_xyzrgb::ColorThresholdXYZRGBComponent"
  "pcl_filter_components_xyzrgb::RGBRangeFilterXYZRGBComponent"
)

install(TARGETS ${PROJECT_NAME}_components
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)

ament_export_include_directories(include)
ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(
  pcl_filter_components
  filter_component_base
  filter_component_synchronizer
  pcl_filter_components_type_adapters
  rclcpp
  rclcpp_components
  rclcpp_lifecycle
  sensor_msgs
  std_msgs
  pcl_msgs
  pcl_conversions
  pcl_ros
)
ament_package()
