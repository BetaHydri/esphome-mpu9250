#pragma once
#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/button/button.h"
#include "madgwick.h"

namespace esphome {
namespace mpu9250 {

class MPU9250Component : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;

  void set_accel_x(sensor::Sensor *s) { ax_s_ = s; }
  void set_accel_y(sensor::Sensor *s) { ay_s_ = s; }
  void set_accel_z(sensor::Sensor *s) { az_s_ = s; }

  void set_gyro_x(sensor::Sensor *s) { gx_s_ = s; }
  void set_gyro_y(sensor::Sensor *s) { gy_s_ = s; }
  void set_gyro_z(sensor::Sensor *s) { gz_s_ = s; }

  void set_mag_x(sensor::Sensor *s) { mx_s_ = s; }
  void set_mag_y(sensor::Sensor *s) { my_s_ = s; }
  void set_mag_z(sensor::Sensor *s) { mz_s_ = s; }

  void set_heading(sensor::Sensor *s) { heading_s_ = s; }
  void set_temperature(sensor::Sensor *s) { temp_s_ = s; }
  void set_calibrate_button(button::Button *b);
  void set_use_madgwick(bool v) { use_madgwick_ = v; }
  void set_declination(float d) { declination_ = d; }

 protected:
  sensor::Sensor *ax_s_{nullptr}, *ay_s_{nullptr}, *az_s_{nullptr};
  sensor::Sensor *gx_s_{nullptr}, *gy_s_{nullptr}, *gz_s_{nullptr};
  sensor::Sensor *mx_s_{nullptr}, *my_s_{nullptr}, *mz_s_{nullptr};
  sensor::Sensor *heading_s_{nullptr};
  sensor::Sensor *temp_s_{nullptr};
  button::Button *calib_btn_{nullptr};

  float ax_{0}, ay_{0}, az_{0};
  float gx_{0}, gy_{0}, gz_{0};
  float mx_{0}, my_{0}, mz_{0};

  float mag_min_[3]{0}, mag_max_[3]{0}, mag_offset_[3]{0};
  bool calibrating_{false};
  uint32_t calib_start_{0};

  bool use_madgwick_{true};
  float declination_{0};

  Madgwick filter_;
  uint32_t last_update_{0};

  void start_calibration();
};

}  // namespace mpu9250
}  // namespace esphome
