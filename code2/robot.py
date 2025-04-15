import time
from pyb import UART
import sensor
import time
import datapacket as datas
import config
import math

sensor.reset()  # Reset and initialize the sensor.
sensor.set_pixformat(sensor.RGB565)  # Set pixel format to RGB565 (or GRAYSCALE)
sensor.set_framesize(sensor.QVGA)  # Set frame size to QVGA (320x240)
sensor.skip_frames(time=200)  # Wait for settings take effect.
sensor.set_windowing((120,96))
clock = time.clock()  # Create a clock object to track the FPS.
# UART 3, and baudrate.
uart2 = UART(1, 115200, timeout_char=1)

uart1 = UART(3, 115200, timeout_char=1)

open = 1
close = 0

thresholds = [
    (0, 60, 13, 127, -128, 127),  # generic_red_thresholds
    (0, 100, -128, 11, 38, 127),
]  # generic_blue_thresholds

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

# home_seting()
time.sleep_ms(1000)
time.sleep_ms(1000)
# i = 0
# while True:
#     clock.tick()
#     img = sensor.snapshot()
#     for blob in img.find_blobs(thresholds, pixels_threshold=200, area_threshold=200):
#         # These values depend on the blob not being circular - otherwise they will be shaky.

#         # These values are stable all the time.
#         img.draw_rectangle(blob.rect())
#         img.draw_cross(blob.cx(), blob.cy())
#         # Note - the blob rotation is unique to 0-180 only.
#     i = i + 1
#     if i == 200:
#         break
#     print(clock.fps())




def git_obj():
    for i in range(100000):
        img = sensor.snapshot()
        time.sleep_us(1)
        if i < 5:
            set_point(0,172,120,0)
        elif i > 5 and i < 10:
            paw(open)
        elif i > 10 and i < 15:
            set_point(45,170,-100,0)
        elif i > 15 and i < 20:
            paw(close)
        elif i > 20 and i < 25:
            set_point(-52,255,-20,0)
        elif i > 25 and i < 30:
            set_point(-52,255,-80,0)
        elif i > 30 and i < 35:
            paw(open)
        elif i > 35 and i < 40:
            set_point(0,172,120,0)
            paw(close)
        elif i > 60 and i < 65:
            set_point(0,172,120,0)
        elif i > 65 and i < 70:
            paw(open)
        elif i > 70 and i < 75:
            set_point(-52,170,-100,0)
        elif i > 75 and i < 80:
            paw(close)
        elif i > 80 and i < 85:
            set_point(45,260,-20,0)
        elif i > 85 and i < 90:
            set_point(45,260,-80,0)
        elif i > 90 and i < 95:
            paw(open)
        elif i > 95 and i < 100:
            set_point(0,172,120,0)
            paw(close)
            while 1:
                img = sensor.snapshot()
            #     img = sensor.snapshot()
        print(i)



def git_obj():
    for i in range(100000):
        img = sensor.snapshot()
        time.sleep_us(1)
        if i < 5:
            set_point(0,172,120,0)
        elif i > 5 and i < 10:
            paw(open)
        elif i > 10 and i < 15:
            set_point(45,170,-100,0)
        elif i > 15 and i < 20:
            paw(close)
        elif i > 20 and i < 25:
            set_point(-52,255,-20,0)
        elif i > 25 and i < 30:
            set_point(-52,255,-80,0)
        elif i > 30 and i < 35:
            paw(open)
        elif i > 35 and i < 40:
            set_point(0,172,120,0)
            paw(close)
        elif i > 60 and i < 65:
            set_point(0,172,120,0)
        elif i > 65 and i < 70:
            paw(open)
        elif i > 70 and i < 75:
            set_point(-52,170,-100,0)
        elif i > 75 and i < 80:
            paw(close)
        elif i > 80 and i < 85:
            set_point(45,260,-20,0)
        elif i > 85 and i < 90:
            set_point(45,260,-80,0)
        elif i > 90 and i < 95:
            paw(open)
        elif i > 95 and i < 100:
            set_point(0,172,120,0)
            paw(close)
            while 1:
                img = sensor.snapshot()
            #     img = sensor.snapshot()
        print(i)



def git_obj2():
    for i in range(100000):
        img = sensor.snapshot()
        time.sleep_us(1)
        if i < 5:
            set_point(0,172,120,0)
        elif i > 5 and i < 10:
            paw(open)
        elif i > 10 and i < 15:
            set_point(0,220,-110,0)
        elif i > 15 and i < 20:
            paw(close)
        elif i > 20 and i < 25:
            set_point(0,172,120,0)
            while 1:
                img = sensor.snapshot()
        print(i)

while True:
    git_obj2()

    # git_obj()
    # while 1:
    #     a = 1
    # print(clock.tick() )
