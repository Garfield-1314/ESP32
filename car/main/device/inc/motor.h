#ifndef __MOTOR_H__
#define __MOTOR_H__

#include <stdbool.h>
#include <stdint.h>

void motor_init(void);
void motor_get_speed(void);
void motor_set_speed(void);
void motor_control_task(void *arg);
void motor_PID_NVS_init(void);
void motor_PID_NVS_load(void);
void motor_PID_NVS_save(void);

#define MOTOR_PWM_MAX_DUTY 8000  // 满载PWM占空比
// #define wheel_Diameter_mm 75.0 //轮子直径，单位mm
// #define wheel_Radius_mm (wheel_Diameter_mm / 2.0f) //轮子半径，单位mm
// #define encoder_lines_per_revolution 13 //编码器每转一圈的线数
// #define motor_reduction_ratio 30.0 //电机减速比
// #define encoder_lines_per_motor_revolution (encoder_lines_per_revolution *
// motor_reduction_ratio * 4) //电机每转一圈的线数

// #define frequency_hz 500.0f //控制频率，单位Hz

// #define wheel_base_mm 190.0f //轮子轴距，单位mm
// #define wheel_track_mm 188.0f //轮子轮距，单位mm
// #define wheel_Circumference_mm (wheel_Diameter_mm * 3.1416)
// //轮子周长，单位mm

#define MOTOR_EN_PIN GPIO_NUM_47  // 电机使能引脚

typedef struct {
  float frequency_hz;

  float wheel_diameter_mm;
  float wheel_Radius_mm;
  int32_t encoder_lines_per_revolution;
  float motor_reduction_ratio;
  int32_t encoder_lines_per_motor_revolution;
  float wheel_base_mm;
  float wheel_track_mm;
  float wheel_Circumference_mm;
  float max_speed_limit;

  int32_t dir0;  // 车轮0运动方向
  int32_t dir1;  // 车轮1运动方向
  int32_t dir2;  // 车轮2运动方向
  int32_t dir3;  // 车轮3运动方向

  int32_t encoder_dir0;  // 车轮0编码器方向
  int32_t encoder_dir1;  // 车轮1编码器方向
  int32_t encoder_dir2;  // 车轮2编码器方向
  int32_t encoder_dir3;  // 车轮3编码器方向

} motor_settings_t;

extern motor_settings_t motor_settings;

#include "Algorithm/inc/pid.h"
typedef struct {
  int encoder_number[4];       // 编码器计数值
  float speed_mm_s[4];         // 速度，单位mm/s
  float target_speed_mm_s[4];  // 目标速度，单位mm/s
  float distance_mm[4];        // 轮子行驶距离，单位mm
  float distance_m[4];         // 轮子行驶距离，单位m

  float pid_output[4];  // PID输出值

  PID_Controller pid_controller[4];  // 四个电机的PID控制器
  float pos_kp[4];                   // 位置PID比例系数
  float pos_ki[4];                   // 位置PID积分系数
  float pos_kd[4];                   // 位置PID微分系数

  float inc_kp[4];  // 增量PID比例系数
  float inc_ki[4];  // 增量PID积分系数
  float inc_kd[4];  // 增量PID微分系数

} motor_status_t;

extern motor_status_t motor_status;

typedef enum {
  MOTOR_CONTROL_MODE_MECANUM = 0,
  MOTOR_CONTROL_MODE_DEBUG = 1,
  MOTOR_CONTROL_MODE_BALANCE = 2
} motor_control_mode_t;

extern motor_control_mode_t motor_control_mode;
extern bool is_motor_enabled;

void motor_set_enable(bool enable);
void motor_settings_calc_dependent_params(void);
void motor_setting_NVS_init(void);
void motor_setting_NVS_load(void);
void motor_setting_NVS_save(void);

#endif  // __MOTOR_H__