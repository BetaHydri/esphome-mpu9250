#include "mpu9250.h"
#include <cmath>

namespace esphome {
namespace mpu9250 {

static const uint8_t PWR_MGMT_1 = 0x6B;
static const uint8_t ACCEL_XOUT_H = 0x3B;
static const uint8_t TEMP_OUT_H = 0x41;
static const uint8_t GYRO_XOUT_H = 0x43;

static const uint8_t AK8963_ADDR = 0x0C;
static const uint8_t AK8963_ST1 = 0x02;
static const uint8_t AK8963_XOUT_L = 0x03;
static const uint8_t AK8963_CNTL1 = 0x0A;

static int16_t to_int16(uint8_t high, uint8_t low) {
  return static_cast<int16_t>((static_cast<uint16_t>(high) << 8) | low);
}

// Helper: write a single byte to a register on an arbitrary I2C address via the bus
static bool bus_write_byte(i2c::I2CBus *bus, uint8_t addr, uint8_t reg, uint8_t val) {
  uint8_t buf[2] = {reg, val};
  return bus->write(addr, buf, 2) == i2c::ERROR_OK;
}

// Helper: read N bytes starting at a register on an arbitrary I2C address via the bus
// Uses write_readv for a single I2C transaction (repeated start, no stop between write and read)
static bool bus_read_bytes(i2c::I2CBus *bus, uint8_t addr, uint8_t reg, uint8_t *data, size_t len) {
  return bus->write_readv(addr, &reg, 1, data, len) == i2c::ERROR_OK;
}

void MPU9250Component::setup() {
  // Wake up MPU9250
  this->write_byte(PWR_MGMT_1, 0x00);
  delay(100);

  // Enable I2C bypass to access AK8963 directly
  this->write_byte(0x37, 0x02);
  delay(10);

  // Set AK8963 to 16-bit, continuous measurement mode 2 (100Hz)
  bus_write_byte(this->bus_, AK8963_ADDR, AK8963_CNTL1, 0x16);
  delay(10);
}

void MPU9250Component::set_calibrate_button(button::Button *b) {
  calib_btn_ = b;
  calib_btn_->add_on_press_callback([this]() { start_calibration(); });
}

void MPU9250Component::start_calibration() {
  calibrating_ = true;
  calib_start_ = millis();
  for (int i = 0; i < 3; i++) {
    mag_min_[i] = 9999.0f;
    mag_max_[i] = -9999.0f;
  }
  ESP_LOGI("mpu9250", "Magnetometer calibration started — rotate sensor slowly around all axes for 30 seconds");
}

void MPU9250Component::update() {
  uint8_t b[6];

  // Read accelerometer (±2g default, 16384 LSB/g)
  if (this->read_bytes(ACCEL_XOUT_H, b, 6)) {
    ax_ = to_int16(b[0], b[1]) / 16384.0f;
    ay_ = to_int16(b[2], b[3]) / 16384.0f;
    az_ = to_int16(b[4], b[5]) / 16384.0f;
    if (ax_s_) ax_s_->publish_state(ax_ * 9.81f);
    if (ay_s_) ay_s_->publish_state(ay_ * 9.81f);
    if (az_s_) az_s_->publish_state(az_ * 9.81f);
  }

  // Read gyroscope (±250 dps default, 131 LSB/dps)
  if (this->read_bytes(GYRO_XOUT_H, b, 6)) {
    gx_ = to_int16(b[0], b[1]) / 131.0f;
    gy_ = to_int16(b[2], b[3]) / 131.0f;
    gz_ = to_int16(b[4], b[5]) / 131.0f;
    if (gx_s_) gx_s_->publish_state(gx_);
    if (gy_s_) gy_s_->publish_state(gy_);
    if (gz_s_) gz_s_->publish_state(gz_);
  }

  // Read temperature (TEMP_OUT_H/L at 0x41, formula: temp = raw / 333.87 + 21.0)
  {
    uint8_t t[2];
    if (this->read_bytes(TEMP_OUT_H, t, 2)) {
      float temp = to_int16(t[0], t[1]) / 333.87f + 21.0f;
      if (temp_s_) temp_s_->publish_state(temp);
    }
  }

  // Read magnetometer (AK8963)
  uint8_t st;
  if (bus_read_bytes(this->bus_, AK8963_ADDR, AK8963_ST1, &st, 1) && (st & 0x01)) {
    uint8_t m[7];
    bus_read_bytes(this->bus_, AK8963_ADDR, AK8963_XOUT_L, m, 7);
    mx_ = to_int16(m[1], m[0]) * 0.15f;
    my_ = to_int16(m[3], m[2]) * 0.15f;
    mz_ = to_int16(m[5], m[4]) * 0.15f;

    // Hard-iron calibration: collect min/max over 30 seconds
    if (calibrating_) {
      mag_min_[0] = std::min(mag_min_[0], mx_);
      mag_min_[1] = std::min(mag_min_[1], my_);
      mag_min_[2] = std::min(mag_min_[2], mz_);
      mag_max_[0] = std::max(mag_max_[0], mx_);
      mag_max_[1] = std::max(mag_max_[1], my_);
      mag_max_[2] = std::max(mag_max_[2], mz_);
      if (millis() - calib_start_ > 30000) {
        calibrating_ = false;
        for (int i = 0; i < 3; i++) {
          mag_offset_[i] = (mag_max_[i] + mag_min_[i]) / 2.0f;
        }
        ESP_LOGI("mpu9250", "Magnetometer calibration done. Offsets: %.1f, %.1f, %.1f",
                 mag_offset_[0], mag_offset_[1], mag_offset_[2]);
      }
    }

    mx_ -= mag_offset_[0];
    my_ -= mag_offset_[1];
    mz_ -= mag_offset_[2];

    if (mx_s_) mx_s_->publish_state(mx_);
    if (my_s_) my_s_->publish_state(my_);
    if (mz_s_) mz_s_->publish_state(mz_);
  }

  // Madgwick sensor fusion for heading
  uint32_t now = millis();
  float dt = (now - last_update_) / 1000.0f;
  last_update_ = now;

  if (use_madgwick_ && heading_s_ && dt > 0.0f && dt < 1.0f) {
    filter_.update(gx_, gy_, gz_, ax_, ay_, az_, mx_, my_, mz_, dt);
    float yaw = filter_.get_yaw() + declination_;
    if (yaw < 0.0f) yaw += 360.0f;
    if (yaw >= 360.0f) yaw -= 360.0f;
    heading_s_->publish_state(yaw);
  }
}

}  // namespace mpu9250
}  // namespace esphome
