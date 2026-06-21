#include "device/inc/motor.h"

#include <math.h>
#include <stdio.h>

#include "Algorithm/inc/Mecanum.h"
#include "Algorithm/inc/pid.h"
#include "device/inc/encoder.h"
#include "driver/gpio.h"
#include "driver/inc/gpio_out.h"
#include "driver/inc/nvs.h"
#include "driver/inc/pwm.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

motor_settings_t motor_settings = {
    .frequency_hz = 500,                 // 控制频率，单位Hz
    .wheel_diameter_mm = 75.0f,          // 轮子直径，单位mm
    .wheel_Radius_mm = 37.5f,            // 轮子半径，单位mm
    .encoder_lines_per_revolution = 13,  // 编码器每转一圈的线数
    .motor_reduction_ratio = 30.0f,      // 电机减速比
    .encoder_lines_per_motor_revolution =
        13 * 30.0f * 4.0f,                      // 电机每转一圈的线数
    .wheel_base_mm = 190.0f,                    // 轮子轴距，单位mm
    .wheel_track_mm = 181.5f,                   // 轮子轮距，单位mm
    .wheel_Circumference_mm = 75.0f * 3.1416f,  // 轮子周长，单位mm
    .max_speed_limit = 100.0f,                  // 速度限制，单位mm/s

    .dir0 = 1,  // 车轮0运动方向
    .dir1 = 0,  // 车轮1运动方向
    .dir2 = 1,  // 车轮2运动方向
    .dir3 = 0,  // 车轮3运动方向

    .encoder_dir0 = 1,  // 车轮0编码器方向
    .encoder_dir1 = 0,  // 车轮1编码器方向
    .encoder_dir2 = 1,  // 车轮2编码器方向
    .encoder_dir3 = 0,  // 车轮3编码器方向
};

motor_status_t motor_status = {
    .encoder_number = {0, 0, 0, 0},
    .speed_mm_s = {0, 0, 0, 0},
    .target_speed_mm_s = {0, 0, 0, 0},
    .pid_output = {0, 0, 0, 0},

    // 没有使用位置型PID控制器
    .pos_kp =
        {1.0f, 1.0f, 1.0f,
         1.0f},  // motor1.pos_kp motor2.pos_kp motor3.pos_kp motor4.pos_kp
    .pos_ki =
        {0.1f, 0.1f, 0.1f,
         0.1f},  // motor1.pos_ki motor2.pos_ki motor3.pos_ki motor4.pos_ki
    .pos_kd =
        {0.0f, 0.0f, 0.0f,
         0.0f},  // motor1.pos_kd motor2.pos_kd motor3.pos_kd motor4.pos_kd

    .inc_kp =
        {5.0f, 5.0f, 5.0f,
         5.0f},  // motor1.inc_kp motor2.inc_kp motor3.inc_kp motor4.inc_kp
    .inc_ki =
        {1.0f, 1.0f, 1.0f,
         1.0f},  // motor1.inc_ki motor2.inc_ki motor3.inc_ki motor4.inc_ki
    .inc_kd = {0.0f, 0.0f, 0.0f,
               0.0f}  // motor1.inc_kd motor2.inc_kd motor3.inc_kd motor4.inc_kd
};

