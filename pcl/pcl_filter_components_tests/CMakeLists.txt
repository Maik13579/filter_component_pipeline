cmake_minimum_required(VERSION 3.8)
project(pcl_filter_components_tests)

find_package(ament_cmake REQUIRED)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(ament_cmake_pytest REQUIRED)
  find_package(filters REQUIRED)
  find_package(pluginlib REQUIRED)
  find_package(PCL REQUIRED COMPONENTS common filters segmentation surface search kdtree sample_consensus)

  add_library(pcl_filter_components_tests_filters SHARED
    test/test_filter_chain_plugins.cpp
  )
  target_include_directories(pcl_filter_components_tests_filters PRIVATE
    ${PCL_INCLUDE_DIRS}
  )
  ament_target_dependencies(pcl_filter_components_tests_filters
    filters
    pluginlib
  )
  target_link_libraries(pcl_filter_components_tests_filters ${PCL_LIBRARIES})
  pluginlib_export_plugin_description_file(filters test_filters_plugins.xml)
  install(TARGETS pcl_filter_components_tests_filters
    DESTINATION lib
  )

  ament_add_pytest_test(test_discovery test/test_discovery.py)

  file(GLOB_RECURSE test_xyzi_filter_sources CONFIGURE_DEPENDS
    test/filters/*.cpp
  )
  ament_add_gtest(test_xyzi_filters ${test_xyzi_filter_sources})
  target_include_directories(test_xyzi_filters PRIVATE
    test/filters
    ../pcl_filter_components/include
    ${PCL_INCLUDE_DIRS}
  )
  target_link_libraries(test_xyzi_filters ${PCL_LIBRARIES})
endif()

ament_package()
