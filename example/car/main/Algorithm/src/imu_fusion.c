#include "Algorithm/inc/imu_fusion.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// 四元数姿态
typedef struct {
    float w, x, y, z;
} Quaternion_t;

static Quaternion_t q;  // 全局姿态四元数

// 滤波器增益 (Madgwick风格)
// beta 越大 → 越信任加速度计（收敛快，但高频噪声大）
// beta 越小 → 越信任陀螺仪（平滑，但有漂移）
// 0.1 对大多数应用是好的折中
static float beta = 0.1f;

// Yaw累计修正（用于imu_fusion_correct_yaw的外部观测修正）
// 与四元数独立维护，最终输出 = quat_yaw + external_correction
// 修正速率极慢，仅抑制长期漂移
static float external_yaw_correction = 0.0f;

/**
 * @brief 四元数归一化
 */
static void quaternion_normalize(Quaternion_t *q) {
    float norm = sqrtf(q->w * q->w + q->x * q->x + q->y * q->y + q->z * q->z);
    if (norm < 1e-10f) norm = 1e-10f;
    q->w /= norm;
    q->x /= norm;
    q->y /= norm;
    q->z /= norm;
}

/**
 * @brief 从四元数提取欧拉角 (弧度 -> 度)
 *        roll  = atan2(2(wx + yz), 1 - 2(x² + y²))
 *        pitch = asin(2(wy - zx))
 *        yaw   = atan2(2(wz + xy), 1 - 2(y² + z²))
 */
static void quaternion_to_euler(const Quaternion_t *q, float *roll, float *pitch, float *yaw) {
    float sinr_cosp = 2.0f * (q->w * q->x + q->y * q->z);
    float cosr_cosp = 1.0f - 2.0f * (q->x * q->x + q->y * q->y);
    if (roll) *roll = atan2f(sinr_cosp, cosr_cosp) * (180.0f / M_PI);

    float sinp = 2.0f * (q->w * q->y - q->z * q->x);
    if (sinp > 1.0f) sinp = 1.0f;
    if (sinp < -1.0f) sinp = -1.0f;
    if (pitch) *pitch = asinf(sinp) * (180.0f / M_PI);

    float siny_cosp = 2.0f * (q->w * q->z + q->x * q->y);
    float cosy_cosp = 1.0f - 2.0f * (q->y * q->y + q->z * q->z);
    if (yaw) *yaw = atan2f(siny_cosp, cosy_cosp) * (180.0f / M_PI);
}

/**
 * @brief 从欧拉角(度)构建四元数
 */
static void euler_to_quaternion(float roll_deg, float pitch_deg, float yaw_deg, Quaternion_t *q) {
    float roll = roll_deg * (M_PI / 180.0f);
    float pitch = pitch_deg * (M_PI / 180.0f);
    float yaw = yaw_deg * (M_PI / 180.0f);

    float cy = cosf(yaw * 0.5f);
    float sy = sinf(yaw * 0.5f);
    float cp = cosf(pitch * 0.5f);
    float sp = sinf(pitch * 0.5f);
    float cr = cosf(roll * 0.5f);
    float sr = sinf(roll * 0.5f);

    q->w = cr * cp * cy + sr * sp * sy;
    q->x = sr * cp * cy - cr * sp * sy;
    q->y = cr * sp * cy + sr * cp * sy;
    q->z = cr * cp * sy - sr * sp * cy;

    quaternion_normalize(q);
}

void imu_fusion_set_gains(float kp, float ki) {
    // kp -> beta (加速度计信任度)
    if (kp > 0.0f) {
        beta = kp;
        if (beta < 0.01f) beta = 0.01f;
        if (beta > 1.0f) beta = 1.0f;
    }
}

void imu_fusion_init(void) {
    // 初始四元数: 无旋转 (w=1, x=y=z=0)
    q.w = 1.0f;
    q.x = 0.0f;
    q.y = 0.0f;
    q.z = 0.0f;
    external_yaw_correction = 0.0f;
    beta = 0.1f;
}

/**
 * @brief 四元数姿态解算 (基于Madgwick/Mahony风格互补滤波)
 *
 * 步骤:
 * 1. Gyro预测: 用体坐标系角速度更新四元数
 * 2. Accel修正: 用加速度计观测调整四元数，修正Roll/Pitch
 *
 * 关键: 陀螺仪角速度在体坐标系下，四元数微分能正确处理任意旋转，
 *       因此无论传感器怎么翻滚，Yaw始终保持正确。
 *
 * @param ax,ay,az 加速度计 (单位: g)
 * @param gx,gy,gz 陀螺仪 (单位: deg/s, 函数内转rad/s)
 * @param dt 时间间隔 (秒)
 */
