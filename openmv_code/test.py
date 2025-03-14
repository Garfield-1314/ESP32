import os
import time
from pyb import UART
import sensor
import time
sensor.reset()  # Reset and initialize the sensor.
sensor.set_pixformat(sensor.RGB565)  # Set pixel format to RGB565 (or GRAYSCALE)
sensor.set_framesize(sensor.QVGA)  # Set frame size to QVGA (320x240)
sensor.skip_frames(time=20)  # Wait for settings take effect.
clock = time.clock()  # Create a clock object to track the FPS.
# UART 3, and baudrate.
uart2 = UART(1, 115200, timeout_char=1)

uart1 = UART(3, 115200, timeout_char=100)

# 假设你已经将 XML 数据以文本方式存储在文件中
# 下面是读取文件并打印音高与时值的代码

# 打开 MusicXML 文件（在 OpenMV 中，文件应当放置在 sd 卡或者文件系统中）
try:
    with open('TKZZ.xml', 'r') as file:  # 请确保路径正确
        xml_content = file.read()
except Exception as e:
    print("无法打开文件:", e)
    xml_content = ''

# 简单的字符串处理来解析音符和时值
# 这个方法相对简单，不如 XML 解析器强大，但能在 OpenMV 上工作

pitches = []
durations = []

# 用来存储高音和低音及其时值
high = []
low = []
high_durations = []
low_durations = []

if xml_content:
    # 查找所有音符的音高和时值
    start_index = 0
    while True:
        # 查找音符的 pitch 和 duration 标签
        pitch_start = xml_content.find('<pitch>', start_index)
        if pitch_start == -1:
            break
        pitch_end = xml_content.find('</pitch>', pitch_start)
        pitch_data = xml_content[pitch_start + 7:pitch_end]
        
        # 获取音高的 step, octave 和 alter
        step_start = pitch_data.find('<step>') + 6
        step_end = pitch_data.find('</step>', step_start)
        step = pitch_data[step_start:step_end]
        
        octave_start = pitch_data.find('<octave>') + 8
        octave_end = pitch_data.find('</octave>', octave_start)
        octave = int(pitch_data[octave_start:octave_end])
        
        alter_start = pitch_data.find('<alter>') + 7
        alter_end = pitch_data.find('</alter>', alter_start)
        alter = pitch_data[alter_start:alter_end] if alter_start != -1 else None
        
        # 处理音高
        alter_sign = '#' if alter == '1' else 'b' if alter == '-1' else ''
        pitch_info = f"{step}{alter_sign}{octave}"
        pitches.append(pitch_info)

        # 查找音符的 duration
        duration_start = xml_content.find('<duration>', pitch_end) + 10
        duration_end = xml_content.find('</duration>', duration_start)
        duration = xml_content[duration_start:duration_end]
        durations.append(duration)

        # 根据音高的音阶判断高音或低音
        if octave >= 4:  # 认为 C5 及以上是高音
            high.append(pitch_info)
            high_durations.append(duration)
        elif octave < 4:  # 认为 C3 及以下是低音
            low.append(pitch_info)
            low_durations.append(duration)

        # 更新索引，继续查找下一个音符
        start_index = pitch_end + 1

# 打印音高、时值，高音、低音及其时值
print("所有音高:")
print(pitches)
print(len(pitches))

# print("\n所有时值:")
# print(durations)

# print("\n高音:")
# print(high)

# print("\n低音:")
# print(low)

# print("\n高音时值:")
# print(high_durations)

# print("\n低音时值:")
# print(low_durations)



print(piano_map_disE_list_mm)


def convert_note_to_weight(note):
    num_index = 0
    # 找到数字部分的起始位置
    while num_index < len(note) and not note[num_index].isdigit():
        num_index += 1
    
    # 分离字母/升号和数字部分
    note_part = note[:num_index]
    number = int(note[num_index:]) if num_index < len(note) else 0
    
    # 计算字母权值 (A=1, B=2,...)
    letter = note_part[0].upper()
    letter_value = ord(letter) - ord('A') + 1
    
    # 检查升号
    sharp = 1 if '#' in note_part else 0
    
    return letter_value + sharp + number










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
                    print('data1:',data1)
                    time.sleep_ms(200)
                    break

def play(flag,E,T):
    if flag == 1:
        set_point(1,-5,230,play_down_w,E)
        time.sleep_ms(T*250)
        set_point(1,-5,250,play_ready,E)
        time.sleep_ms(100)



def playb(flag,E,T):
    if flag == 1:
        set_point(1,-5,280,play_down_b+3,E)
        time.sleep_ms(T*250)
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


def go2point(flag,X,Y,Z,E):
    set_point(flag,X,Y,Z,E)
    print(flag,X,Y,Z,E)


def key_now(key_nom):
    for index in range(len(piano_map_list_2)):
        if pitches[key_nom] in pitches:
            # print(pitches[key_nom])
            return pitches[key_nom]

def key_last(key_nom):
    if key_nom > 0:
        for index in range(len(piano_map_list_2)):
                if pitches[key_nom-1] in pitches:
                    # print(pitches[key_nom-1])
                    return pitches[key_nom-1]

def key_next(key_nom):
    if key_nom < len(pitches):
        for index in range(len(piano_map_list_2)):
            if pitches[key_nom+1] in pitches:
                # print(pitches[key_nom+1])
                return pitches[key_nom+1]
                

def playpiano_1():
    for i in range(len(pitches)):
        tkey_now = key_now(i)
        tkey_last = key_last(i)
        tkey_next= key_next(i)
        print(tkey_last,tkey_now,tkey_next)


# home_seting()
# set_point(1,-5,200,play_ready,0)
# time.sleep_ms(1000)


playpiano_1()

# set_point(1,-5,200,play_ready,0)
while True: 
    clock.tick()  # Update the FPS clock.
    img = sensor.snapshot()  # Take a picture and return the image.
