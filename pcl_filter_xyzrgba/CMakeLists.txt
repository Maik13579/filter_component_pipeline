cmake_minimum_required(VERSION 3.8)
project(pcl_filter_xyzrgba)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(pcl_filter_components REQUIRED)
find_package(pcl_filter_base REQUIRED)
find_package(pcl_filter_synchronizer REQUIRED)
find_package(pcl_filter_type_adapters REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(pcl_msgs REQUIRED)
find_package(pcl_conversions REQUIRED)
find_package(pcl_ros REQUIRED)
find_package(PCL REQUIRED COMPONENTS common filters)

add_library(${PROJECT_NAME}_components SHARED
  src/voxel_grid_xyzrgba_component.cpp
  src/passthrough_xyzrgba_component.cpp
  src/crop_box_xyzrgba_component.cpp
  src/point_cloud_merger_xyzrgba_component.cpp
)
target_include_directories(${PROJECT_NAME}_components PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${PCL_INCLUDE_DIRS}
)
target_link_libraries(${PROJECT_NAME}_components ${PCL_LIBRARIES})
ament_target_dependencies(${PROJECT_NAME}_components
  pcl_filter_components
  pcl_filter_base
  pcl_filter_synchronizer
  pcl_filter_type_adapters
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
  "pcl_filter_xyzrgba::VoxelGridXYZRGBAComponent"
  "pcl_filter_xyzrgba::PassThroughXYZRGBAComponent"
  "pcl_filter_xyzrgba::CropBoxXYZRGBAComponent"
  "pcl_filter_xyzrgba::PointCloudMergerXYZRGBAComponent"
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
  pcl_filter_base
  pcl_filter_synchronizer
  pcl_filter_type_adapters
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
