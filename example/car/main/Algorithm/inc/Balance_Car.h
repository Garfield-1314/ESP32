#ifndef __BALANCE_CAR_H__
#define __BALANCE_CAR_H__

#include <stdint.h>

#include "Algorithm/inc/pid.h"

typedef struct {
  float pitch_offset;  // 机械平衡角度偏移
  float target_speed;  // 目标速度
  float target_turn;   // 目标转向速度

  // PIDs
  PID_Controller pid_balance;  // 直立环
  PID_Controller pid_speed;    // 速度环
  PID_Controller pid_turn;     // 转向环

  // Config
  float speed_filter_factor;  // 速度滤波系数
} Balance_Car_t;

void Balance_Car_Init(void);
void Balance_Car_Set_Target(float speed, float turn);

/**
 * @brief 平衡车控制循环，建议 100Hz - 500Hz 调用
 *
 * @param pitch 当前俯仰角 (deg)
 * @param gyro_y Y轴角速度 (deg/s) - 注意单位需一致
 * @param gyro_z Z轴角速度 (deg/s)
 * @param enc_l 左轮编码器速度 (mm/s)
 * @param enc_r 右轮编码器速度 (mm/s)
 * @param out_left [out] 左轮PWM输出 (-MOTOR_PWM_MAX_DUTY ~ MOTOR_PWM_MAX_DUTY)
 * @param out_right [out] 右轮PWM输出
 */
void Balance_Car_Control(float pitch, float gyro_y, float gyro_z, float enc_l,
                         float enc_r, int32_t *out_left, int32_t *out_right);

void Balance_Car_Task(void *arg);

#endif
