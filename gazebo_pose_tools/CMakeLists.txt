cmake_minimum_required(VERSION 3.8)
project(gazebo_pose_tools)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(gazebo_pose_interfaces REQUIRED)
find_package(gz_transport_vendor REQUIRED)
find_package(gz-transport13 REQUIRED)
find_package(gz-msgs10 REQUIRED)

add_executable(gazebo_pose_service_node src/gazebo_pose_service_node.cpp)

ament_target_dependencies(gazebo_pose_service_node
  rclcpp
  geometry_msgs
  builtin_interfaces
  gazebo_pose_interfaces
)

target_link_libraries(gazebo_pose_service_node
  gz-transport13::gz-transport13
  gz-msgs10::gz-msgs10
)

install(TARGETS gazebo_pose_service_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
