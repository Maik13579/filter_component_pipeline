cmake_minimum_required(VERSION 3.8)
project(pcl_filter_components)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(pcl_filter_base REQUIRED)
find_package(pcl_filter_synchronizer REQUIRED)
find_package(pcl_filter_type_adapters REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(pcl_msgs REQUIRED)
find_package(pcl_conversions REQUIRED)
find_package(pcl_ros REQUIRED)
find_package(PCL REQUIRED COMPONENTS common filters)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${PCL_INCLUDE_DIRS}
)
install(TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
)
install(DIRECTORY include/ DESTINATION include)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)

  ament_add_gtest(test_filters test/test_filters.cpp)
  target_include_directories(test_filters PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PCL_INCLUDE_DIRS}
  )
  target_link_libraries(test_filters ${PCL_LIBRARIES})

  ament_add_gtest(test_parameter_descriptors test/test_parameter_descriptors.cpp)
  target_include_directories(test_parameter_descriptors PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PCL_INCLUDE_DIRS}
  )
  target_link_libraries(test_parameter_descriptors ${PCL_LIBRARIES})
  ament_target_dependencies(test_parameter_descriptors
    pcl_filter_base
    pcl_filter_synchronizer
    pcl_filter_type_adapters
    rclcpp
    rclcpp_lifecycle
    sensor_msgs
    std_msgs
    pcl_msgs
    pcl_conversions
    pcl_ros
  )
  set_tests_properties(test_parameter_descriptors PROPERTIES
    ENVIRONMENT
      "RMW_IMPLEMENTATION=rmw_fastrtps_cpp;ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST;ROS_LOG_DIR=/tmp/pcl_filter_components_test_logs"
  )
endif()

ament_export_include_directories(include)
ament_export_targets(export_${PROJECT_NAME})
ament_export_dependencies(
  pcl_filter_base
  pcl_filter_synchronizer
  pcl_filter_type_adapters
  rclcpp
  rclcpp_lifecycle
  sensor_msgs
  std_msgs
  pcl_msgs
  pcl_conversions
  pcl_ros
)
ament_package()
