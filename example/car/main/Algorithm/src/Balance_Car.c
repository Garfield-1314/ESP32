#include "Algorithm/inc/Balance_Car.h"

#include <math.h>

#include "Algorithm/inc/Balance_Attitude.h"
#include "Algorithm/inc/pid.h"
#include "device/inc/imu_hal.h"
#include "device/inc/motor.h"
#include "driver/gpio.h"
#include "driver/inc/gpio_out.h"
#include "driver/inc/pwm.h"
#include "freertos/FreeRTOS.h"

Balance_Car_t balance_car;

// 简单低通滤波
static float filter_speed(float new_val, float old_val, float factor) {
  return old_val * (1.0f - factor) + new_val * factor;
}

void Balance_Car_Init(void) {
  // 机械零点偏移 (需实际调试)
  balance_car.pitch_offset = 0.0f;
  balance_car.target_speed = 0.0f;
  balance_car.target_turn = 0.0f;
  balance_car.speed_filter_factor = 0.05f;  // 速度环滤波强一点
  
  // 1. 直立环 PID (PD控制为主)
  // Kp: 响应倾角，Kd: 响应角速度 (阻尼)
  // 参数需根据车辆具体物理特性调整
  PID_Init(&balance_car.pid_balance, 300.0f, 0.0f, 1.5f, MOTOR_PWM_MAX_DUTY,
           -MOTOR_PWM_MAX_DUTY, 0, 0);

  // 2. 速度环 PID (PI控制)
  // 速度环输出叠加到直立环的目标角度，或者直接叠加PWM
  // 这里采用叠加PWM的方式（并联PID），也可以采用串级（速度环输出目标角度）
  // 通常平恒车速度环Ki很重要，消除静差（保持不倒）
  PID_Init(&balance_car.pid_speed, 80.0f, 0.4f, 0.0f, MOTOR_PWM_MAX_DUTY / 2,
           -MOTOR_PWM_MAX_DUTY / 2, 2000, -2000);

  // 3. 转向环 PID (PD控制)
  // 结合Z轴陀螺仪
  PID_Init(&balance_car.pid_turn, 20.0f, 0.0f, 0.5f, MOTOR_PWM_MAX_DUTY / 2,
           -MOTOR_PWM_MAX_DUTY / 2, 0, 0);
}

void Balance_Car_Set_Target(float speed, float turn) {
  balance_car.target_speed = speed;
  balance_car.target_turn = turn;
}

static float speed_integral = 0.0f;
static float filtered_speed = 0.0f;

void Balance_Car_Control(float pitch, float gyro_y, float gyro_z, float enc_l,
                         float enc_r, int32_t *out_left, int32_t *out_right) {
  // --- 1. 直立环 (Vertical / Balance Loop) ---
  // 目标角度 = 机械零点
  // 误差 = (当前角度 - 机械零点)
  // 若使用PD: Output = Kp * error + Kd * (-gyro_y)
  // 注意: gyro_y 是角速度，本身就是角度的微分，所以不需要手动求导
  // 极性: 假设前倾为正。如果前倾，车轮应向前加速 (PWM正)。
  // 需确认 pitch 和 gyro_y 的极性与电机正向是否一致。

  float balance_pwm =
      balance_car.pid_balance.Kp * (pitch - balance_car.pitch_offset) +
      balance_car.pid_balance.Kd * gyro_y;

  // --- 2. 速度环 (Speed / Velocity Loop) ---
  // 输入: 左右轮编码器速度平均值
  // 速度环频率通常比直立环低，或者需要强滤波
  float current_avg_speed = (enc_l + enc_r) / 2.0f;
  filtered_speed = filter_speed(current_avg_speed, filtered_speed,
                                balance_car.speed_filter_factor);

  // 速度误差 = 目标速度 - 实际速度
  // 平衡车的速度环有些特殊:
  // 如果车前倾 (pitch > 0), 车加速向前 (speed > 0).
  // 为了让车减速或后退回来，我们需要给一个后仰的目标角度 ?
  // 经典并联 PID:
  // Speed_Output = Kp * error + Ki * integral
  // 这个 Output 直接加到 Balance PWM 上。
  // 极性分析: 如果车速过快 (向前)，我们需要车身 "后仰" 从而减速。
  // 后仰意味着 pitch < 0。
  // 在直立环中，pitch < 0 会产生 负 PWM (向后)。
  // 所以，如果 Speed > Target，我们需要 Output 为 负 叠加到 Balance PWM?
  // 或者 Speed term 作为 Balance PID 的 Target Angle。

  // 采用串级方案更稳定: Speed Loop Output -> Pitch Angle Target
  // 但这里为了简化，使用并联叠加 + 积分控制
  // 简单的速度环: Kp * (Speed_Err) + Ki * Integral(Speed_Err)
  // 如果想要车往前跑，给正直立环offset让车前倾。

  float speed_error = (filtered_speed - balance_car.target_speed);

  // 积分限幅
  speed_integral += speed_error;
  if (speed_integral > 20000.0f) speed_integral = 20000.0f;
  if (speed_integral < -20000.0f) speed_integral = -20000.0f;

  float speed_pwm = balance_car.pid_speed.Kp * speed_error +
                    balance_car.pid_speed.Ki * speed_integral;

  // --- 3. 转向环 (Turn Loop) ---
  // 结合 Gyro Z (角速度) 和 Turn Target
  // 转向是在左右轮差速
  float turn_diff = balance_car.pid_turn.Kp * balance_car.target_turn +
                    balance_car.pid_turn.Kd * gyro_z;

  // --- 4. 融合输出 ---
  // 直立环 + 速度环 (控制前后)
  float pwm_main = balance_pwm + speed_pwm;

  // 差速转向
  float pwm_l = pwm_main + turn_diff;
  float pwm_r = pwm_main - turn_diff;

  // 限幅
  if (pwm_l > MOTOR_PWM_MAX_DUTY) pwm_l = MOTOR_PWM_MAX_DUTY;
  if (pwm_l < -MOTOR_PWM_MAX_DUTY) pwm_l = -MOTOR_PWM_MAX_DUTY;
  if (pwm_r > MOTOR_PWM_MAX_DUTY) pwm_r = MOTOR_PWM_MAX_DUTY;
  if (pwm_r < -MOTOR_PWM_MAX_DUTY) pwm_r = -MOTOR_PWM_MAX_DUTY;

  *out_left = (int32_t)pwm_l;
  *out_right = (int32_t)pwm_r;
}

