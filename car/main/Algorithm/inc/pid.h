#ifndef __PID_H__
#define __PID_H__

#include <stdint.h>

/*
 * PID_Controller
 * ----------------
 * PID 控制器结构体，用于保存控制器状态与参数。
 * 字段说明：
 *  - Kp, Ki, Kd：比例、积分、微分增益。
 *  - target：期望目标值（设定点）。
 *  - actual：当前测量值。
 *  - error, prevError, prevPrevError：误差及历史误差，用于微分/增量计算。
 *  - integral：积分累加值（用于位置型 PID）。
 *  - output：上一次计算得到的输出值。
 *  - maxOutput / minOutput：输出限幅上/下界。
 *  - maxIntegral / minIntegral：积分防饱和（防积分风up）上下限。
 */
typedef struct {
  float Kp;
  float Ki;
  float Kd;

  float target;
  float actual;
  float error;
  float prevError;
  float prevPrevError;
  float integral;

  float output;
  float maxOutput;
  float minOutput;
  float maxIntegral;
  float minIntegral;
} PID_Controller;

/*
 * 初始化 PID 控制器上下文。
 * 参数：
 *  - pid：指向要初始化的控制器结构体指针。
 *  - Kp, Ki, Kd：PID 三项增益。
 *  - maxOutput/minOutput：输出限幅（上、下界）。
 *  - maxIntegral/minIntegral：积分限幅（用于防积分饱和）。
 */
void PID_Init(PID_Controller *pid, float Kp, float Ki, float Kd,
              float maxOutput, float minOutput, float maxIntegral,
              float minIntegral);

/*
 * 位置型 PID 计算（Positional form）。
 * 返回已限幅的控制器输出值。
 */
float PID_Compute_Positional(PID_Controller *pid, float target, float actual);

/*
 * 增量型 PID 计算（Incremental form）：计算增量并叠加到 pid->output。
 * 返回更新并限幅后的控制器输出值。
 */
float PID_Compute_Incremental(PID_Controller *pid, float target, float actual);

/*
 * 重置控制器内部状态：误差历史、积分项及输出。
 */
void PID_Reset(PID_Controller *pid);

void PID_Reset_State(PID_Controller *pid);

#endif  // __PID_H__