// 13-bit resolution (0-8191), 50% duty cycle is approx 4096
void motor_init(void) {
  gpio_out_init(GPIO_NUM_4, 0);    // motor1_dir
  gpio_out_init(GPIO_NUM_5, 0);    // motor2_dir
  gpio_out_init(GPIO_NUM_6, 0);    // motor3_dir
  gpio_out_init(GPIO_NUM_7, 0);    // motor4_dir
  gpio_out_init(MOTOR_EN_PIN, 0);  // motor_enable
  gpio_out_set(MOTOR_EN_PIN, 0);   // disable motors

  pwm_init(GPIO_NUM_16, 0, 1000);  // motor1_pwm
  pwm_init(GPIO_NUM_39, 1, 1000);  // motor2_pwm
  pwm_init(GPIO_NUM_42, 2, 1000);  // motor3_pwm
  pwm_init(GPIO_NUM_3, 3, 1000);   // motor4_pwm

  pwm_set_duty(0, 0);
  pwm_set_duty(1, 0);
  pwm_set_duty(2, 0);
  pwm_set_duty(3, 0);

  encoder_init(0, GPIO_NUM_8, GPIO_NUM_9);    // motor1_encoder
  encoder_init(1, GPIO_NUM_10, GPIO_NUM_11);  // motor2_encoder
  encoder_init(2, GPIO_NUM_12, GPIO_NUM_13);  // motor3_encoder
  encoder_init(3, GPIO_NUM_14, GPIO_NUM_15);  // motor4_encoder

  PID_Init(&motor_status.pid_controller[0], motor_status.inc_kp[0],
           motor_status.inc_ki[0], motor_status.inc_kd[0], MOTOR_PWM_MAX_DUTY,
           -MOTOR_PWM_MAX_DUTY, 1000.0f,
           -1000.0f);  // 初始化四个电机的增量PID控制器
  PID_Init(&motor_status.pid_controller[1], motor_status.inc_kp[1],
           motor_status.inc_ki[1], motor_status.inc_kd[1], MOTOR_PWM_MAX_DUTY,
           -MOTOR_PWM_MAX_DUTY, 1000.0f, -1000.0f);
  PID_Init(&motor_status.pid_controller[2], motor_status.inc_kp[2],
           motor_status.inc_ki[2], motor_status.inc_kd[2], MOTOR_PWM_MAX_DUTY,
           -MOTOR_PWM_MAX_DUTY, 1000.0f, -1000.0f);
  PID_Init(&motor_status.pid_controller[3], motor_status.inc_kp[3],
           motor_status.inc_ki[3], motor_status.inc_kd[3], MOTOR_PWM_MAX_DUTY,
           -MOTOR_PWM_MAX_DUTY, 1000.0f, -1000.0f);
}

void motor_get_speed(void) {
  // motor_status.encoder_number[0] = encoder_get_count(0);
  // motor_status.encoder_number[1] = -encoder_get_count(1);
  // motor_status.encoder_number[2] = encoder_get_count(2);
  // motor_status.encoder_number[3] = -encoder_get_count(3);

  // 计算每个轮子的速度
  if (motor_settings.encoder_dir0 == 1) {
    motor_status.encoder_number[0] = encoder_get_count(0);
  } else {
    motor_status.encoder_number[0] = -encoder_get_count(0);
  }

  if (motor_settings.encoder_dir1 == 1) {
    motor_status.encoder_number[1] = encoder_get_count(1);
  } else {
    motor_status.encoder_number[1] = -encoder_get_count(1);
  }

  if (motor_settings.encoder_dir2 == 1) {
    motor_status.encoder_number[2] = encoder_get_count(2);
  } else {
    motor_status.encoder_number[2] = -encoder_get_count(2);
  }

  if (motor_settings.encoder_dir3 == 1) {
    motor_status.encoder_number[3] = encoder_get_count(3);
  } else {
    motor_status.encoder_number[3] = -encoder_get_count(3);
  }

  for (int i = 0; i < 4; i++) {
    motor_status.speed_mm_s[i] =
        motor_status.encoder_number[i] * motor_settings.wheel_Circumference_mm *
        motor_settings.frequency_hz /
        motor_settings
            .encoder_lines_per_motor_revolution;  // 计算速度，单位mm/s
    encoder_clear_count(i);
    motor_status.encoder_number[i] = 0;  // 读取后清零
  }
  // printf("Motor 0 speed_mm_s: %.2f\n", motor_status.speed_mm_s[0]);
  // printf("Motor 1 speed_mm_s: %.2f\n", motor_status.speed_mm_s[1]);
  // printf("Motor 2 speed_mm_s: %.2f\n", motor_status.speed_mm_s[2]);
  // printf("Motor 3 speed_mm_s: %.2f\n", motor_status.speed_mm_s[3]);
}

void motor_get_distance(void) {
  // 计算每个轮子的距离
  for (int i = 0; i < 4; i++) {
    // distance = (speed_mm_s / frequency_hz)
    motor_status.distance_mm[i] +=
        motor_status.speed_mm_s[i] / motor_settings.frequency_hz;
    motor_status.distance_m[i] = motor_status.distance_mm[i] / 1000.0f;
    // 这里可以将距离累加到一个全局变量中，或者进行其他处理
  }
  // printf("Motor 0 distance_mm: %.2f\n", motor_status.distance_mm[0]);
}

