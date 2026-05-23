cmake_minimum_required(VERSION 3.8)
project(duatic_message_logger)



# find dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_ros REQUIRED)
find_package(spdlog REQUIRED)
find_package(rclcpp)

add_library(duatic_message_logger src/logging.cpp)
add_library(duatic_message_logger::duatic_message_logger ALIAS duatic_message_logger)
target_compile_features(duatic_message_logger PUBLIC c_std_99 cxx_std_20)  # Require C99 and C++20
target_include_directories(duatic_message_logger PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>)
target_link_libraries(duatic_message_logger
  spdlog::spdlog
)
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

if(rclcpp_FOUND)
  message(STATUS "rclcpp found - building with ROS2 logging support")
  target_link_libraries(duatic_message_logger
    rclcpp::rclcpp
  )
  target_compile_definitions(duatic_message_logger PRIVATE "ENABLE_ROS2_LOGGING")
endif()

# Causes the visibility macros to use dllexport rather than dllimport,
# which is appropriate when building the dll but not consuming it.
target_compile_definitions(duatic_message_logger PRIVATE "DUATIC_MESSAGE_LOGGER_BUILDING_LIBRARY")

install(
  DIRECTORY include/
  DESTINATION include/${PROJECT_NAME}
)
install(
  TARGETS duatic_message_logger
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_logging
    test/test_logging.cpp
  )
  target_link_libraries(test_logging
    duatic_message_logger
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
ament_export_dependencies(spdlog rclcpp)
ament_package()
