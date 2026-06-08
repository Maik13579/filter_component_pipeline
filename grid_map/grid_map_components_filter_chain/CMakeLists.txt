cmake_minimum_required(VERSION 3.8)
project(grid_map_components_filter_chain)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(filter_component_base REQUIRED)
find_package(filter_component_synchronizer REQUIRED)
find_package(filters REQUIRED)
find_package(grid_map_components_type_adapter REQUIRED)
find_package(grid_map_core REQUIRED)
find_package(grid_map_cv REQUIRED)
find_package(grid_map_filters REQUIRED)
find_package(grid_map_msgs REQUIRED)
find_package(grid_map_ros REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)

add_library(${PROJECT_NAME}_components SHARED
  src/filter_chain_components.cpp
)
target_include_directories(${PROJECT_NAME}_components PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
ament_target_dependencies(${PROJECT_NAME}_components
  filter_component_base
  filter_component_synchronizer
  filters
  grid_map_components_type_adapter
  grid_map_core
  grid_map_cv
  grid_map_filters
  grid_map_msgs
  grid_map_ros
  rclcpp
  rclcpp_components
  rclcpp_lifecycle
)

rclcpp_components_register_nodes(${PROJECT_NAME}_components
  "grid_map_components_filter_chain::RosFilterChainGridMapComponent"
)

install(TARGETS ${PROJECT_NAME}_components
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY config DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_pytest REQUIRED)

  ament_add_pytest_test(test_discovery_py test/test_discovery.py
    APPEND_ENV PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}
  )
endif()

ament_export_include_directories(include)
ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(
  filter_component_base
  filter_component_synchronizer
  filters
  grid_map_components_type_adapter
  grid_map_core
  grid_map_cv
  grid_map_filters
  grid_map_msgs
  grid_map_ros
  rclcpp
  rclcpp_components
  rclcpp_lifecycle
)
ament_package()