void motor_set_speed(void) {
  for (int i = 0; i < 4; i++) {
    // Update PID controller parameters

    float target_speed = motor_status.target_speed_mm_s[i];
    if (target_speed > motor_settings.max_speed_limit)
      target_speed = motor_settings.max_speed_limit;
    if (target_speed < -motor_settings.max_speed_limit)
      target_speed = -motor_settings.max_speed_limit;

    motor_status.pid_output[i] = round(
        PID_Compute_Incremental(&motor_status.pid_controller[i], target_speed,
                                motor_status.speed_mm_s[i]));
    // Set motor direction and PWM based on PID output
    if (motor_status.pid_output[i] >= 0) {
      switch (i) {
        case 0:
          if (motor_settings.dir0 == 1) {
            gpio_out_set(GPIO_NUM_4, 1);
          } else {
            gpio_out_set(GPIO_NUM_4, 0);
          }
          break;  // motor1_dir

        case 1:
          if (motor_settings.dir1 == 1) {
            gpio_out_set(GPIO_NUM_5, 1);
          } else {
            gpio_out_set(GPIO_NUM_5, 0);
          }
          break;  // motor2_dir
        case 2:
          if (motor_settings.dir2 == 1) {
            gpio_out_set(GPIO_NUM_6, 1);
          } else {
            gpio_out_set(GPIO_NUM_6, 0);
          }
          break;  // motor3_dir
        case 3:
          if (motor_settings.dir3 == 1) {
            gpio_out_set(GPIO_NUM_7, 1);
          } else {
            gpio_out_set(GPIO_NUM_7, 0);
          }
          break;  // motor4_dir
      }
      pwm_set_duty(i, motor_status.pid_output[i] > MOTOR_PWM_MAX_DUTY
                          ? MOTOR_PWM_MAX_DUTY
                          : motor_status.pid_output[i]);
    } else {
      switch (i) {
        case 0:
          if (motor_settings.dir0 == 1) {
            gpio_out_set(GPIO_NUM_4, 0);
          } else {
            gpio_out_set(GPIO_NUM_4, 1);
          }
          break;  // motor1_dir

        case 1:
          if (motor_settings.dir1 == 1) {
            gpio_out_set(GPIO_NUM_5, 0);
          } else {
            gpio_out_set(GPIO_NUM_5, 1);
          }
          break;  // motor2_dir
        case 2:
          if (motor_settings.dir2 == 1) {
            gpio_out_set(GPIO_NUM_6, 0);
          } else {
            gpio_out_set(GPIO_NUM_6, 1);
          }
          break;  // motor3_dir
        case 3:
          if (motor_settings.dir3 == 1) {
            gpio_out_set(GPIO_NUM_7, 0);
          } else {
            gpio_out_set(GPIO_NUM_7, 1);
          }
          break;  // motor4_dir
      }
      pwm_set_duty(i, motor_status.pid_output[i] > MOTOR_PWM_MAX_DUTY
                          ? MOTOR_PWM_MAX_DUTY
                          : -motor_status.pid_output[i]);
    }
  }
  // printf("Motor %d target_speed %f speed %f PID output: %.2f\n", 3,
  // motor_status.target_speed_mm_s[3], motor_status.speed_mm_s[3],
  // motor_status.pid_output[3]);
}

motor_control_mode_t motor_control_mode = MOTOR_CONTROL_MODE_MECANUM;
bool is_motor_enabled = false;

void motor_set_enable(bool enable) {
  is_motor_enabled = enable;
  if (enable) {
    gpio_out_set(MOTOR_EN_PIN, 1);
  } else {
    gpio_out_set(MOTOR_EN_PIN, 0);
  }
}

