cmake_minimum_required(VERSION 3.8)
project(pcl_filter_components_xyzi)

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
  src/downsampling/voxel_grid_xyzi_component.cpp
  src/selection/passthrough_xyzi_component.cpp
  src/spatial/crop_box_xyzi_component.cpp
  src/multi_input/point_cloud_merger_xyzi_component.cpp
  src/downsampling/approximate_voxel_grid_xyzi_component.cpp
  src/downsampling/uniform_sampling_xyzi_component.cpp
  src/downsampling/random_sample_xyzi_component.cpp
  src/downsampling/grid_minimum_xyzi_component.cpp
  src/outlier_removal/statistical_outlier_removal_xyzi_component.cpp
  src/outlier_removal/radius_outlier_removal_xyzi_component.cpp
  src/outlier_removal/conditional_removal_xyzi_component.cpp
  src/selection/extract_indices_xyzi_component.cpp
  src/selection/remove_nan_xyzi_component.cpp
  src/selection/remove_infinite_xyzi_component.cpp
  src/selection/keep_organized_xyzi_component.cpp
  src/spatial/frustum_culling_xyzi_component.cpp
  src/spatial/crop_sphere_xyzi_component.cpp
  src/spatial/project_inliers_xyzi_component.cpp
  src/spatial/plane_clipper_xyzi_component.cpp
  src/surface/median_filter_xyzi_component.cpp
  src/surface/local_maximum_xyzi_component.cpp
  src/surface/bilateral_filter_xyzi_component.cpp
  src/surface/voxel_grid_covariance_xyzi_component.cpp
  src/surface/morphological_filter_xyzi_component.cpp
  src/surface/moving_least_squares_xyzi_component.cpp
  src/segmentation/plane_model_filter_xyzi_component.cpp
  src/segmentation/sac_segmentation_extract_xyzi_component.cpp
  src/segmentation/euclidean_cluster_extract_xyzi_component.cpp
  src/multi_input/point_cloud_concatenate_xyzi_component.cpp
  src/multi_input/point_cloud_subtract_xyzi_component.cpp
  src/multi_input/point_cloud_difference_xyzi_component.cpp
  src/intensity/intensity_threshold_xyzi_component.cpp
  src/intensity/intensity_range_xyzi_component.cpp
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
  "pcl_filter_components_xyzi::VoxelGridXYZIComponent"
  "pcl_filter_components_xyzi::PassThroughXYZIComponent"
  "pcl_filter_components_xyzi::CropBoxXYZIComponent"
  "pcl_filter_components_xyzi::PointCloudMergerXYZIComponent"
  "pcl_filter_components_xyzi::ApproximateVoxelGridXYZIComponent"
  "pcl_filter_components_xyzi::UniformSamplingXYZIComponent"
  "pcl_filter_components_xyzi::RandomSampleXYZIComponent"
  "pcl_filter_components_xyzi::GridMinimumXYZIComponent"
  "pcl_filter_components_xyzi::StatisticalOutlierRemovalXYZIComponent"
  "pcl_filter_components_xyzi::RadiusOutlierRemovalXYZIComponent"
  "pcl_filter_components_xyzi::ConditionalRemovalXYZIComponent"
  "pcl_filter_components_xyzi::ExtractIndicesXYZIComponent"
  "pcl_filter_components_xyzi::RemoveNaNXYZIComponent"
  "pcl_filter_components_xyzi::RemoveInfiniteXYZIComponent"
  "pcl_filter_components_xyzi::KeepOrganizedXYZIComponent"
  "pcl_filter_components_xyzi::FrustumCullingXYZIComponent"
  "pcl_filter_components_xyzi::CropSphereXYZIComponent"
  "pcl_filter_components_xyzi::ProjectInliersXYZIComponent"
  "pcl_filter_components_xyzi::PlaneClipperXYZIComponent"
  "pcl_filter_components_xyzi::MedianFilterXYZIComponent"
  "pcl_filter_components_xyzi::LocalMaximumXYZIComponent"
  "pcl_filter_components_xyzi::BilateralFilterXYZIComponent"
  "pcl_filter_components_xyzi::VoxelGridCovarianceXYZIComponent"
  "pcl_filter_components_xyzi::MorphologicalFilterXYZIComponent"
  "pcl_filter_components_xyzi::MovingLeastSquaresXYZIComponent"
  "pcl_filter_components_xyzi::PlaneModelFilterXYZIComponent"
  "pcl_filter_components_xyzi::SACSegmentationExtractXYZIComponent"
  "pcl_filter_components_xyzi::EuclideanClusterExtractXYZIComponent"
  "pcl_filter_components_xyzi::PointCloudConcatenateXYZIComponent"
  "pcl_filter_components_xyzi::PointCloudSubtractXYZIComponent"
  "pcl_filter_components_xyzi::PointCloudDifferenceXYZIComponent"
  "pcl_filter_components_xyzi::IntensityThresholdXYZIComponent"
  "pcl_filter_components_xyzi::IntensityRangeFilterXYZIComponent"
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
