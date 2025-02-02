# flipper-uv-AS7331

- gui inspired by https://github.com/xMasterX/all-the-plugins/tree/dev/base_pack/flipper_i2ctools

## Description

A Flipper Zero application that reads UVA, UVB, and UVC values from the AS7331 UV sensor.

### Features

- Real-time UV light measurement
- Displays UVA (315-400nm), UVB (280-315nm), and UVC (200-280nm) values
- I2C communication with AS7331 sensor
- Simple and intuitive interface

## Installation

1. Download the .fap file
2. Copy to your Flipper Zero's `apps` directory
3. Connect AS7331 sensor to your Flipper:
   - GND → GND
   - VCC → 3.3V
   - SDA → C0
   - SCL → C1

## Hardware Requirements

- Flipper Zero
- AS7331 UV sensor
  - buy module ([SparkFun AS7331 module](https://www.sparkfun.com/sparkfun-mini-spectral-uv-sensor-as7331-qwiic.html))
  - module repository (https://github.com/sparkfun/SparkFun_Spectral_UV_Sensor_AS7331)
  - Arduino library (https://github.com/sparkfun/SparkFun_AS7331_Arduino_Library)
  - manufacturer ([ams AS7331 Spectral UV Sensor](https://ams-osram.com/products/sensor-solutions/ambient-light-color-spectral-proximity-sensors/ams-as7331-spectral-uv-sensor))
