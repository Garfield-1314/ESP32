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

from musicxml_parser import parse_musicxml


file_path = 'TKZZ.xml'# 解析 MusicXML 文件
pitches, durations, high, low, high_durations, low_durations = parse_musicxml(file_path)

# # 打印结果
# print("所有音符:", pitches)

print(durations)

map_midi_sequence = config.convert_notes(config.piano_map_list_2)
print("map_midi_sequence:",map_midi_sequence)


pitches_midi_sequence = config.convert_notes(pitches)
print("pitches:",map_midi_sequence)

high_midi_sequence = config.convert_notes(high)
print("high:",high_midi_sequence)

piano_start_point_x = 0
piano_start_point_y = 174
piano_start_point_z = 120
piano_start_point_e = 0

play_ready=-30
play_down_b=-70
play_down_w=-75

def set_point(flag,X,Y,Z,E):
    if flag == 1:
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

def play(flag,E,T):
    if flag == 1:
        set_point(1,-5,230,play_down_w,E)
        time.sleep_ms(T*150)
        set_point(1,-5,250,play_ready,E)
        time.sleep_ms(100)



def playb(flag,E,T):
    if flag == 1:
        set_point(1,-5,280,play_down_b+3,E)
        time.sleep_ms(T*150)
        set_point(1,-5,250,play_ready+3,E)
        time.sleep_ms(100)


def home_seting():
    data_to_send = "G28\r\n"
    print(data_to_send)
    uart1.write(data_to_send)
    while(True):
        if uart1.any():
            print(uart1.read())
            break


def go2point(strs,num,flag,X,Y,Z,E):
    set_point(flag,X,Y,Z,E)
    print(strs,num,flag,X,Y,Z,E)


def key_now(key_nom):
    for index in range(len(map_midi_sequence)):
        if pitches_midi_sequence[key_nom]  == map_midi_sequence[index]:
            # print(pitches[key_nom])
            return pitches_midi_sequence[key_nom],index

def key_last(key_nom):
    if key_nom > 0:
        for index in range(len(map_midi_sequence)):
            if pitches_midi_sequence[key_nom-1]  == map_midi_sequence[index]:
                # print(pitches[key_nom])
                return pitches_midi_sequence[key_nom-1],index
    else:
        return 0,0

def key_next(key_nom):
    if key_nom < len(pitches)-1:
        for index in range(len(map_midi_sequence)):
            if pitches_midi_sequence[key_nom+1]  == map_midi_sequence[index]:
                # print(pitches[key_nom])
                return pitches_midi_sequence[key_nom+1],index



def playpiano_1():
    arm1_point_last = 0
    arm2_point_last = 0
    for i in range(len(pitches)):
        tkey_now ,index_now = key_now(i)
        tkey_last ,index_last = key_last(i)
        tkey_next ,index_next = key_next(i)
        print(i)
        # print(index_last,index_now,index_next)

        dis1 = abs(index_now - arm1_point_last)
        dis2 = abs(index_now - arm2_point_last)
        # print(dis1,dis2,arm1_point_last,arm2_point_last)
        if i == 0:
            if tkey_now in high_midi_sequence:
                dish = int((config.piano_map_disE_list_mm[index_now]-(config.key_len*20))/2)
                go2point('START',i,1,-5,250,play_ready,E=dish)
                arm1_point_last = index_now
                if index_now in config.black_key:
                    # time.sleep_ms(1000)
                    playb(1,E=int((config.piano_map_disE_list_mm[index_now]-(config.key_len*20))/2),T=int(durations[index_now]))
                else:
                    # time.sleep_ms(1000)
                    play(1,E=int((config.piano_map_disE_list_mm[index_now]-(config.key_len*20))/2),T=int(durations[index_now]))

        else:
            if dis1 <= dis2 and index_now >= 35:
                while True:  
                    data_to_send = b"{}".format(999)
                    datas.send_packet(uart2, data_to_send)
                    time.sleep_ms(100)
                    if receiver.process(uart2):
                        received_data = receiver.packet
                        content = received_data.decode('utf-8') 
                        if content == 'OVER':
                            # print(content)    
                            time.sleep_ms(200) 
                            break
                dish = int((config.piano_map_disE_list_mm[index_now]-(config.key_len*20))/2)
                if dish < 0 :
                    dish = 200
                go2point('run1',i,1,-5,250,play_ready,E=dish)
                arm1_point_last = index_now
                arm2_point_last = 20
                if index_now in config.black_key:
                    # time.sleep_ms(1000)
                    playb(1,E=int((config.piano_map_disE_list_mm[index_now]-(config.key_len*20))/2),T=int(durations[index_now]))
                else:
                    # time.sleep_ms(1000)
                    play(1,E=int((config.piano_map_disE_list_mm[index_now]-(config.key_len*20))/2),T=int(durations[index_now]))
            else:
                go2point("run2",i,1,-4,250,play_ready,E=int((config.piano_map_disE_list_mm[61]-(config.key_len*20))/2))
                arm1_point_last = 61
                arm2_point_last = index_now
                data_to_send = b"{}".format(i)
                while True:
                    datas.send_packet(uart2, data_to_send)
                    time.sleep_ms(20)
                    if receiver.process(uart2):
                        received_data = receiver.packet
                        content = received_data.decode('utf-8')  
                        if content == 'OVER':
                            print(content)    
                            # time.sleep_ms(100) 
                            break




home_seting()
set_point(1,-5,200,play_ready,0)
time.sleep_ms(1000)


playpiano_1()

# set_point(1,-5,200,play_ready,0)
while True: 
    time.sleep_ms(100)
    # data_to_send = b"see"
    # datas.send_packet(uart2, data_to_send)
    # if receiver.process(uart2):
    #     received_data = receiver.packet
    #     receiver.reset()
    #     content = received_data.decode('utf-8')  
    #     print(content)
    # clock.tick()  # Update the FPS clock.
    # # img = sensor.snapshot()  # Take a picture and return the image.
    # print(clock.tick() ) 
