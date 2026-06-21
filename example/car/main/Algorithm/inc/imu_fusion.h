#ifndef __IMU_FUSION_H__
#define __IMU_FUSION_H__

#include <math.h>
#include <stdint.h>

void imu_fusion_init(void);
void imu_fusion_update(float ax, float ay, float az, float gx, float gy,
                       float gz, float dt);
void imu_fusion_get_euler(float *roll, float *pitch, float *yaw);
void imu_fusion_set_gains(float kp, float ki);

/**
 * @brief Correct the estimated yaw using an external observation (e.g., encoder odometry).
 *        Uses a slow complementary correction to suppress long-term drift.
 * @param yaw_observation Observed yaw angle in radians (must be in same units as internal)
 */
void imu_fusion_correct_yaw(float yaw_observation);

#endif  // __IMU_FUSION_H__
