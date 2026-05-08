cmake_minimum_required(VERSION 3.8)
project(message_logger)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_ros REQUIRED)
find_package(duatic_message_logger REQUIRED)

add_library(message_logger INTERFACE)
add_library(message_logger::message_logger ALIAS message_logger)

target_include_directories(message_logger INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>)



target_link_libraries(message_logger INTERFACE duatic_message_logger::duatic_message_logger)

install(
  DIRECTORY include/
  DESTINATION include/${PROJECT_NAME}
)
install(
  TARGETS message_logger
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_message_logger
    test/test_message_logger.cpp
  )
  target_link_libraries(test_message_logger
    message_logger
  )
endif()

ament_export_include_directories(
  "include/${PROJECT_NAME}"
)

ament_export_targets(
  export_${PROJECT_NAME} HAS_LIBRARY_TARGET
)

ament_export_dependencies(
  duatic_message_logger
)

ament_package()