// 辅助函数：设置电机 (硬编码 GPIO/Channel，需与 motor.c 保持一致)
static void set_motor_pwm(int motor_idx, int32_t pwm_val) {
  int dir_gpio = -1;
  int pwm_ch = motor_idx;
  int dir_logic = 0;  // 0 or 1 based on motor_settings.dirX

  if (motor_idx == 0) {
    dir_gpio = GPIO_NUM_4;  // M1
    dir_logic = motor_settings.dir0;
  } else if (motor_idx == 1) {
    dir_gpio = GPIO_NUM_5;  // M2
    dir_logic = motor_settings.dir1;
  } else {
    return;  // 只控制前两路
  }

  // 根据电机配置的方向逻辑设置 GPIO
  // int level = (pwm_val >= 0) ? dir_logic : !dir_logic;

  // 如果 pwm_val < 0，可能是反转，但 pwm_set_duty 只接受正数吗？
  // driver/inc/pwm.h: pwm_set_duty(int channel, int duty);
  // motor.c: pwm_set_duty(..., abs(pid_output));
  // 以及 direction GPIO 设置

  if (pwm_val >= 0) {
    if (dir_logic == 1)
      gpio_out_set(dir_gpio, 1);
    else
      gpio_out_set(dir_gpio, 0);

    pwm_set_duty(pwm_ch, pwm_val);
  } else {
    if (dir_logic == 1)
      gpio_out_set(dir_gpio, 0);
    else
      gpio_out_set(dir_gpio, 1);

    pwm_set_duty(pwm_ch, -pwm_val);
  }
}

void Balance_Car_Task(void *arg) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(
      1000 / motor_settings.frequency_hz);  // 控制频率，建议 100Hz - 500Hz

  // 初始化姿态解算
  Balance_Attitude_Init(0.98f);
  Balance_Car_Init();

  int32_t pwm_l = 0;
  int32_t pwm_r = 0;

  for (;;) {
    // 只有在模式正确且电机使能时运行
    if (motor_control_mode == MOTOR_CONTROL_MODE_BALANCE && is_motor_enabled) {
      // 1. 获取传感器数据
      // imu_hal 后台任务持续更新 imu_data (支持 ICM42688 / MPU6050)

      // 注意: 确保 IMU 坐标系与车辆坐标系匹配
      // 假设:
      // Y轴角速度 -> 对应 Pitch 变化
      // Z轴角速度 -> 对应 Yaw (转向)

      float ax = imu_data.accel_x;
      float ay = imu_data.accel_y;
      float az = imu_data.accel_z;
      float gx = imu_data.gyro_x;
      float gy = imu_data.gyro_y;
      float gz = imu_data.gyro_z;

      // 2. 更新姿态 (5ms = 0.005s)
      Balance_Attitude_Update(ax, ay, az, gx, gy, gz, 0.005f);

      // 3. 获取速度
      // motor_get_speed() updates `motor_status.speed_mm_s`
      // 但如果其他任务不调用它，我们需要自己调用
      motor_get_speed();

      float speed_l = motor_status.speed_mm_s[0];
      float speed_r = motor_status.speed_mm_s[1];

      // 4. 计算控制
      float pitch = Balance_Attitude_GetPitch();

      Balance_Car_Control(pitch, gy, gz, speed_l, speed_r, &pwm_l, &pwm_r);

      // 5. 设置电机
      set_motor_pwm(0, pwm_l);
      set_motor_pwm(1, pwm_r);

    } else {
      // 重置积分项，避免模式切换瞬间暴走
      balance_car.pid_speed.integral = 0;

      // 为安全起见，非平衡模式下，如果也没有Mecanum任务运行，可能需要主动停机?
      // 但 Mecanum 模式下有 Mecanum_control_task 控制。
    }

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}
