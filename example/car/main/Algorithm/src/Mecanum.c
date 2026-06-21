#include "Algorithm/inc/Mecanum.h"
#include "Algorithm/inc/imu_fusion.h"

#include <math.h>
#include <string.h>

#include "device/inc/imu_hal.h"
#include "device/inc/motor.h"
#include "driver/inc/nvs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Mecanum_Controller_t mecanum_ctrl;

void Mecanum_Init(void) {
  memset(&mecanum_ctrl, 0, sizeof(Mecanum_Controller_t));
  mecanum_ctrl.params.wheel_track = motor_settings.wheel_track_mm;
  mecanum_ctrl.params.wheel_base = motor_settings.wheel_base_mm;
  mecanum_ctrl.params.wheel_radius = motor_settings.wheel_Radius_mm;

  // 初始化PID参数 (需要根据实际情况调整)
  // 这里给一些默认值，实际使用中需要调参
  PID_Init(&mecanum_ctrl.pid_x, 1.0f, 0.0f, 0.0f, 500.0f, -500.0f, 100.0f,
           -100.0f);
  PID_Init(&mecanum_ctrl.pid_y, 1.0f, 0.0f, 0.0f, 500.0f, -500.0f, 100.0f,
           -100.0f);
  PID_Init(&mecanum_ctrl.pid_theta, 1.0f, 0.0f, 0.0f, 5.0f, -5.0f, 1.0f, -1.0f);

  Mecanum_PID_NVS_init();
}

Mecanum_Wheel_Speed_t Mecanum_InverseKinematics(Mecanum_Velocity_t vel) {
  Mecanum_Wheel_Speed_t wheel_speed;

  float k =
      (mecanum_ctrl.params.wheel_base + mecanum_ctrl.params.wheel_track) / 2.0f;

  wheel_speed.v_fl = vel.vx - vel.vy - vel.omega * k;
  wheel_speed.v_fr = vel.vx + vel.vy + vel.omega * k;
  wheel_speed.v_rl = vel.vx + vel.vy - vel.omega * k;
  wheel_speed.v_rr = vel.vx - vel.vy + vel.omega * k;

  return wheel_speed;
}

Mecanum_Velocity_t Mecanum_ForwardKinematics(void) {
  Mecanum_Wheel_Speed_t wheel_speed;
  Mecanum_Velocity_t vel;

  float k =
      (mecanum_ctrl.params.wheel_base + mecanum_ctrl.params.wheel_track) / 2.0f;

  if (k == 0) k = 1.0f;  // 防止除零

  wheel_speed.v_fl = motor_status.speed_mm_s[0];
  wheel_speed.v_fr = motor_status.speed_mm_s[1];
  wheel_speed.v_rl = motor_status.speed_mm_s[2];
  wheel_speed.v_rr = motor_status.speed_mm_s[3];

  // printf("Wheel Speeds: FL=%.2f FR=%.2f RL=%.2f RR=%.2f\n", wheel_speed.v_fl,
  // wheel_speed.v_fr, wheel_speed.v_rl, wheel_speed.v_rr);

  vel.vx = (wheel_speed.v_fl + wheel_speed.v_fr + wheel_speed.v_rl +
            wheel_speed.v_rr) /
           4.0f;
  vel.vy = (-wheel_speed.v_fl + wheel_speed.v_fr + wheel_speed.v_rl -
            wheel_speed.v_rr) /
           4.0f;
  vel.omega = (-wheel_speed.v_fl + wheel_speed.v_fr - wheel_speed.v_rl +
               wheel_speed.v_rr) /
              (4.0f * k);

  return vel;
}

