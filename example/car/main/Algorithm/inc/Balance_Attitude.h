#ifndef __BALANCE_ATTITUDE_H__
#define __BALANCE_ATTITUDE_H__

#include <math.h>
#include <stdint.h>

typedef struct {
  float pitch;  // 俯仰角 (度)
  float roll;   // 横滚角 (度)
  float dt;     // 采样时间 (秒)
  float alpha;  // 互补滤波系数 (0.0 - 1.0), 通常接近1 (e.g. 0.98)
} Balance_Attitude_t;

/**
 * @brief 初始化姿态解算
 * @param alpha 互补滤波系数 (例如 0.98)
 */
void Balance_Attitude_Init(float alpha);

/**
 * @brief 更新姿态解算 (互补滤波)
 *
 * @param ax 加速度计 X轴 (m/s^2 或 g)
 * @param ay 加速度计 Y轴
 * @param az 加速度计 Z轴
 * @param gx 陀螺仪 X轴 (rad/s)
 * @param gy 陀螺仪 Y轴 (rad/s)
 * @param gz 陀螺仪 Z轴 (rad/s)
 * @param dt 距离上次更新的时间间隔 (秒)
 */
void Balance_Attitude_Update(float ax, float ay, float az, float gx, float gy,
                             float gz, float dt);

/**
 * @brief 获取当前的俯仰角
 * @return float 俯仰角 (度)
 */
float Balance_Attitude_GetPitch(void);

/**
 * @brief 获取当前的横滚角
 * @return float 横滚角 (度)
 */
float Balance_Attitude_GetRoll(void);

#endif  // __BALANCE_ATTITUDE_H__
