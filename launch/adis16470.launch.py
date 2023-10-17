import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration
from launch.actions import DeclareLaunchArgument
from launch_ros.actions import Node
from launch.conditions import IfCondition
from rclpy.impl import rcutils_logger
from launch.actions import SetEnvironmentVariable

# based on
# 1. https://docs.ros.org/en/foxy/Tutorials/Intermediate/URDF/Using-URDF-with-Robot-State-Publisher.html#create-a-launch-file
# 2. https://answers.ros.org/question/382000/ros2-makes-launch-files-crazy-too-soon-to-be-migrating/?answer=382141#post-id-382141
# 3. https://answers.ros.org/question/374926/ros2-how-to-launch-rviz2-with-config-file/
def generate_launch_description():  
  SetEnvironmentVariable('RCUTILS_COLORIZED_OUTPUT', "1")
  SetEnvironmentVariable('RCUTILS_CONSOLE_OUTPUT_FORMAT', "[{severity}] [{name}]: {message}")
    
  logger = rcutils_logger.RcutilsLogger(name="adi_imu_ros2_launch")
  
  base_dir = get_package_share_directory('adi_imu_ros2')
  #use_sim_time = LaunchConfiguration('use_sim_time', default='false')
  #with_rviz = LaunchConfiguration('with_rviz', default='false')
  
  logger.info("using configurations from {}".format(base_dir))
  
  #read urdf file and pass it to robot_state_publisher
  urdf_path = os.path.join(base_dir, 'urdf', 'adis16470_breakout.urdf')
  with open(urdf_path, 'r') as urdf_file:
        robot_desc = urdf_file.read()
        
  rviz_config_path = os.path.join(base_dir, 'config', 'adis16470.rviz')
 
  return LaunchDescription([
    DeclareLaunchArgument('with_rviz',
                          default_value='false',
                          description='launch rviz node to visualize'),
    Node(package='robot_state_publisher',
         executable='robot_state_publisher',
         name='robot_state_publisher',
         output='screen',
         parameters=[{'use_sim_time': False, 'robot_description': robot_desc}],
         arguments=[urdf_path]),
    Node(package='adi_imu_ros2',
         executable='adis16470_node',
         name='adis16470_node'),
    Node(package='imu_filter_madgwick',
         executable='imu_filter_madgwick_node',
         name='imu_filter',
         output='screen',
         parameters=[{'use_mag': False, 'fixed_frame':'imu_viz_fixed'}],
         remappings=[("imu/data_raw", "imu")]),
    Node(package='rviz2',
         executable='rviz2',
         name='rviz2_imu',
         arguments=['-d', rviz_config_path],
         condition=IfCondition(LaunchConfiguration('with_rviz')))
  ]
)