void Mecanum_UpdateOdometry(void) {
  float dt = 1.0f / motor_settings.frequency_hz;

  // 1. 计算当前机器人坐标系下的速度
  mecanum_ctrl.current_vel = Mecanum_ForwardKinematics();

  // 2. 将速度转换到世界坐标系
  float cos_theta = cosf(mecanum_ctrl.current_pose.theta);
  float sin_theta = sinf(mecanum_ctrl.current_pose.theta);

  float delta_x = (mecanum_ctrl.current_vel.vx * cos_theta -
                   mecanum_ctrl.current_vel.vy * sin_theta) *
                  dt;
  float delta_y = (mecanum_ctrl.current_vel.vx * sin_theta +
                   mecanum_ctrl.current_vel.vy * cos_theta) *
                  dt;

  // 3. 更新位姿 (融合编码器和IMU)
  // 策略：使用互补滤波融合 Gyro Rate 和 Encoder Rate/Position
  // 轮式里程计短期会有滑移，但无漂移（除非轮子空转）；IMU长时有漂移。
  // 这里以 Gyro 积分作为主要角度来源，利用 Encoder 里程计来抑制 IMU 的漂移。

  // A. 编码器增量积分 (作为低频参考)
  float delta_theta_enc = mecanum_ctrl.current_vel.omega * dt;
  mecanum_ctrl.encoder_accumulated_theta += delta_theta_enc;

  // 归一化编码器角度到 -PI ~ PI
  while (mecanum_ctrl.encoder_accumulated_theta > M_PI)
    mecanum_ctrl.encoder_accumulated_theta -= 2.0f * M_PI;
  while (mecanum_ctrl.encoder_accumulated_theta < -M_PI)
    mecanum_ctrl.encoder_accumulated_theta += 2.0f * M_PI;

  // B. IMU 增量 (作为高频来源)
  // gyro_z 单位是 deg/s (由 imu_hal 统一处理)
  float delta_theta_gyro = imu_data.gyro_z * (M_PI / 180.0f) * dt;

  // 零速校正 (ZVU): 如果编码器认为完全静止，则强制 Gyro 增量为 0 (消除静止温漂)
  if (fabsf(mecanum_ctrl.current_vel.omega) < 0.01f &&
      fabsf(mecanum_ctrl.current_vel.vx) < 1.0f &&
      fabsf(mecanum_ctrl.current_vel.vy) < 1.0f) {
    if (fabsf(delta_theta_gyro) < (0.5f * M_PI / 180.0f)) {  // 阈值: 0.5度/帧
      delta_theta_gyro = 0.0f;
    }
  }

  // Step 4: 动态互补滤波系数 (基于加速度计振动检测)
  // 振动越强 → alpha越低 → 更信任编码器
  // 使用加速度计Z轴的变化率作为振动指标
  static float prev_acc_z = 0.0f;
  float vib_mag = fabsf(imu_data.accel_z - prev_acc_z);
  prev_acc_z = imu_data.accel_z;

  // alpha范围: 0.93 ~ 0.99
  // 振动弱 (vib<0.1g) → alpha接近0.99
  // 振动强 (vib>2.0g) → alpha降低到0.93
  float alpha = 0.99f - 0.03f * vib_mag;
  if (alpha < 0.93f) alpha = 0.93f;

  // C. 互补滤波融合
  // 预测: 基于上一时刻的角度 + Gyro 增量
  float predicted_theta = mecanum_ctrl.current_pose.theta + delta_theta_gyro;

  // 计算误差: 编码器角度 - 预测角度
  float error = mecanum_ctrl.encoder_accumulated_theta - predicted_theta;

  // 处理角度回绕，获取最短路径误差
  while (error > M_PI) error -= 2.0f * M_PI;
  while (error < -M_PI) error += 2.0f * M_PI;

  mecanum_ctrl.current_pose.theta = predicted_theta + (1.0f - alpha) * error;

  // printf("Theta: %.3f (Enc: %.3f) alpha=%.3f vib=%.3f\n",
  //        mecanum_ctrl.current_pose.theta,
  //        mecanum_ctrl.encoder_accumulated_theta, alpha, vib_mag);

  // Step 2: 将编码器融合后的Yaw观测反馈给imu_fusion层
  // 以抑制imu_fusion内部Yaw的长期漂移
  // 注意: current_pose.theta 是弧度，imu_fusion_correct_yaw 需要度
  imu_fusion_correct_yaw(mecanum_ctrl.current_pose.theta * (180.0f / M_PI));

  mecanum_ctrl.current_pose.x += delta_x;
  mecanum_ctrl.current_pose.y -= delta_y;

  // 归一化角度到 -PI ~ PI
  while (mecanum_ctrl.current_pose.theta > M_PI)
    mecanum_ctrl.current_pose.theta -= 2.0f * M_PI;
  while (mecanum_ctrl.current_pose.theta < -M_PI)
    mecanum_ctrl.current_pose.theta += 2.0f * M_PI;
}

Mecanum_Wheel_Speed_t target_wheel_speed;