void imu_fusion_update(float ax, float ay, float az, float gx, float gy,
                       float gz, float dt) {
    // 将陀螺仪从 deg/s 转为 rad/s
    float gx_r = gx * (M_PI / 180.0f);
    float gy_r = gy * (M_PI / 180.0f);
    float gz_r = gz * (M_PI / 180.0f);

    // ========== 1. Gyro预测: 四元数微分 ==========
    // q_dot = 0.5 * q * [0, gx, gy, gz]
    // 其中 q * omega 是四元数乘法
    float qw = q.w, qx = q.x, qy = q.y, qz = q.z;

    float q_dot_w = 0.5f * (-qx * gx_r - qy * gy_r - qz * gz_r);
    float q_dot_x = 0.5f * ( qw * gx_r + qy * gz_r - qz * gy_r);
    float q_dot_y = 0.5f * ( qw * gy_r - qx * gz_r + qz * gx_r);
    float q_dot_z = 0.5f * ( qw * gz_r + qx * gy_r - qy * gx_r);

    // 预测: q += q_dot * dt
    float q_pred_w = qw + q_dot_w * dt;
    float q_pred_x = qx + q_dot_x * dt;
    float q_pred_y = qy + q_dot_y * dt;
    float q_pred_z = qz + q_dot_z * dt;

    // 归一化预测四元数
    float norm_pred = sqrtf(q_pred_w * q_pred_w + q_pred_x * q_pred_x +
                            q_pred_y * q_pred_y + q_pred_z * q_pred_z);
    if (norm_pred > 1e-10f) {
        q_pred_w /= norm_pred;
        q_pred_x /= norm_pred;
        q_pred_y /= norm_pred;
        q_pred_z /= norm_pred;
    }

    // ========== 2. Accel修正 (只修正Roll/Pitch, 不影响Yaw) ==========
    // 思路: 用加速度计观测的gravity方向 vs 四元数预测的gravity方向,
    //       计算误差梯度, 沿梯度方向修正四元数。

    // 归一化加速度计向量
    float acc_norm = sqrtf(ax * ax + ay * ay + az * az);
    if (acc_norm < 1e-6f) {
        // 加速度太小(自由落体)，跳过修正
        q = (Quaternion_t){q_pred_w, q_pred_x, q_pred_y, q_pred_z};
        return;
    }
    float ax_n = ax / acc_norm;
    float ay_n = ay / acc_norm;
    float az_n = az / acc_norm;

    // 从预测四元数计算重力在体坐标系中的投影
    // gravity = [2*(x*z - w*y), 2*(w*x + y*z), w² - x² - y² + z²]
    float gx_pred = 2.0f * (q_pred_x * q_pred_z - q_pred_w * q_pred_y);
    float gy_pred = 2.0f * (q_pred_w * q_pred_x + q_pred_y * q_pred_z);
    float gz_pred = q_pred_w * q_pred_w - q_pred_x * q_pred_x
                    - q_pred_y * q_pred_y + q_pred_z * q_pred_z;

    // 误差 = 观测重力 × 预测重力 (叉积)
    // 叉积大小 ≈ sin(θ), 小角度时 ≈ θ
    float err_x = ay_n * gz_pred - az_n * gy_pred;
    float err_y = az_n * gx_pred - ax_n * gz_pred;
    float err_z = ax_n * gy_pred - ay_n * gx_pred;

    // 将误差从体坐标系转换到四元数修正
    // 用修正梯度更新四元数 (Madgwick梯度下降法)
    // 修正量 = beta * dt * J^T * f, 简化实现:
    // 将误差作为角速度修正叠加到陀螺仪
    float gx_corrected = gx_r + beta * err_x;
    float gy_corrected = gy_r + beta * err_y;
    float gz_corrected = gz_r + beta * err_z;

    // 用修正后的角速度重新做四元数微分
    q_dot_w = 0.5f * (-qx * gx_corrected - qy * gy_corrected - qz * gz_corrected);
    q_dot_x = 0.5f * ( qw * gx_corrected + qy * gz_corrected - qz * gy_corrected);
    q_dot_y = 0.5f * ( qw * gy_corrected - qx * gz_corrected + qz * gx_corrected);
    q_dot_z = 0.5f * ( qw * gz_corrected + qx * gy_corrected - qy * gx_corrected);

    q.w = qw + q_dot_w * dt;
    q.x = qx + q_dot_x * dt;
    q.y = qy + q_dot_y * dt;
    q.z = qz + q_dot_z * dt;

    quaternion_normalize(&q);
}

void imu_fusion_get_euler(float *roll, float *pitch, float *yaw) {
    float q_yaw_deg;
    quaternion_to_euler(&q, roll, pitch, &q_yaw_deg);

    // 应用外部修正（来自编码器反馈的长期漂移修正）
    float corrected_yaw = q_yaw_deg + external_yaw_correction;

    // 归一化到 -180 ~ 180
    while (corrected_yaw > 180.0f) corrected_yaw -= 360.0f;
    while (corrected_yaw < -180.0f) corrected_yaw += 360.0f;

    if (yaw) *yaw = corrected_yaw;
}

/**
 * @brief 用外部观测修正Yaw（如编码器航位推算）
 *
 * 注意: 四元数本身不能直接修正Yaw（因为没有磁力计观测），
 *       所以这里维护一个externel_yaw_correction偏移量，
 *       在get_euler时叠加到四元数Yaw上。
 *
 *       这样即使经过任意翻滚，四元数Yaw仍然正确跟踪姿态，
 *       外部修正仅用于补偿陀螺仪的长期积分漂移。
 *
 * @param yaw_observation 外部观测的Yaw角度 (度)
 */
void imu_fusion_correct_yaw(float yaw_observation) {
    float q_yaw_deg;
    quaternion_to_euler(&q, NULL, NULL, &q_yaw_deg);

    float corrected_yaw = q_yaw_deg + external_yaw_correction;

    // 归一化到 -180 ~ 180
    while (corrected_yaw > 180.0f) corrected_yaw -= 360.0f;
    while (corrected_yaw < -180.0f) corrected_yaw += 360.0f;

    // 计算误差 (外部观测 - 当前修正后Yaw)
    float error = yaw_observation - corrected_yaw;

    // 归一化误差
    while (error > 180.0f) error -= 360.0f;
    while (error < -180.0f) error += 360.0f;

    // 极慢修正 (增益 0.001, 约 2000步 = 4秒时间常数 @ 500Hz)
    external_yaw_correction += 0.001f * error;

    // 限制修正量范围，防止异常
    if (external_yaw_correction > 180.0f) external_yaw_correction = 180.0f;
    if (external_yaw_correction < -180.0f) external_yaw_correction = -180.0f;
}