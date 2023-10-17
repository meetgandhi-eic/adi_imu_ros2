cmake_minimum_required(VERSION 3.8)
project(adi_imu_ros2)

add_executable(adis16470_node src/adis16470_node.cpp src/adis16470.cpp)
include_directories(include)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)

find_package(rclcpp REQUIRED)
ament_target_dependencies(adis16470_node rclcpp)

find_package(sensor_msgs REQUIRED)
ament_target_dependencies(adis16470_node sensor_msgs)

find_package(std_srvs REQUIRED)
ament_target_dependencies(adis16470_node std_srvs)

install(TARGETS adis16470_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/${PROJECT_NAME}/
  DESTINATION include/${PROJECT_NAME}
  FILES_MATCHING PATTERN "*.h"
  )

install(DIRECTORY launch urdf config
  DESTINATION share/${PROJECT_NAME}
  USE_SOURCE_PERMISSIONS
  )

ament_package()
