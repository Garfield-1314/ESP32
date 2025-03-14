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
uart2 = UART(1, 115200, timeout_char=1)

uart1 = UART(3, 115200, timeout_char=1)

receiver = datas.PacketReceiver()

from musicxml_parser import parse_musicxml


file_path = 'TKZZ.xml'# 解析 MusicXML 文件
pitches, durations, high, low, high_durations, low_durations = parse_musicxml(file_path)

# 打印结果
print("所有音符:", pitches)


map_midi_sequence = config.convert_notes(config.piano_map_list_2)
print("map_midi_sequence:",map_midi_sequence)


pitches_midi_sequence = config.convert_notes(pitches)
print("pitches:",map_midi_sequence)

piano_start_point_x = 0
piano_start_point_y = 174
piano_start_point_z = 120
piano_start_point_e = 0

play_ready=-30
play_down_b=-70
play_down_w=-75

def set_point(X,Y,Z,E):
    data_to_send = "G1 X{} Y{} Z{} E{}\r\n".format(X, Y, Z, E)
    while(True):
        uart1.write(data_to_send)
        time.sleep_ms(100)
        if uart1.any():
            # time.sleep_ms(200)
            data1 = uart1.read()  # 读取所有可用数据
            if (b'MOVE' in data1):
                print('data1:',data1)
                # dict_data, list_data = parse_gcode_move(data1)
                # if dict_data['X']!=0 or dict_data['Y']!=0 or dict_data['Z']!=0:
                    # print("uart1:",dict_data['X'],dict_data['Y'],dict_data['Z'])
                break

def ready2play(E):
    set_point(0,174,-45,E)

def play(E,T):
    set_point(0,174,play_down_w,E)
    time.sleep_ms(T*150)
    set_point(0,174,-45,E)
    time.sleep_ms(100)
    # print("play")

def playb(E,T):
    set_point(0,220,play_down_b,E)
    time.sleep_ms(T*150)
    set_point(0,220,-35,E)
    time.sleep_ms(100)
    # print("play")



def home_seting():
   data_to_send = "G28\r\n"
   print(data_to_send)
   uart1.write(data_to_send)
   while(True):
    if uart1.any():
        print(uart1.read())
        break


def go2point(strs,num,flag,X,Y,Z,E):
    set_point(X,Y,Z,E)
    print(strs,num,flag,X,Y,Z,E)

def playpiano(number):
    pitches_now = config.pitches2[number]
    for index in range(len(config.piano_map_list_2)):
        if pitches_now == config.piano_map_list_2[index]:
            go2point("G",0,number,-4,200,play_ready,E=int((config.piano_map_disE_list_mm[index]-(config.key_len*11))/2))
            if index in config.black_key:
                time.sleep_ms(1000)
                # playb(E=int((config.piano_map_disE_list_mm[index]-(config.key_len*11))/2),T=int(durations[number]))
                data_to_send = b"OVER"
                for i in range(3):
                    datas.send_packet(uart2, data_to_send)
                    time.sleep_ms(100)
                break
            else:
                time.sleep_ms(1000)
                # play(E=int((config.piano_map_disE_list_mm[index]-(config.key_len*11))/2),T=int(durations[number]))
                data_to_send = b"OVER"
                for i in range(3):
                    datas.send_packet(uart2, data_to_send)
                    time.sleep_ms(100)
                break

# playpiano()
# home_seting()

set_point(0,200,play_ready,0)


while True:
    if receiver.process(uart2):
        content = 0
        received_data = receiver.packet
        receiver.reset() 
        content = int(received_data.decode('utf-8'))  # 结果：'999'
        # print(content)    
        if content != None and content<=len(pitches):
            playpiano(content)
        elif  content == 999:
            go2point("G",0,0,-4,200,play_ready,E=int(100)/2)
            data_to_send = b"OVER"
            for i in range(3):
                datas.send_packet(uart2, data_to_send)
                time.sleep_ms(100)
set_point(0,174,120,0)

while True: 
    time.sleep_ms(100)
    # data_to_send = b"back"
    # datas.send_packet(uart2, data_to_send)
    # if receiver.process(uart2):
    #     received_data = receiver.packet
    #     receiver.reset()
    #     content = received_data.decode('utf-8')  
    #     print(content)
    # clock.tick()  # Update the FPS clock.
    # img = sensor.snapshot()  # Take a picture and return the image.
    # print(clock.tick() ) 