void motor_control_task(void *arg) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency =
      pdMS_TO_TICKS(1000 / motor_settings.frequency_hz);
  for (;;) {
    if (motor_control_mode == MOTOR_CONTROL_MODE_DEBUG) {
      motor_get_speed();
      motor_get_distance();
      motor_set_speed();
    }
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// NVS for motor PID parameters
static const char *TAG_NVS = "motor_nvs";

void motor_PID_NVS_load(void) {
  char key[16];
  esp_err_t err;

  for (int i = 0; i < 4; i++) {
    // Kp
    snprintf(key, sizeof(key), "m%d_inc_kp", i);
    err = nvs_read_float(key, &motor_status.inc_kp[i]);
    if (err == ESP_OK) {
      motor_status.pid_controller[i].Kp = motor_status.inc_kp[i];
    }

    // Ki
    snprintf(key, sizeof(key), "m%d_inc_ki", i);
    err = nvs_read_float(key, &motor_status.inc_ki[i]);
    if (err == ESP_OK) {
      motor_status.pid_controller[i].Ki = motor_status.inc_ki[i];
    }

    // Kd
    snprintf(key, sizeof(key), "m%d_inc_kd", i);
    err = nvs_read_float(key, &motor_status.inc_kd[i]);
    if (err == ESP_OK) {
      motor_status.pid_controller[i].Kd = motor_status.inc_kd[i];
    }
  }
  ESP_LOGI(TAG_NVS, "PID params loaded");
}

void motor_PID_NVS_init(void) {
  esp_err_t err = nvs_init_custom();
  if (err == ESP_OK) {
    motor_PID_NVS_load();
  } else {
    ESP_LOGE(TAG_NVS, "NVS Init Failed");
  }
}

void motor_PID_NVS_save(void) {
  char key[16];
  for (int i = 0; i < 4; i++) {
    snprintf(key, sizeof(key), "m%d_inc_kp", i);
    nvs_write_float(key, motor_status.inc_kp[i]);

    snprintf(key, sizeof(key), "m%d_inc_ki", i);
    nvs_write_float(key, motor_status.inc_ki[i]);

    snprintf(key, sizeof(key), "m%d_inc_kd", i);
    nvs_write_float(key, motor_status.inc_kd[i]);
  }
  ESP_LOGI(TAG_NVS, "PID params saved");
}

void motor_setting_NVS_init(void) {
  esp_err_t err = nvs_init_custom();
  if (err == ESP_OK) {
    motor_setting_NVS_load();
  } else {
    ESP_LOGE(TAG_NVS, "NVS Init Failed");
  }
}

void motor_settings_calc_dependent_params(void) {
  motor_settings.wheel_diameter_mm = motor_settings.wheel_Radius_mm * 2.0f;
  motor_settings.wheel_Circumference_mm =
      motor_settings.wheel_diameter_mm * 3.1416f;

  if (motor_settings.encoder_lines_per_revolution == 0)
    motor_settings.encoder_lines_per_revolution = 13;
  motor_settings.encoder_lines_per_motor_revolution =
      (int32_t)(motor_settings.encoder_lines_per_revolution *
                motor_settings.motor_reduction_ratio * 4.0f);
}

void motor_setting_NVS_load(void) {
  nvs_read_float("frequency_hz", &motor_settings.frequency_hz);
  nvs_read_float("wheel_Radius_mm", &motor_settings.wheel_Radius_mm);
  nvs_read_float("motor_reduction_ratio",
                 &motor_settings.motor_reduction_ratio);
  nvs_read_float("wheel_base_mm", &motor_settings.wheel_base_mm);
  nvs_read_float("wheel_track_mm", &motor_settings.wheel_track_mm);
  nvs_read_float("max_spd", &motor_settings.max_speed_limit);
  nvs_read_i32("enc_lines_rev", &motor_settings.encoder_lines_per_revolution);

  nvs_read_i32("dir0", &motor_settings.dir0);
  nvs_read_i32("dir1", &motor_settings.dir1);
  nvs_read_i32("dir2", &motor_settings.dir2);
  nvs_read_i32("dir3", &motor_settings.dir3);

  nvs_read_i32("encoder_dir0", &motor_settings.encoder_dir0);
  nvs_read_i32("encoder_dir1", &motor_settings.encoder_dir1);
  nvs_read_i32("encoder_dir2", &motor_settings.encoder_dir2);
  nvs_read_i32("encoder_dir3", &motor_settings.encoder_dir3);

  motor_settings_calc_dependent_params();
  ESP_LOGI(TAG_NVS, "Motor settings loaded");
}

void motor_setting_NVS_save(void) {
  motor_settings_calc_dependent_params();  // Ensure consistent status before
                                           // save

  nvs_write_float("frequency_hz", motor_settings.frequency_hz);
  nvs_write_float("wheel_Radius_mm", motor_settings.wheel_Radius_mm);
  nvs_write_float("motor_reduction_ratio",
                  motor_settings.motor_reduction_ratio);
  nvs_write_float("wheel_base_mm", motor_settings.wheel_base_mm);
  nvs_write_float("wheel_track_mm", motor_settings.wheel_track_mm);
  nvs_write_float("max_spd", motor_settings.max_speed_limit);
  nvs_write_i32("enc_lines_rev", motor_settings.encoder_lines_per_revolution);

  nvs_write_i32("dir0", motor_settings.dir0);
  nvs_write_i32("dir1", motor_settings.dir1);
  nvs_write_i32("dir2", motor_settings.dir2);
  nvs_write_i32("dir3", motor_settings.dir3);

  nvs_write_i32("encoder_dir0", motor_settings.encoder_dir0);
  nvs_write_i32("encoder_dir1", motor_settings.encoder_dir1);
  nvs_write_i32("encoder_dir2", motor_settings.encoder_dir2);
  nvs_write_i32("encoder_dir3", motor_settings.encoder_dir3);

  ESP_LOGI(TAG_NVS, "Motor settings saved");
}