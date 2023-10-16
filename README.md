# adi_imu_ros2

This package contains ROS 2 driver nodes for Analog Devices(ADI) sensor
products mainly communicate by SPI(Serial Periferal Interface).

Currently supported devices are:

- [ADIS16470](http://www.analog.com/en/products/mems/inertial-measurement-units/adis16470.html)
  - Wide Dynamic Range Mini MEMS IMU
  
You need a SPI interface on your PC to communicate with device. This
package supports
[Devantech's USB-IIS](https://www.robot-electronics.co.uk/htm/usb_iss_tech.htm)
as the USB-SPI converter.

## USB-IIS

### Overview

<div align="center">
  <img src="doc/USB-ISS.jpg" width="60%"/>
</div>

[USB-IIS](https://www.robot-electronics.co.uk/htm/usb_iss_tech.htm) is
a USB to Serial/I2C/SPI converter, simple, small and easy to use. You
don't need any extra library like libusb or libftdi. The device is
available on /dev/ttyACM* as modem device.

Please consult the
[product information](https://www.robot-electronics.co.uk/htm/usb_iss_tech.htm)
and
[SPI documentation](https://www.robot-electronics.co.uk/htm/usb_iss_spi_tech.htm)
for the detail.

### Tips

You need to remove the jumper block on ``Power link`` pins to provide
3.3V for the device.

You need to add your user to dialout group to acces /dev/ttyACM* .

``` $ sudo adduser your_user_name dialout ```

If it takes several seconds until /dev/ttyACM* available, you need to
uninstall modemmanager as:

``` $ sudo apt remove modemmanager ```

## ADIS16470

### Overview

[ADIS16470](http://www.analog.com/en/products/mems/inertial-measurement-units/adis16470.html)
is a complete inertial system that includes a triaxis gyroscope and a
triaxis accelerometer.

<div align="center">
  <img src="doc/ADIS16470_Breakout.jpg" width="60%"/>
</div>

You can use
[Breakout board](http://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/EVAL-ADIS16470.html)
for easy use.

### Connection

<div align="center">
  <img src="doc/ADIS16470_Connection.jpg" width="60%"/>
</div>

You need to build a flat cable to connect the USB-ISS and the
ADIS16470 breakout board. The picture shows a implementation.

Very simple schematic is here. J1 is the USB-ISS pin and J2 is the 2mm
pin headers on the ADIS16470 breakout board.

<div align="center">
  <img src="doc/ADIS16470_Cable.png" width="60%"/>
</div>

Note: you only need to connect one of the power-line(3.3V and
GND). They are connected in the breakout board.

### BOM
- J1: 2550 Connector 6pin
  - Available at [Akiduki](http://akizukidenshi.com/catalog/g/gC-12155/)
- J2: FCI Connector for 1.0mm pitch ribon cables
  - Available at [RS Components](https://jp.rs-online.com/web/p/idc-connectors/6737749/)
- 1.0 mm pitch ribon cable
  - Available at [Aitendo](http://www.aitendo.com/product/11809)

### Quick start

Connect your sensor to USB port. Run the adis16470_node as:

``` $ ros2 run adi_imu_ros2 adis16470_node ```

You can see the model of ADIS16470 breakout board in rviz panel.

<div align="center">
  <img src="doc/img_rviz.png" width="60%"/>
</div>

### Topics

- /imu (sensor_msgs/Imu)

  IMU raw output. It contains angular velocities and linear
  accelerations. The orientation is always unit quaternion.

- /temperature (sensor_msgs/Temperature)

  Temperature of the IMU. To publish this message, you need to set
  true the parameter named 'publish_temperature'. See sample launch
  file.

### Service

- /imu/bias_estimate

  This service activate ADIS16470's internal bias estimation
  function. You should call this service after the IMU is placed
  steady for at least 40 seconds. The bias value of the gyro sensors
  are calcuarated as the average of the duration. The sensor value are
  obtained after it is substracted by the bias value. The bias value
  is stored on the chip and cleared when it powered up or reset.
