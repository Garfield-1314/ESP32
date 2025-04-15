import time
from pyb import UART
import sensor
import time
import datapacket as datas
import config

sensor.reset()  # Reset and initialize the sensor.
sensor.set_pixformat(sensor.RGB565)  # Set pixel format to RGB565 (or GRAYSCALE)
sensor.set_framesize(sensor.QVGA)  # Set frame size to QVGA (320x240)
sensor.skip_frames(time=200)  # Wait for settings take effect.
clock = time.clock()  # Create a clock object to track the FPS.
# UART 3, and baudrate.
uart2 = UART(1, 115200, timeout_char=1)

uart1 = UART(3, 115200, timeout_char=1)

open = 1
close = 0


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
    while(True):
        uart1.write(data_to_send1)
        time.sleep_ms(100)
        if uart1.any():
            # time.sleep_ms(200)
            data1 = uart1.read()  # 读取所有可用数据
            if (b'MOVE' in data1):
                # print('data1:',data1)
                time.sleep_ms(200)
                break

def home_seting():
    data_to_send = "G28\r\n"
    print(data_to_send)
    uart1.write(data_to_send)
    while(True):
        if uart1.any():
            print(uart1.read())
            break


def git_obj():
    for i in range(100000):
        img = sensor.snapshot() 
        time.sleep_us(1)
        if i < 5:
            set_point(0,172,120,0)
        elif i > 5 and i < 10:
            paw(close)
        elif i > 10 and i < 15:
            set_point(0,220,-110,0)
        elif i > 15 and i < 20:
            paw(open)
        elif i > 20 and i < 25:
            set_point(-150,172,20,0)
        elif i > 25 and i < 30:
            set_point(-150,172,-10,0)
        elif i > 30 and i < 35:
            paw(close)
        elif i > 35 and i < 40:
            set_point(0,172,120,0)
            paw(open)
            while 1:
                img = sensor.snapshot() 
        print(i)

# home_seting()


while True: 
    git_obj()
    # while 1:
    #     a = 1 
    # print(clock.tick() ) 
