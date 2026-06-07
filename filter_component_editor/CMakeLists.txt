cmake_minimum_required(VERSION 3.8)
project(filter_component_editor)

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)

ament_python_install_package(${PROJECT_NAME})

install(FILES plugin.xml DESTINATION share/${PROJECT_NAME})
install(DIRECTORY doc DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_pytest REQUIRED)

  ament_add_pytest_test(test_pipeline_graph_py test/test_pipeline_graph.py
    APPEND_ENV PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}
  )
endif()

ament_package()
