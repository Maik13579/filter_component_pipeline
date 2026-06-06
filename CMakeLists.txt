cmake_minimum_required(VERSION 3.8)
project(pcl_filter_components)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(pcl_msgs REQUIRED)
find_package(pcl_conversions REQUIRED)
find_package(pcl_ros REQUIRED)
find_package(yaml_cpp_vendor REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(PCL REQUIRED COMPONENTS common filters)

add_library(${PROJECT_NAME}_components SHARED
  src/voxel_grid_xyzi_component.cpp
  src/passthrough_xyzi_component.cpp
  src/crop_box_xyzi_component.cpp
  src/point_cloud_merger_xyzi_component.cpp
)
target_include_directories(${PROJECT_NAME}_components PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
  ${PCL_INCLUDE_DIRS}
)
target_link_libraries(${PROJECT_NAME}_components ${PCL_LIBRARIES})
ament_target_dependencies(${PROJECT_NAME}_components
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
  "pcl_filter_components::VoxelGridXYZIComponent"
  "pcl_filter_components::PassThroughXYZIComponent"
  "pcl_filter_components::CropBoxXYZIComponent"
  "pcl_filter_components::PointCloudMergerXYZIComponent"
)

add_library(${PROJECT_NAME}_pipeline SHARED
  src/pipeline_graph.cpp
  src/pipeline_factory_node.cpp
)
target_include_directories(${PROJECT_NAME}_pipeline PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)
target_link_libraries(${PROJECT_NAME}_pipeline yaml-cpp::yaml-cpp)
ament_target_dependencies(${PROJECT_NAME}_pipeline
  rclcpp
  rclcpp_components
  rclcpp_lifecycle
)

add_executable(pcl_pipeline_factory src/pipeline_factory_main.cpp)
target_link_libraries(pcl_pipeline_factory ${PROJECT_NAME}_pipeline)
ament_target_dependencies(pcl_pipeline_factory
  rclcpp
)

install(TARGETS ${PROJECT_NAME}_components
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
install(TARGETS ${PROJECT_NAME}_pipeline pcl_pipeline_factory
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)
install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY config launch DESTINATION share/${PROJECT_NAME})
install(FILES plugin.xml DESTINATION share/${PROJECT_NAME})
ament_python_install_package(${PROJECT_NAME} PACKAGE_DIR python/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(ament_cmake_pytest REQUIRED)

  ament_add_gtest(test_type_adapter test/test_type_adapter.cpp)
  target_include_directories(test_type_adapter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PCL_INCLUDE_DIRS}
  )
  target_link_libraries(test_type_adapter ${PCL_LIBRARIES})
  ament_target_dependencies(test_type_adapter
    rclcpp
    sensor_msgs
    std_msgs
    pcl_msgs
    pcl_conversions
  )
  set_tests_properties(test_type_adapter PROPERTIES
    ENVIRONMENT
      "RMW_IMPLEMENTATION=rmw_fastrtps_cpp;ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST;ROS_LOG_DIR=/tmp/pcl_filter_components_test_logs"
  )

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
  target_link_libraries(test_parameter_descriptors
    ${PROJECT_NAME}_components
    ${PCL_LIBRARIES}
  )
  ament_target_dependencies(test_parameter_descriptors
    rclcpp
    rclcpp_lifecycle
  )
  set_tests_properties(test_parameter_descriptors PROPERTIES
    ENVIRONMENT
      "RMW_IMPLEMENTATION=rmw_fastrtps_cpp;ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST;ROS_LOG_DIR=/tmp/pcl_filter_components_test_logs"
  )

  ament_add_gtest(test_pipeline_graph test/test_pipeline_graph.cpp)
  target_include_directories(test_pipeline_graph PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
  target_link_libraries(test_pipeline_graph ${PROJECT_NAME}_pipeline)

  ament_add_gtest(test_pipeline_factory test/test_pipeline_factory.cpp)
  target_include_directories(test_pipeline_factory PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
  target_compile_definitions(test_pipeline_factory PRIVATE
    PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
  )
  target_link_libraries(test_pipeline_factory ${PROJECT_NAME}_pipeline)
  ament_target_dependencies(test_pipeline_factory
    rclcpp
  )
  set_tests_properties(test_pipeline_factory PROPERTIES
    ENVIRONMENT
      "RMW_IMPLEMENTATION=rmw_fastrtps_cpp;ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST;ROS_LOG_DIR=/tmp/pcl_filter_components_test_logs"
  )

  ament_add_pytest_test(test_pipeline_graph_py test/test_pipeline_graph.py
    APPEND_ENV PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/python
  )
endif()

ament_export_include_directories(include)
ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(
  rclcpp
  rclcpp_components
  rclcpp_lifecycle
  sensor_msgs
  std_msgs
  pcl_msgs
  pcl_conversions
  pcl_ros
  yaml_cpp_vendor
  yaml-cpp
)
ament_package()
