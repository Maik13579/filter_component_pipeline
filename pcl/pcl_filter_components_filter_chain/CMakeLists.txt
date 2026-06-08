cmake_minimum_required(VERSION 3.8)
project(pcl_filter_components_filter_chain)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(filter_component_base REQUIRED)
find_package(filter_component_synchronizer REQUIRED)
find_package(filters REQUIRED)
find_package(pcl_filter_components_type_adapters REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(pcl_msgs REQUIRED)
find_package(pcl_conversions REQUIRED)
find_package(PCL REQUIRED COMPONENTS common)

add_library(${PROJECT_NAME}_components SHARED
  src/filter_chain_components.cpp
)
target_include_directories(${PROJECT_NAME}_components PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${PCL_INCLUDE_DIRS}
)
target_link_libraries(${PROJECT_NAME}_components ${PCL_LIBRARIES})
ament_target_dependencies(${PROJECT_NAME}_components
  filter_component_base
  filter_component_synchronizer
  filters
  pcl_filter_components_type_adapters
  rclcpp
  rclcpp_components
  rclcpp_lifecycle
  sensor_msgs
  pcl_msgs
  pcl_conversions
)

rclcpp_components_register_nodes(${PROJECT_NAME}_components
  "pcl_filter_components_filter_chain::RosFilterChainXYZComponent"
  "pcl_filter_components_filter_chain::RosFilterChainXYZIComponent"
  "pcl_filter_components_filter_chain::RosFilterChainXYZRGBComponent"
  "pcl_filter_components_filter_chain::RosFilterChainXYZRGBAComponent"
  "pcl_filter_components_filter_chain::RosFilterChainPointNormalComponent"
  "pcl_filter_components_filter_chain::RosFilterChainPointIndicesComponent"
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
  filter_component_base
  filter_component_synchronizer
  filters
  pcl_filter_components_type_adapters
  rclcpp
  rclcpp_components
  rclcpp_lifecycle
  sensor_msgs
  pcl_msgs
  pcl_conversions
)
ament_package()