Mecanum_Wheel_Speed_t Mecanum_Control_Loop(void) {
  // 1. 更新里程计
  Mecanum_UpdateOdometry();

  // 2. 位置环 PID 计算 (串级控制: 位置误差 -> 目标速度)
  // 计算世界坐标系下的速度指令
  float vx_global_speed =
      PID_Compute_Positional(&mecanum_ctrl.pid_x, mecanum_ctrl.target_pose.x,
                             mecanum_ctrl.current_pose.x);
  float vy_global_speed =
      -PID_Compute_Positional(&mecanum_ctrl.pid_y, mecanum_ctrl.target_pose.y,
                              mecanum_ctrl.current_pose.y);
  float omega_speed = PID_Compute_Positional(&mecanum_ctrl.pid_theta,
                                             mecanum_ctrl.target_pose.theta,
                                             mecanum_ctrl.current_pose.theta);

  // 3. 转换到机器人坐标系
  float cos_theta = cosf(mecanum_ctrl.current_pose.theta);
  float sin_theta = sinf(mecanum_ctrl.current_pose.theta);

  Mecanum_Velocity_t target_robot_vel;
  target_robot_vel.vx =
      vx_global_speed * cos_theta + vy_global_speed * sin_theta;
  target_robot_vel.vy =
      -vx_global_speed * sin_theta + vy_global_speed * cos_theta;
  target_robot_vel.omega = omega_speed;

  mecanum_ctrl.target_vel = target_robot_vel;

  // 4. 逆运动学: 目标速度 -> 目标轮速
  target_wheel_speed = Mecanum_InverseKinematics(target_robot_vel);

  // 速度平滑处理 (Slew Rate Limiter), 防止打滑
  // 可以在这里调整加速度限制
  float max_accel_mm_s2 = 200.0f;  // 最大加速度 mm/s²
  float max_step = max_accel_mm_s2 / motor_settings.frequency_hz;

  float final_targets[4] = {target_wheel_speed.v_fl, target_wheel_speed.v_fr,
                            target_wheel_speed.v_rl, target_wheel_speed.v_rr};

  for (int i = 0; i < 4; i++) {
    float diff = final_targets[i] - motor_status.target_speed_mm_s[i];
    if (diff > max_step) {
      motor_status.target_speed_mm_s[i] += max_step;
    } else if (diff < -max_step) {
      motor_status.target_speed_mm_s[i] -= max_step;
    } else {
      motor_status.target_speed_mm_s[i] = final_targets[i];
    }
  }

  return target_wheel_speed;
}

void Mecanum_control_task(void *arg) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency =
      pdMS_TO_TICKS(1000 / motor_settings.frequency_hz);
  for (;;) {
    if (motor_control_mode == MOTOR_CONTROL_MODE_MECANUM) {
      motor_get_speed();
      Mecanum_Control_Loop();
      motor_set_speed();
    }
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

static const char *TAG_NVS = "mecanum_nvs";

void Mecanum_PID_NVS_init(void) {
  // 初始化NVS并加载PID参数
  esp_err_t err = nvs_init_custom();
  if (err == ESP_OK) {
    Mecanum_PID_NVS_load();
  } else {
    ESP_LOGE(TAG_NVS, "NVS Init Failed");
  }
}

void Mecanum_PID_NVS_load(void) {
  // motor_PID_NVS_load(); // Already called in init, but keep if needed for
  // refresh

  nvs_read_float("mec_x_p", &mecanum_ctrl.pid_x.Kp);
  nvs_read_float("mec_x_i", &mecanum_ctrl.pid_x.Ki);
  nvs_read_float("mec_x_d", &mecanum_ctrl.pid_x.Kd);

  nvs_read_float("mec_y_p", &mecanum_ctrl.pid_y.Kp);
  nvs_read_float("mec_y_i", &mecanum_ctrl.pid_y.Ki);
  nvs_read_float("mec_y_d", &mecanum_ctrl.pid_y.Kd);

  nvs_read_float("mec_th_p", &mecanum_ctrl.pid_theta.Kp);
  nvs_read_float("mec_th_i", &mecanum_ctrl.pid_theta.Ki);
  nvs_read_float("mec_th_d", &mecanum_ctrl.pid_theta.Kd);

  ESP_LOGI("Mecanum", "Mecanum PID loaded");
}

void Mecanum_PID_NVS_save(void) {
  // motor_PID_NVS_save();

  nvs_write_float("mec_x_p", mecanum_ctrl.pid_x.Kp);
  nvs_write_float("mec_x_i", mecanum_ctrl.pid_x.Ki);
  nvs_write_float("mec_x_d", mecanum_ctrl.pid_x.Kd);

  nvs_write_float("mec_y_p", mecanum_ctrl.pid_y.Kp);
  nvs_write_float("mec_y_i", mecanum_ctrl.pid_y.Ki);
  nvs_write_float("mec_y_d", mecanum_ctrl.pid_y.Kd);

  nvs_write_float("mec_th_p", mecanum_ctrl.pid_theta.Kp);
  nvs_write_float("mec_th_i", mecanum_ctrl.pid_theta.Ki);
  nvs_write_float("mec_th_d", mecanum_ctrl.pid_theta.Kd);

  ESP_LOGI("Mecanum", "Mecanum PID saved");
}