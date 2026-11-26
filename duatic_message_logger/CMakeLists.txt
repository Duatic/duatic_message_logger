cmake_minimum_required(VERSION 3.22)
project(duatic_message_logger)



# find dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_ros REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(rcutils REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(pybind11_vendor REQUIRED)
find_package(pybind11 REQUIRED)

add_library(duatic_message_logger src/logging.cpp src/rcutils_sink.cpp src/ros.cpp)
add_library(duatic_message_logger::duatic_message_logger ALIAS duatic_message_logger)
target_compile_features(duatic_message_logger PUBLIC c_std_99 cxx_std_20)  # Require C99 and C++20
target_include_directories(duatic_message_logger PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>)
target_link_libraries(duatic_message_logger
  spdlog::spdlog
  fmt::fmt
  ${CMAKE_DL_LIBS}
)
# rcutils headers only: its functions are looked up at runtime, so the library does not link it
target_include_directories(duatic_message_logger SYSTEM PRIVATE
  $<TARGET_PROPERTY:rcutils::rcutils,INTERFACE_INCLUDE_DIRECTORIES>)
target_compile_options(duatic_message_logger PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -Wshadow
  -Wnon-virtual-dtor
  -Wconversion
  -Wundef
  -Wformat=2
  -Wsign-conversion
  -Wnull-dereference
  -Werror=return-type
  -fPIC # As this is used in ros2control as plugin -> generate platform indipendent code
)

# Causes the visibility macros to use dllexport rather than dllimport,
# which is appropriate when building the dll but not consuming it.
target_compile_definitions(duatic_message_logger PRIVATE "DUATIC_MESSAGE_LOGGER_BUILDING_LIBRARY")

# detail/ holds the library's own headers, used only to build it
install(
  DIRECTORY include/
  DESTINATION include/${PROJECT_NAME}
  PATTERN "detail" EXCLUDE
)
install(
  TARGETS duatic_message_logger
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

# Python bindings: the package duatic_message_logger with the extension module _bindings. The build tree gets the same
# layout, so the tests import it from there.
set(PYTHON_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/python)
pybind11_add_module(_bindings src/python/bindings.cpp)
target_link_libraries(_bindings PRIVATE duatic_message_logger)
target_compile_options(_bindings PRIVATE -Wall -Wextra -Werror=return-type)
set_target_properties(_bindings PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PYTHON_BUILD_DIR}/${PROJECT_NAME})
# Copies each Python source into the build tree whenever it changes
file(GLOB PYTHON_SOURCES CONFIGURE_DEPENDS python/${PROJECT_NAME}/*.py)
set(PYTHON_BUILD_SOURCES "")
foreach(source ${PYTHON_SOURCES})
  get_filename_component(source_name ${source} NAME)
  set(copy ${PYTHON_BUILD_DIR}/${PROJECT_NAME}/${source_name})
  add_custom_command(OUTPUT ${copy}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${source} ${copy}
    DEPENDS ${source}
  )
  list(APPEND PYTHON_BUILD_SOURCES ${copy})
endforeach()
add_custom_target(${PROJECT_NAME}_python_sources ALL DEPENDS ${PYTHON_BUILD_SOURCES})
ament_python_install_package(${PROJECT_NAME} PACKAGE_DIR python/${PROJECT_NAME})
install(TARGETS _bindings LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_logging
    test/test_logging.cpp
  )
  target_link_libraries(test_logging
    duatic_message_logger
  )

  # Stands in for a ROS 2 process: rcutils is linked by the test, not by the library
  ament_add_gtest(test_rcutils_detection test/test_rcutils_detection.cpp)
  target_link_libraries(test_rcutils_detection duatic_message_logger rcutils::rcutils)

  # The registry is process-wide, so each value of DUATIC_LOG_LEVEL needs its own process
  ament_add_gtest(test_env_level test/test_env_level.cpp ENV DUATIC_LOG_LEVEL=WARN)
  target_link_libraries(test_env_level duatic_message_logger)
  ament_add_gtest(test_env_level_invalid test/test_env_level_invalid.cpp ENV DUATIC_LOG_LEVEL=verbose)
  target_link_libraries(test_env_level_invalid duatic_message_logger)

  find_package(ament_cmake_pytest REQUIRED)
  ament_add_pytest_test(test_python test/python
    ENV PYTHONPATH=${PYTHON_BUILD_DIR} PYTHONDONTWRITEBYTECODE=1
  )
endif()

ament_export_include_directories(
  "include/${PROJECT_NAME}"
)
ament_export_libraries(
  duatic_message_logger
)
ament_export_targets(
  export_${PROJECT_NAME}
)
ament_export_dependencies(spdlog fmt)
ament_package()
