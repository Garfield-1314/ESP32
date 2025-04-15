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

receiver = datas.PacketReceiver()

print(config.P1)
measure = len(config.P1)
print(config.P1M)
print(config.P1N)

N = 0

for i in range(len(config.P1N)):
    if config.P1N[i] == 'eighth':
        config.P1N[i] = 1
    elif config.P1N[i] == 'quarter':
        config.P1N[i] = 2
    elif config.P1N[i] == 'half':
        config.P1N[i] = 4
    

print(config.P1N)

piano_start_point_x = 0
piano_start_point_y = 174
piano_start_point_z = 120
piano_start_point_e = 0

play_ready=-30
play_down_b=-70
play_down_w=-75

def set_point(X,Y,Z,E,F):
    data_to_send = "G1 X{} Y{} Z{} E{} F{}\r\n".format(X, Y, Z, E, F)
    while(True):
        uart1.write(data_to_send)
        time.sleep_ms(100)
        # print('amount')
        if uart1.any():
            data1 = uart1.read()  # 读取所有可用数据
            # if (b'INFO' in data1):
            print('data1:',data1)
            # dict_data, list_data = parse_gcode_move(data1)
            # if dict_data['X']!=0 or dict_data['Y']!=0 or dict_data['Z']!=0:
                # print("uart1:",dict_data['X'],dict_data['Y'],dict_data['Z'])
            break

def playw(E,T):
    set_point(0,250,play_down_w,E,150)
    time.sleep_ms(T*125)
    set_point(0,250,play_ready,E,150)
    time.sleep_ms(50)



def playb(E,T):
    set_point(0,320,play_down_b-20,E,150)
    time.sleep_ms(T*125)
    set_point(0,250,play_ready+3,E,150)
    time.sleep_ms(50)


def home_seting():
    data_to_send = "G28\r\n"
    print(data_to_send)
    uart1.write(data_to_send)
    while(True):
        if uart1.any():
            print(uart1.read())
            break


def go2point(X,Y,Z,E):
    set_point(X,Y,Z,E,0)
    print(X,Y,Z,E,0)


def callback():
    while True:
        data_to_send = b"Get"
        datas.send_packet(uart2, data_to_send)
        time.sleep_ms(10)
        if receiver.process(uart2):
            print('content:',receiver.packet)
            break 


def playpiano():
    num = 0
    for i in range(measure):
        print(i,config.P1[i])
        time.sleep_ms(500)
        # callback()
        for p in range(config.P1[i]):
            print(num,config.P1M[num],config.P1N[num])
            if config.P1M[num] == "R":
                go2point(0,250,play_ready,E=int(200)/2)
                time.sleep_ms(config.P1N[num]*125)
                time.sleep_ms(250)
                print("sleep")
            else :
                for index in range(len(config.piano_map_list_2)):
                    if config.P1M[num] == config.piano_map_list_2[index]:
                        go2point(0,250,play_ready,E=int((config.piano_map_disE_list_mm[index]-(config.key_len*20))/2))
                        # while True:
                        #     print(1)
                        if index in config.black_key:
                            # time.sleep_ms(1000)
                            playb(E=int((config.piano_map_disE_list_mm[index]-(config.key_len*20))/2),T=int(config.P1N[num]))
                            break
                        else:
                            # time.sleep_ms(1000)
                            playw(E=int((config.piano_map_disE_list_mm[index]-(config.key_len*20))/2),T=int(config.P1N[num]))
                            break
            num = num + 1


home_seting()

for i in range(20):
    time.sleep_ms(1000)

while True:
    if receiver.process(uart2):
        if receiver.packet == bytearray(b'Init OVER'):
            print('content:',receiver.packet)
            data_to_send = b"OK"
            datas.send_packet(uart2, data_to_send)
            time.sleep_ms(10)
            datas.send_packet(uart2, data_to_send)
            time.sleep_ms(10)
            datas.send_packet(uart2, data_to_send)
            break
        
playpiano()

set_point(0,174,120,0,0)
