import time
from pyb import UART
import sensor
import time
import datapacket as datas
import config

sensor.reset()  # Reset and initialize the sensor.
sensor.set_pixformat(sensor.RGB565)  # Set pixel format to RGB565 (or GRAYSCALE)
sensor.set_framesize(sensor.QQVGA)  # Set frame size to QVGA (320x240)
sensor.skip_frames(time=200)  # Wait for settings take effect.
clock = time.clock()  # Create a clock object to track the FPS.
# UART 3, and baudrate.
uart2 = UART(1, 115200, timeout_char=1)

uart1 = UART(3, 115200, timeout_char=1)

open = 1
close = 0


class PID:
    def __init__(self, Kp, Ki, Kd, output_limits=(None, None)):
        self.Kp = Kp
        self.Ki = Ki
        self.Kd = Kd
        self.output_min, self.output_max = output_limits

        self.reset()  # 调用reset初始化所有状态变量

    def reset(self):
        """重置控制器状态"""
        self.integral = 0
        self.last_error = 0
        self.last_time = time.ticks_ms()
        self.last_measured_value = 0  # 添加这个初始化

    def compute(self, setpoint, measured_value):
        """计算PID输出"""
        now = time.ticks_ms()
        dt = time.ticks_diff(now, self.last_time) / 1000.0

        error = setpoint - measured_value

        P = self.Kp * error

        self.integral += error * dt
        I = self.Ki * self.integral

        # 使用安全的微分项计算
        if dt > 0:
            derivative = -(measured_value - self.last_measured_value) / dt
        else:
            derivative = 0

        D = self.Kd * derivative

        output = P + I + D

        # 应用输出限幅
        if self.output_min is not None and output < self.output_min:
            output = self.output_min
            self.integral = self.integral  # 抗饱和
        elif self.output_max is not None and output > self.output_max:
            output = self.output_max
            self.integral = self.integral  # 抗饱和

        # 更新状态变量
        self.last_error = error
        self.last_measured_value = measured_value
        self.last_time = now

        return output
# 使用示例
pidx = PID(Kp=1.0, Ki=0.0, Kd=0.0, output_limits=(-1, 1))
pidy = PID(Kp=1.0, Ki=0.0, Kd=0.0, output_limits=(-1, 1))

X_at = 80  # 目标值
Y_at = 60  # 目标值

# while True:
#     output = pid.compute(setpoint, measured_value)

#     # 这里应添加实际控制系统的执行逻辑
#     # 例如：控制加热器PWM、电机速度等
#     # 然后获取新的测量值（这里用伪代码示例）
#     # applied_output(output)
#     # measured_value = read_sensor()

#     time.sleep(0.1)  # 控制周期100ms

def electromagnet(flag):
    if flag == close:
        data_to_send1 = "M1\r\n"
        uart1.write(data_to_send1)
    else:
        data_to_send1 = "M2\r\n"
        uart1.write(data_to_send1)

def paw(flag):
    if flag == close:
        data_to_send1 = "M5\r\n"
        uart1.write(data_to_send1)
    else:
        data_to_send1 = "M3\r\n"
        uart1.write(data_to_send1)



def set_point(X,Y,Z,E):
    data_to_send1 = "G1 X{} Y{} Z{} E{}\r\n".format(X, Y, Z, E)
    uart1.write(data_to_send1)
    time.sleep_ms(100)

def home_seting():
    data_to_send = "G28\r\n"
    print(data_to_send)
    uart1.write(data_to_send)
    while(True):
        if uart1.any():
            print(uart1.read())
            break

home_seting()



thresholds = [
(0, 36, -128, 12, -128, 127)
]  # generic_blue_thresholds

x = 0
y = 172



while True:
    clock.tick()
    img = sensor.snapshot()


    for blob in img.find_blobs(thresholds, pixels_threshold=200, area_threshold=200):
        # These values depend on the blob not being circular - otherwise they will be shaky.
        if blob.elongation() > 0.5:
            img.draw_cross(blob.cx(), blob.cy(),color=(255, 0, 0))
            outputX = pidx.compute(X_at,blob.x())
            outputY = pidy.compute(Y_at,blob.y())
            print(outputX,outputY)
            x = x+outputX
            y = y-outputY
            set_point(x,y,0,0)
    # print("FPS %f" % clock.fps())
