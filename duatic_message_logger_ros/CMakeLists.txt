cmake_minimum_required(VERSION 3.22)
project(duatic_message_logger_ros)

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_ros REQUIRED)
find_package(duatic_message_logger REQUIRED)
find_package(rclcpp REQUIRED)
find_package(spdlog REQUIRED)

add_library(duatic_message_logger_ros src/ros_logging.cpp)
add_library(duatic_message_logger_ros::duatic_message_logger_ros ALIAS duatic_message_logger_ros)
target_compile_features(duatic_message_logger_ros PUBLIC cxx_std_20)
target_include_directories(duatic_message_logger_ros PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>)
target_link_libraries(duatic_message_logger_ros
  duatic_message_logger::duatic_message_logger
  rclcpp::rclcpp
  spdlog::spdlog
)
target_compile_options(duatic_message_logger_ros PRIVATE
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
)

install(
  DIRECTORY include/
  DESTINATION include/${PROJECT_NAME}
)
install(
  TARGETS duatic_message_logger_ros
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_ros_logging test/test_ros_logging.cpp)
  target_link_libraries(test_ros_logging duatic_message_logger_ros)
endif()

ament_export_include_directories("include/${PROJECT_NAME}")
ament_export_libraries(duatic_message_logger_ros)
ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(duatic_message_logger rclcpp spdlog)
ament_package()
