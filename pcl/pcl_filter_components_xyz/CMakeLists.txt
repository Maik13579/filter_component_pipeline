cmake_minimum_required(VERSION 3.8)
project(pcl_filter_components_xyz)

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
  src/downsampling/voxel_grid_xyz_component.cpp
  src/selection/passthrough_xyz_component.cpp
  src/spatial/crop_box_xyz_component.cpp
  src/multi_input/point_cloud_merger_xyz_component.cpp
  src/downsampling/approximate_voxel_grid_xyz_component.cpp
  src/downsampling/uniform_sampling_xyz_component.cpp
  src/downsampling/random_sample_xyz_component.cpp
  src/downsampling/grid_minimum_xyz_component.cpp
  src/outlier_removal/statistical_outlier_removal_xyz_component.cpp
  src/outlier_removal/radius_outlier_removal_xyz_component.cpp
  src/outlier_removal/conditional_removal_xyz_component.cpp
  src/selection/extract_indices_xyz_component.cpp
  src/selection/remove_nan_xyz_component.cpp
  src/selection/remove_infinite_xyz_component.cpp
  src/selection/keep_organized_xyz_component.cpp
  src/spatial/frustum_culling_xyz_component.cpp
  src/spatial/crop_sphere_xyz_component.cpp
  src/spatial/project_inliers_xyz_component.cpp
  src/spatial/plane_clipper_xyz_component.cpp
  src/surface/median_filter_xyz_component.cpp
  src/surface/local_maximum_xyz_component.cpp
  src/surface/bilateral_filter_xyz_component.cpp
  src/surface/voxel_grid_covariance_xyz_component.cpp
  src/surface/morphological_filter_xyz_component.cpp
  src/surface/moving_least_squares_xyz_component.cpp
  src/segmentation/plane_model_filter_xyz_component.cpp
  src/segmentation/sac_segmentation_extract_xyz_component.cpp
  src/segmentation/euclidean_cluster_extract_xyz_component.cpp
  src/multi_input/point_cloud_concatenate_xyz_component.cpp
  src/multi_input/point_cloud_subtract_xyz_component.cpp
  src/multi_input/point_cloud_difference_xyz_component.cpp
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
  "pcl_filter_components_xyz::VoxelGridXYZComponent"
  "pcl_filter_components_xyz::PassThroughXYZComponent"
  "pcl_filter_components_xyz::CropBoxXYZComponent"
  "pcl_filter_components_xyz::PointCloudMergerXYZComponent"
  "pcl_filter_components_xyz::ApproximateVoxelGridXYZComponent"
  "pcl_filter_components_xyz::UniformSamplingXYZComponent"
  "pcl_filter_components_xyz::RandomSampleXYZComponent"
  "pcl_filter_components_xyz::GridMinimumXYZComponent"
  "pcl_filter_components_xyz::StatisticalOutlierRemovalXYZComponent"
  "pcl_filter_components_xyz::RadiusOutlierRemovalXYZComponent"
  "pcl_filter_components_xyz::ConditionalRemovalXYZComponent"
  "pcl_filter_components_xyz::ExtractIndicesXYZComponent"
  "pcl_filter_components_xyz::RemoveNaNXYZComponent"
  "pcl_filter_components_xyz::RemoveInfiniteXYZComponent"
  "pcl_filter_components_xyz::KeepOrganizedXYZComponent"
  "pcl_filter_components_xyz::FrustumCullingXYZComponent"
  "pcl_filter_components_xyz::CropSphereXYZComponent"
  "pcl_filter_components_xyz::ProjectInliersXYZComponent"
  "pcl_filter_components_xyz::PlaneClipperXYZComponent"
  "pcl_filter_components_xyz::MedianFilterXYZComponent"
  "pcl_filter_components_xyz::LocalMaximumXYZComponent"
  "pcl_filter_components_xyz::BilateralFilterXYZComponent"
  "pcl_filter_components_xyz::VoxelGridCovarianceXYZComponent"
  "pcl_filter_components_xyz::MorphologicalFilterXYZComponent"
  "pcl_filter_components_xyz::MovingLeastSquaresXYZComponent"
  "pcl_filter_components_xyz::PlaneModelFilterXYZComponent"
  "pcl_filter_components_xyz::SACSegmentationExtractXYZComponent"
  "pcl_filter_components_xyz::EuclideanClusterExtractXYZComponent"
  "pcl_filter_components_xyz::PointCloudConcatenateXYZComponent"
  "pcl_filter_components_xyz::PointCloudSubtractXYZComponent"
  "pcl_filter_components_xyz::PointCloudDifferenceXYZComponent"
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
