#include "Algorithm/inc/Balance_Attitude.h"

static Balance_Attitude_t attitude;

/**
 * @brief 初始化姿态解算
 * @param alpha 互补滤波系数 (例如 0.98)
 */
void Balance_Attitude_Init(float alpha) {
  attitude.pitch = 0.0f;
  attitude.roll = 0.0f;
  attitude.dt = 0.0f;
  attitude.alpha = alpha;
}

/**
 * @brief 更新姿态解算 (互补滤波)
 *
 * @param ax 加速度计 X轴
 * @param ay 加速度计 Y轴
 * @param az 加速度计 Z轴
 * @param gx 陀螺仪 X轴 (rad/s)
 * @param gy 陀螺仪 Y轴 (rad/s)
 * @param gz 陀螺仪 Z轴 (rad/s)
 * @param dt 距离上次更新的时间间隔 (秒)
 */
void Balance_Attitude_Update(float ax, float ay, float az, float gx, float gy,
                             float gz, float dt) {
  // 1. 计算加速度计得到的倾角 (单位: 弧度 -> 度)
  // 假设车直立时，Z轴向下(重力)，X轴向前，Y轴向右 (根据实际IMU安装调整)
  // Roll (绕X轴旋转) = atan2(ay, az)
  // Pitch (绕Y轴旋转) = atan2(-ax, sqrt(ay*ay + az*az))

  float acc_roll = atan2f(ay, az) * (180.0f / M_PI);
  float acc_pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * (180.0f / M_PI);

  // 2. 陀螺仪积分得到角度变化 (单位: rad/s -> deg/s * s -> deg)
  // 注意: 陀螺仪单位必须转换为 deg/s
  float gyro_d_roll = gx * (180.0f / M_PI) * dt;
  float gyro_d_pitch = gy * (180.0f / M_PI) * dt;

  // 3. 互补滤波融合
  // 下一次角度 = alpha * (上一次角度 + 陀螺仪积分) + (1 - alpha) * 加速度计角度
  // alpha 越大，越信任陀螺仪（动态响应好，但有漂移）；alpha
  // 越小，越信任加速度计（无漂移，但有高频噪声）

  // 处理 Roll
  attitude.roll = attitude.alpha * (attitude.roll + gyro_d_roll) +
                  (1.0f - attitude.alpha) * acc_roll;

  // 处理 Pitch
  attitude.pitch = attitude.alpha * (attitude.pitch + gyro_d_pitch) +
                   (1.0f - attitude.alpha) * acc_pitch;

  attitude.dt = dt;
}

/**
 * @brief 获取当前的俯仰角
 * @return float 俯仰角 (度)
 */
float Balance_Attitude_GetPitch(void) { return attitude.pitch; }

/**
 * @brief 获取当前的横滚角
 * @return float 横滚角 (度)
 */
float Balance_Attitude_GetRoll(void) { return attitude.roll; }
