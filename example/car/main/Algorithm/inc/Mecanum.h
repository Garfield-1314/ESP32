#ifndef __MECANUM_H__
#define __MECANUM_H__

#include "Algorithm/inc/pid.h"

// 轮子布局: X型布局
// FL (1)   FR (2)
// RL (3)   RR (4)
//
// 运动学公式 (X型布局)
// v_fl = vx + vy - omega * k
// v_fr = vx - vy + omega * k
// v_rl = vx - vy - omega * k
// v_rr = vx + vy + omega * k

// 麦克纳姆轮底盘参数
typedef struct {
  float wheel_track;   // 轮距 (左右轮中心距离)
  float wheel_base;    // 轴距 (前后轮中心距离)
  float wheel_radius;  // 轮半径
} Mecanum_Params_t;

// 机器人位姿
typedef struct {
  float x;      // X坐标 (mm)
  float y;      // Y坐标 (mm)
  float theta;  // 弧度
} Mecanum_Pose_t;

// 机器人速度
typedef struct {
  float vx;     // X轴速度 (mm/s)
  float vy;     // Y轴速度 (mm/s)
  float omega;  // 角速度 (rad/s)
} Mecanum_Velocity_t;

// 四轮速度
typedef struct {
  float v_fl;  // 左前
  float v_fr;  // 右前
  float v_rl;  // 左后
  float v_rr;  // 右后
} Mecanum_Wheel_Speed_t;

// 麦克纳姆轮控制器
typedef struct {
  Mecanum_Params_t params;
  Mecanum_Pose_t current_pose;
  Mecanum_Pose_t target_pose;
  Mecanum_Velocity_t current_vel;
  Mecanum_Velocity_t target_vel;

  // 位置PID (串级控制的外环)
  PID_Controller pid_x;
  PID_Controller pid_y;
  PID_Controller pid_theta;
  float encoder_accumulated_theta;  // 辅助融合的编码器角度
} Mecanum_Controller_t;

// 全局控制器实例声明
extern Mecanum_Controller_t mecanum_ctrl;

// 初始化
void Mecanum_Init(void);

// 逆运动学: 机器人速度 -> 四轮速度
Mecanum_Wheel_Speed_t Mecanum_InverseKinematics(Mecanum_Velocity_t vel);

// 正运动学: 四轮速度 -> 机器人速度
Mecanum_Velocity_t Mecanum_ForwardKinematics(void);

// 更新里程计
void Mecanum_UpdateOdometry(void);

// 串并联控制计算
// 输入: 当前四轮速度 (用于更新里程计和速度反馈)
// 输出: 目标四轮速度 (用于电机底层PID)
Mecanum_Wheel_Speed_t Mecanum_Control_Loop(void);

void Mecanum_control_task(void *arg);

extern Mecanum_Wheel_Speed_t target_wheel_speed;

void Mecanum_PID_NVS_init(void);
void Mecanum_PID_NVS_load(void);
void Mecanum_PID_NVS_save(void);

#endif  // __MECANUM_H__