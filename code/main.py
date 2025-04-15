import time
from pyb import UART
import sensor
import time
import datapacket as datas
import config

sensor.reset()  # Reset and initialize the sensor.
sensor.set_pixformat(sensor.RGB565)  # Set pixel format to RGB565 (or GRAYSCALE)
sensor.set_framesize(sensor.QVGA)  # Set frame size to QVGA (320x240)
sensor.skip_frames(time=20)  # Wait for settings take effect.
clock = time.clock()  # Create a clock object to track the FPS.
# UART 3, and baudrate.
uart2 = UART(1, 115200, timeout_char=5)

uart1 = UART(3, 115200, timeout_char=5)
    
receiver = datas.PacketReceiver()


print(config.P2)
measure = len(config.P2)
print(config.P2M)
print(config.P2N)

N = 0

for i in range(len(config.P2N)):
    if config.P2N[i] == 'eighth':
        config.P2N[i] = 1
    elif config.P2N[i] == 'quarter':
        config.P2N[i] = 2
    elif config.P2N[i] == 'half':
        config.P2N[i] = 4
    

print(config.P2N)


play_ready=-30
play_down_b=-55
play_down_w=-65

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

def ready2play(E):
    set_point(148,174,-15,E,150)

def playw(E,T):
    set_point(148,174,play_down_w+9,E,150)
    time.sleep_ms(T*125)
    set_point(148,174,-15,E,150)
    time.sleep_ms(50)

def playb(E,T):
    set_point(148,220,play_down_b+16,E,150)
    time.sleep_ms(T*125)
    set_point(148,220,-15,E,150)
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
        print(i,config.P2[i])
        time.sleep_ms(250)
        # callback()
        for p in range(config.P2[i]):
            print(num,config.P2M[num],config.P2N[num])
            if config.P2M[num] == "R":
                go2point(148,174,play_ready,E=int(100)/2)
                time.sleep_ms(config.P2N[num]*125)
                time.sleep_ms(500)
                print("sleep")
            else :
                for index in range(len(config.piano_map_list_2)):
                    if config.P2M[num] == config.piano_map_list_2[index]:
                        go2point(148,174,play_ready,E=int((config.piano_map_disE_list_mm[index]-(config.key_len*11))/2))
                        if index in config.black_key:
                            # time.sleep_ms(1000)
                            playb(E=int((config.piano_map_disE_list_mm[index]-(config.key_len*11))/2),T=int(config.P2N[num]))
                            break
                        else:
                            # time.sleep_ms(1000)
                            playw(E=int((config.piano_map_disE_list_mm[index]-(config.key_len*11))/2),T=int(config.P2N[num]))
                            break
            num = num + 1

home_seting()

for i in range(20):
    time.sleep_ms(1000)

while True:
    data_to_send = b"Init OVER"
    datas.send_packet(uart2, data_to_send)
    time.sleep_ms(10)
    if receiver.process(uart2):
        if receiver.packet == bytearray(b"OK"):
            print('content:',receiver.packet)
            break 


playpiano()


set_point(0,174,120,0,0)



