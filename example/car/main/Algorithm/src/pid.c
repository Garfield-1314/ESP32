#include "Algorithm/inc/pid.h"

void PID_Init(PID_Controller *pid, float Kp, float Ki, float Kd,
              float maxOutput, float minOutput, float maxIntegral,
              float minIntegral) {
  pid->Kp = Kp;
  pid->Ki = Ki;
  pid->Kd = Kd;
  pid->maxOutput = maxOutput;
  pid->minOutput = minOutput;
  pid->maxIntegral = maxIntegral;
  pid->minIntegral = minIntegral;
  PID_Reset(pid);
}

void PID_Reset(PID_Controller *pid) {
  pid->target = 0.0f;
  pid->actual = 0.0f;
  pid->error = 0.0f;
  pid->prevError = 0.0f;
  pid->prevPrevError = 0.0f;
  pid->integral = 0.0f;
  pid->output = 0.0f;
}

float PID_Compute_Positional(PID_Controller *pid, float target, float actual) {
  pid->target = target;
  pid->actual = actual;
  pid->error = pid->target - pid->actual;

  pid->integral += pid->error;

  // Integral anti-windup
  if (pid->integral > pid->maxIntegral) {
    pid->integral = pid->maxIntegral;
  } else if (pid->integral < pid->minIntegral) {
    pid->integral = pid->minIntegral;
  }

  pid->output = pid->Kp * pid->error + pid->Ki * pid->integral +
                pid->Kd * (pid->error - pid->prevError);

  pid->prevError = pid->error;

  // Output limiting
  if (pid->output > pid->maxOutput) {
    pid->output = pid->maxOutput;
  } else if (pid->output < pid->minOutput) {
    pid->output = pid->minOutput;
  }

  return pid->output;
}

float PID_Compute_Incremental(PID_Controller *pid, float target, float actual) {
  pid->target = target;
  pid->actual = actual;
  pid->error = pid->target - pid->actual;

  float deltaOutput =
      pid->Kp * (pid->error - pid->prevError) + pid->Ki * pid->error +
      pid->Kd * (pid->error - 2 * pid->prevError + pid->prevPrevError);

  pid->output += deltaOutput;

  pid->prevPrevError = pid->prevError;
  pid->prevError = pid->error;

  // Output limiting
  if (pid->output > pid->maxOutput) {
    pid->output = pid->maxOutput;
  } else if (pid->output < pid->minOutput) {
    pid->output = pid->minOutput;
  }

  return pid->output;
}

void PID_Reset_State(PID_Controller *pid) {
  pid->target = 0;
  pid->actual = 0;
  pid->error = 0;
  pid->prevError = 0;
  pid->prevPrevError = 0;
  pid->integral = 0;
  pid->output = 0;
}