cmake_minimum_required(VERSION 3.8)
project(grid_map_components_type_adapter)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(grid_map_core REQUIRED)
find_package(grid_map_msgs REQUIRED)
find_package(grid_map_ros REQUIRED)
find_package(rclcpp REQUIRED)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
install(DIRECTORY include/ DESTINATION include)
install(TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)

  ament_add_gtest(test_type_adapter test/test_type_adapter.cpp)
  target_include_directories(test_type_adapter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
  ament_target_dependencies(test_type_adapter
    grid_map_core
    grid_map_msgs
    grid_map_ros
    rclcpp
  )
  set_tests_properties(test_type_adapter PROPERTIES
    ENVIRONMENT
      "RMW_IMPLEMENTATION=rmw_fastrtps_cpp;ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST;ROS_LOG_DIR=/tmp/grid_map_components_type_adapter_test_logs"
  )
endif()

ament_export_include_directories(include)
ament_export_targets(export_${PROJECT_NAME})
ament_export_dependencies(
  grid_map_core
  grid_map_msgs
  grid_map_ros
  rclcpp
)
ament_package()
