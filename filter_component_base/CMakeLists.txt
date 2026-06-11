cmake_minimum_required(VERSION 3.8)
project(filter_component_base)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(class_loader REQUIRED)
find_package(component_shm REQUIRED)
find_package(filter_component_synchronizer REQUIRED)
find_package(filters REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(lifecycle_msgs REQUIRED)
find_package(rcl_interfaces REQUIRED)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(std_msgs REQUIRED)

  ament_add_gtest(test_filter_chain_component test/test_filter_chain_component.cpp)
  target_include_directories(test_filter_chain_component PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  )
  ament_target_dependencies(test_filter_chain_component
    component_shm
    filter_component_synchronizer
    filters
    rclcpp
    rclcpp_lifecycle
    std_msgs
  )
  set_tests_properties(test_filter_chain_component PROPERTIES
    ENVIRONMENT
      "RMW_IMPLEMENTATION=rmw_fastrtps_cpp;ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST;ROS_LOG_DIR=/tmp/filter_component_base_test_logs"
  )

  ament_add_gtest(test_filter_component_shm test/test_filter_component_shm.cpp)
  target_include_directories(test_filter_component_shm PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  )
  ament_target_dependencies(test_filter_component_shm
    component_shm
    filter_component_synchronizer
    filters
    rclcpp
    rclcpp_lifecycle
  )
  set_tests_properties(test_filter_component_shm PROPERTIES
    ENVIRONMENT
      "RMW_IMPLEMENTATION=rmw_fastrtps_cpp;ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST;ROS_LOG_DIR=/tmp/filter_component_base_test_logs"
  )
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
ament_target_dependencies(${PROJECT_NAME} INTERFACE
  component_shm
)

add_executable(inspect_filter_components src/inspect_filter_components.cpp)
target_include_directories(inspect_filter_components PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
ament_target_dependencies(inspect_filter_components
  ament_index_cpp
  class_loader
  component_shm
  filter_component_synchronizer
  filters
  rclcpp
  rclcpp_components
  rclcpp_lifecycle
)

install(DIRECTORY include/ DESTINATION include)
install(TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
)
install(TARGETS inspect_filter_components
  DESTINATION lib/${PROJECT_NAME}
)

ament_export_include_directories(include)
ament_export_targets(export_${PROJECT_NAME})
ament_export_dependencies(
  component_shm
  filter_component_synchronizer
  filters
  rclcpp
  rclcpp_components
  rclcpp_lifecycle
  lifecycle_msgs
  rcl_interfaces
  ament_index_cpp
  class_loader
)
ament_package()
