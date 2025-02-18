// Copyright (c) 2017, Analog Devices Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in
//   the documentation and/or other materials provided with the
//   distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#include "adi_driver/adis16470.h"

#include <string>
#include <functional>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <sensor_msgs/msg/temperature.hpp>
#include <std_srvs/srv/trigger.hpp>
#include <unistd.h>

using namespace std::chrono_literals;
using namespace std::placeholders;

class ImuNode : public rclcpp::Node
{
public:
  Adis16470 imu;
  rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_data_pub_;
  rclcpp::Publisher<sensor_msgs::msg::Temperature>::SharedPtr temp_data_pub_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr bias_srv_;
  std::string device_;
  std::string frame_id_;
  bool burst_mode_;
  bool publish_temperature_;
  double rate_;
  rclcpp::TimerBase::SharedPtr timer_;


  void bias_estimate (const std_srvs::srv::Trigger::Request::SharedPtr req,
                      const std_srvs::srv::Trigger::Response::SharedPtr res)
  {
    if (imu.bias_correction_update() < 0)
    {
      res->success = false;
      res->message = "Bias correction update failed";
      RCLCPP_ERROR(this->get_logger(), "bias_estimate failed");
      return;
    }
    res->success = true;
    res->message = "Bias correction update success";
    RCLCPP_INFO(this->get_logger(), "bias_estimate success");
  }

  explicit ImuNode()
  : Node("imu_adis16470")
  {
    this->declare_parameter<std::string>("device", std::string("/dev/ttyACM0"));
    this->declare_parameter<std::string>("frame_id", std::string("imu"));
    this->declare_parameter<bool>("burst_mode", true);
    this->declare_parameter<double>("rate", 100.0);
    this->declare_parameter<bool>("publish_temperature", true);

    device_ = this->get_parameter("device").as_string();
    frame_id_ = this->get_parameter("frame_id").as_string();
    burst_mode_ = this->get_parameter("burst_mode").as_bool();
    rate_ = this->get_parameter("rate").as_double();
    publish_temperature_ = this->get_parameter("publish_temperature").as_bool();

    RCLCPP_INFO(this->get_logger(), "device: %s", device_.c_str());
    RCLCPP_INFO(this->get_logger(), "frame_id: %s", frame_id_.c_str());
    RCLCPP_INFO(this->get_logger(), "rate: %f [Hz]", rate_);
    RCLCPP_INFO(this->get_logger(), "burst_mode: %s", (burst_mode_ ? "true": "false"));
    RCLCPP_INFO(this->get_logger(), "publish_temperature: %s", (publish_temperature_ ? "true": "false"));

    timer_ = this->create_wall_timer(rate_ * 1ms, std::bind(&ImuNode::publish_data, this));

    // Data publisher
    imu_data_pub_ = this->create_publisher<sensor_msgs::msg::Imu>("imu", 100);
    if (publish_temperature_)
    {
        temp_data_pub_ = this->create_publisher<sensor_msgs::msg::Temperature>("temperature", 100);
    }

    bias_srv_ = this->create_service<std_srvs::srv::Trigger>("bias_estimate",
                                                              std::bind(&ImuNode::bias_estimate, this, _1, _2));
  }

  ~ImuNode()
  {
    imu.closePort();
  }

  /**
   * @brief Check if the device is opened
   */
  bool is_opened(void)
  {
    return (imu.fd_ >= 0);
  }
  /**
   * @brief Open IMU device file
   */
  bool open(void)
  {
    // Open device file
    if (imu.openPort(device_) < 0)
    {
      RCLCPP_ERROR(this->get_logger(), "Failed to open device %s", device_.c_str());
      return false;
    }
    // Wait 10ms for SPI ready
    usleep(10000);
    int16_t pid = 0;
    imu.get_product_id(pid);
    if (imu.product_id_ != pid) {
      RCLCPP_ERROR(this->get_logger(), "Port opened but found invalid product ID: %d expected: %d",
                                        pid, imu.product_id_);
      imu.closePort();
      return false;
    }
    RCLCPP_INFO(this->get_logger(), "Product ID: %d", pid);
    imu.set_bias_estimation_time(0x070a);
    return true;
  }
  
  int publish_imu_data()
  {
    sensor_msgs::msg::Imu data;
    data.header.frame_id = frame_id_;
    data.header.stamp = this->get_clock()->now();

    // Linear acceleration
    data.linear_acceleration.x = imu.accl[0];
    data.linear_acceleration.y = imu.accl[1];
    data.linear_acceleration.z = imu.accl[2];

    // Angular velocity
    data.angular_velocity.x = imu.gyro[0];
    data.angular_velocity.y = imu.gyro[1];
    data.angular_velocity.z = imu.gyro[2];

    // Orientation (not provided)
    data.orientation.x = 0;
    data.orientation.y = 0;
    data.orientation.z = 0;
    data.orientation.w = 1;

    imu_data_pub_->publish(data);
    return 0;
  }
  int publish_temp_data()
  {
    sensor_msgs::msg::Temperature data;
    data.header.frame_id = frame_id_;
    data.header.stamp = this->get_clock()->now();

    // imu Temperature
    data.temperature = imu.temp;
    data.variance = 0;
    
    temp_data_pub_->publish(data);
    return 0;
  }
  void publish_data()
  {
    bool publish_temp = false;
    if ((burst_mode_) && (0 != imu.update_burst())) //burst mode
    {
      RCLCPP_ERROR(this->get_logger(), "Cannot update burst");
      return;
    }
    else if (0 != imu.update()) //normal mode
    {
      RCLCPP_ERROR(this->get_logger(), "Cannot update");
      return;
    }
    
    // publish IMU data
    publish_imu_data();

    //publish temperature if enabled
    if (temp_data_pub_)
    {
      publish_temp_data();
    }
  }
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<ImuNode>();

  node->open();
  while (!node->is_opened() && rclcpp::ok())
  {
    RCLCPP_WARN(node->get_logger(), "Keep trying to open the device in 1 second period...");
    sleep(1);
    node->open();
  }
  rclcpp::spin(node);
  rclcpp::shutdown();
  return(0);
}
