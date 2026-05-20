#pragma once
#include <cmath>

namespace esphome {
namespace mpu9250 {

class Madgwick {
 public:
  void update(float gx, float gy, float gz,
              float ax, float ay, float az,
              float mx, float my, float mz,
              float dt) {
    if (dt <= 0.0f) return;

    float beta = 0.1f;
    float sample_freq = 1.0f / dt;

    // Convert gyro degrees/sec to rad/sec
    gx *= 0.0174533f;
    gy *= 0.0174533f;
    gz *= 0.0174533f;

    float q0 = q_[0], q1 = q_[1], q2 = q_[2], q3 = q_[3];

    // Rate of change of quaternion from gyroscope
    float qDot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
    float qDot2 = 0.5f * ( q0 * gx + q2 * gz - q3 * gy);
    float qDot3 = 0.5f * ( q0 * gy - q1 * gz + q3 * gx);
    float qDot4 = 0.5f * ( q0 * gz + q1 * gy - q2 * gx);

    // Normalise accelerometer measurement
    float norm = sqrtf(ax * ax + ay * ay + az * az);
    if (norm > 0.0f) {
      norm = 1.0f / norm;
      ax *= norm;
      ay *= norm;
      az *= norm;

      // Normalise magnetometer measurement
      float mnorm = sqrtf(mx * mx + my * my + mz * mz);
      if (mnorm > 0.0f) {
        mnorm = 1.0f / mnorm;
        mx *= mnorm;
        my *= mnorm;
        mz *= mnorm;

        // Auxiliary variables to avoid repeated arithmetic
        float _2q0mx = 2.0f * q0 * mx;
        float _2q0my = 2.0f * q0 * my;
        float _2q0mz = 2.0f * q0 * mz;
        float _2q1mx = 2.0f * q1 * mx;
        float _2q0 = 2.0f * q0;
        float _2q1 = 2.0f * q1;
        float _2q2 = 2.0f * q2;
        float _2q3 = 2.0f * q3;
        float _2q0q2 = 2.0f * q0 * q2;
        float _2q2q3 = 2.0f * q2 * q3;
        float q0q0 = q0 * q0;
        float q0q1 = q0 * q1;
        float q0q2 = q0 * q2;
        float q0q3 = q0 * q3;
        float q1q1 = q1 * q1;
        float q1q2 = q1 * q2;
        float q1q3 = q1 * q3;
        float q2q2 = q2 * q2;
        float q2q3 = q2 * q3;
        float q3q3 = q3 * q3;

        // Reference direction of Earth's magnetic field
        float hx = mx * q0q0 - _2q0my * q3 + _2q0mz * q2 + mx * q1q1 +
                   _2q1 * my * q2 + _2q1 * mz * q3 - mx * q2q2 - mx * q3q3;
        float hy = _2q0mx * q3 + my * q0q0 - _2q0mz * q1 + _2q1mx * q2 -
                   my * q1q1 + my * q2q2 + _2q2 * mz * q3 - my * q3q3;
        float _2bx = sqrtf(hx * hx + hy * hy);
        float _2bz = -_2q0mx * q2 + _2q0my * q1 + mz * q0q0 + _2q1mx * q3 -
                     mz * q1q1 + _2q2 * my * q3 - mz * q2q2 + mz * q3q3;
        float _4bx = 2.0f * _2bx;
        float _4bz = 2.0f * _2bz;

        // Gradient decent algorithm corrective step
        float s0 = -_2q2 * (2.0f * q1q3 - _2q0q2 - ax) +
                   _2q1 * (2.0f * q0q1 + _2q2q3 - ay) -
                   _2bz * q2 * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) +
                   (-_2bx * q3 + _2bz * q1) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) +
                   _2bx * q2 * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
        float s1 = _2q3 * (2.0f * q1q3 - _2q0q2 - ax) +
                   _2q0 * (2.0f * q0q1 + _2q2q3 - ay) -
                   4.0f * q1 * (1.0f - 2.0f * q1q1 - 2.0f * q2q2 - az) +
                   _2bz * q3 * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) +
                   (_2bx * q2 + _2bz * q0) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) +
                   (_2bx * q3 - _4bz * q1) * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
        float s2 = -_2q0 * (2.0f * q1q3 - _2q0q2 - ax) +
                   _2q3 * (2.0f * q0q1 + _2q2q3 - ay) -
                   4.0f * q2 * (1.0f - 2.0f * q1q1 - 2.0f * q2q2 - az) +
                   (-_4bx * q2 - _2bz * q0) * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) +
                   (_2bx * q1 + _2bz * q3) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) +
                   (_2bx * q0 - _4bz * q2) * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
        float s3 = _2q1 * (2.0f * q1q3 - _2q0q2 - ax) +
                   _2q2 * (2.0f * q0q1 + _2q2q3 - ay) +
                   (-_4bx * q3 + _2bz * q1) * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) +
                   (-_2bx * q0 + _2bz * q2) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) +
                   _2bx * q1 * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);

        // Normalise step magnitude
        norm = sqrtf(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);
        if (norm > 0.0f) {
          norm = 1.0f / norm;
          s0 *= norm;
          s1 *= norm;
          s2 *= norm;
          s3 *= norm;
        }

        // Apply feedback step
        qDot1 -= beta * s0;
        qDot2 -= beta * s1;
        qDot3 -= beta * s2;
        qDot4 -= beta * s3;
      }
    }

    // Integrate rate of change of quaternion to yield quaternion
    q0 += qDot1 * dt;
    q1 += qDot2 * dt;
    q2 += qDot3 * dt;
    q3 += qDot4 * dt;

    // Normalise quaternion
    norm = sqrtf(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    if (norm > 0.0f) {
      norm = 1.0f / norm;
      q_[0] = q0 * norm;
      q_[1] = q1 * norm;
      q_[2] = q2 * norm;
      q_[3] = q3 * norm;
    }
  }

  float get_yaw() const {
    float q0 = q_[0], q1 = q_[1], q2 = q_[2], q3 = q_[3];
    float yaw = atan2f(2.0f * (q1 * q2 + q0 * q3),
                       q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3);
    yaw *= 57.29578f;  // rad to deg
    if (yaw < 0.0f) yaw += 360.0f;
    return yaw;
  }

  float get_pitch() const {
    return asinf(-2.0f * (q_[1] * q_[3] - q_[0] * q_[2])) * 57.29578f;
  }

  float get_roll() const {
    float q0 = q_[0], q1 = q_[1], q2 = q_[2], q3 = q_[3];
    return atan2f(2.0f * (q0 * q1 + q2 * q3),
                  q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3) * 57.29578f;
  }

 private:
  float q_[4] = {1.0f, 0.0f, 0.0f, 0.0f};
};

}  // namespace mpu9250
}  // namespace esphome
