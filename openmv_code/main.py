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
uart2 = UART(1, 115200, timeout_char=20)

uart1 = UART(3, 115200, timeout_char=20)

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


#钢琴长122.5cm
#白键52个
#黑键36个
# piano_map_list = [
#     "A0", "Bb0", "B0",
#     "C1", "Db1", "D1", "Eb1", "E1", "F1", "Gb1", "G1", "Ab1", "A1", "Bb1", "B1",    #C1
#     "C2", "Db2", "D2", "Eb2", "E2", "F2", "Gb2", "G2", "Ab2", "A2", "Bb2", "B2",    #C2  
#     "C3", "Db3", "D3", "Eb3", "E3", "F3", "Gb3", "G3", "Ab3", "A3", "Bb3", "B3",    #C3
#     "C4", "Db4", "D4", "Eb4", "E4", "F4", "Gb4", "G4", "Ab4", "A4", "Bb4", "B4",    #C4
#     "C5", "Db5", "D5", "Eb5", "E5", "F5", "Gb5", "G5", "Ab5", "A5", "Bb5", "B5",    #C5
#     "C6", "Db6", "D6", "Eb6", "E6", "F6", "Gb6", "G6", "Ab6", "A6", "Bb6", "B6",    #C6
#     "C7", "Db7", "D7", "Eb7", "E7", "F7", "Gb7", "G7", "Ab7", "A7", "Bb7", "B7",    #C7
#     "C8"                                                                            #C8
# ]


piano_map_list_2 = [
    "A0", "A#0", "B0",
    "C1", "C#1", "D1", "D#1", "E1", "F1", "F#1", "G1", "G#1", "A1", "A#1", "B1",    # C1
    "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2",      # C2
    "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3",      # C3
    "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4",      # C4
    "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5",      # C5
    "C6", "C#6", "D6", "D#6", "E6", "F6", "F#6", "G6", "G#6", "A6", "A#6", "B6",      # C6
    "C7", "C#7", "D7", "D#7", "E7", "F7", "F#7", "G7", "G#7", "A7", "A#7", "B7",      # C7
    "C8"                                                                              # C8
]

black_key = [1,4,6,9,
            11,13,16,18,
            21,23,25,28,
            30,33,35,37,
            40,42,45,47,49,
            52,54,57,59,
            61,64,66,69,
            71,73,76,78,
            81,82,84]

key_len=23.7

piano_map_disE_list_mm = [
    # A₀组（键1-3）
    key_len*0,    # 1. A₀ (白键)
    (key_len*0+key_len*1)/2,   # 2. A♯₀/B♭₀ (黑键)
    key_len*1,   # 3. B₀ (白键)
    
    # 第1八度：C₁→B₁（键4-15）
    key_len*2,   # 4. C₁ (白键)
    (key_len*2+key_len*3)/2,   # 5. C♯₁/D♭₁ (黑键)
    key_len*3,   # 6. D₁ (白键)
    (key_len*3+key_len*4)/2,   # 7. D♯₁/E♭₁ (黑键)
    key_len*4,   # 8. E₁ (白键)
    key_len*5,  # 9. F₁ (白键)
    (key_len*5+key_len*6)/2,  # 10. F♯₁/G♭₁ (黑键)
    key_len*6,  # 11. G₁ (白键)
    (key_len*6+key_len*7)/2,  # 12. G♯₁/A♭₁ (黑键)
    key_len*7,  # 13. A₁ (白键)
    (key_len*7+key_len*8)/2,  # 14. A♯₁/B♭₁ (黑键)
    key_len*8,  # 15. B₁ (白键),
    
    # 第2八度：C₂→B₂（键16-27）
    key_len*9,  # 16. C₂ (白键)
    (key_len*9+key_len*10)/2,  # 17. C♯₂/D♭₂ (黑键)
    key_len*10,  # 18. D₂ (白键)
    (key_len*10+key_len*11)/2,  # 19. D♯₂/E♭₂ (黑键)
    key_len*11,  # 20. E₂ (白键)
    key_len*12,  # 21. F₂ (白键)
    (key_len*12+key_len*13)/2,  # 22. F♯₂/G♭₂ (黑键)
    key_len*13,  # 23. G₂ (白键)
    (key_len*13+key_len*14)/2,  # key_len. G♯₂/A♭₂ (黑键)
    key_len*14,  # 25. A₂ (白键)
    (key_len*14+key_len*15)/2,  # 26. A♯₂/B♭₂ (黑键)
    key_len*15,  # 27. B₂ (白键),
    
    # 第3八度：C₃→B₃（键28-39）
    key_len*16,  # 28. C₃ (白键)
    (key_len*16+key_len*17)/2,  # 29. C♯₃/D♭₃ (黑键)
    key_len*17,  # 30. D₃ (白键)
    (key_len*17+key_len*18)/2,  # 31. D♯₃/E♭₃ (黑键)
    key_len*18,  # 32. E₃ (白键)
    key_len*19,  # 33. F₃ (白键)
    (key_len*19+key_len*20)/2,  # 34. F♯₃/G♭₃ (黑键)
    key_len*20,  # 35. G₃ (白键)
    (key_len*20+key_len*21)/2,  # 36. G♯₃/A♭₃ (黑键)
    key_len*21,  # 37. A₃ (白键)
    (key_len*21+key_len*22)/2,  # 38. A♯₃/B♭₃ (黑键)
    key_len*22,  # 39. B₃ (白键),
    
    # 第4八度：C₄→B₄（中央C在此！键40-51）
    key_len*23,  # 40. C₄ (中央C, 白键)
    (key_len*23+key_len*24)/2,  # 41. C♯₄/D♭₄ (黑键)
    key_len*24,  # 42. D₄ (白键)
    (key_len*24+key_len*25)/2,  # 43. D♯₄/E♭₄ (黑键)
    key_len*25,  # 44. E₄ (白键)
    key_len*26,  # 45. F₄ (白键)
    (key_len*26+key_len*27)/2,  # 46. F♯₄/G♭₄ (黑键)
    key_len*27,  # 47. G₄ (白键)
    (key_len*27+key_len*28)/2,  # 48. G♯₄/A♭₄ (黑键)
    key_len*28,  # 49. A₄ (白键)
    (key_len*28+key_len*29)/2,  # 50. A♯₄/B♭₄ (黑键)
    key_len*29,  # 51. B₄ (白键),
    
    # 第5八度：C₅→B₅（键52-63）
    key_len*30,  # 52. C₅ (白键)
    (key_len*30+key_len*31)/2,  # 53. C♯₅/D♭₅ (黑键)
    key_len*31,  # 54. D₅ (白键) 
    (key_len*31+key_len*32)/2,  # 55. D♯₅/E♭₅ (黑键)
    key_len*32,  # 56. E₅ (白键)
    key_len*33,  # 57. F₅ (白键)
    (key_len*33+key_len*34)/2,  # 58. F♯₅/G♭₅ (黑键)
    key_len*34,  # 59. G₅ (白键)
    (key_len*34+key_len*35)/2,  # 60. G♯₅/A♭₅ (黑键)
    key_len*35,  # 61. A₅ (白键)
    (key_len*35+key_len*36)/2,  # 62. A♯₅/B♭₅ (黑键)
    key_len*36,  # 63. B₅ (白键),
    
    # 第6八度：C₆→B₆（键64-75）
    key_len*37,  # 64. C₆ (白键)
    (key_len*37+key_len*38)/2,  # 65. C♯₆/D♭₆ (黑键)
    key_len*38,  # 66. D₆ (白键)
    (key_len*38+key_len*39)/2,  # 67. D♯₆/E♭₆ (黑键)
    key_len*39,  # 68. E₆ (白键)
    key_len*40,  # 69. F₆ (白键)
    (key_len*40+key_len*41)/2,  # 70. F♯₆/G♭₆ (黑键)
    key_len*41,  # 71. G₆ (白键)
    (key_len*41+key_len*42)/2,  # 72. G♯₆/A♭₆ (黑键)
    key_len*42,  # 73. A₆ (白键)
    (key_len*42+key_len*43)/2,  # 74. A♯₆/B♭₆ (黑键)
    key_len*43,  # 75. B₆ (白键),
    
    # 第7八度：C₇→B₇（键76-87）
    key_len*44,  # 76. C₇ (白键)
    (key_len*44+key_len*45)/2,  # 77. C♯₇/D♭₇ (黑键)
    key_len*45,  # 78. D₇ (白键)
    (key_len*45+key_len*46)/2, # 79. D♯₇/E♭₇ (黑键)
    key_len*46, # 80. E₇ (白键)
    key_len*47, # 81. F₇ (白键)
    (key_len*47+key_len*48)/2, # 82. F♯₇/G♭₇ (黑键)
    key_len*48, # 83. G₇ (白键) → 修正为1050（原计算错误）
    (key_len*48+key_len*49)/2, # 84. G♯₇/A♭₇ (黑键)
    key_len*49, # 85. A₇ (白键)
    (key_len*49+key_len*50)/2, # 86. A♯₇/B♭₇ (黑键)
    key_len*50, # 87. B₇ (白键),
    
    # 最高音C₈（键88）
    key_len*51  # 88. C₈ (白键)
]

print(piano_map_disE_list_mm)

piano_start_point_x = 0
piano_start_point_y = 174
piano_start_point_z = 120
piano_start_point_e = 0

play_ready=-30
play_down_b=-70
play_down_w=-75


def parse_gcode_move(data):
    line = data.decode('utf-8').strip()
    start = line.find('[') + 1
    end = line.find(']', start)
    
    # 同时返回字典和有序列表
    coord_dict = {}
    coord_list = []
    
    for param in line[start:end].split():
        key, val = param.split(':')
        coord_dict[key] = float(val)
        coord_list.append(float(val))  # 按出现顺序存储
    
    return coord_dict, coord_list

# 使用示例
data = b'INFO: LINEAR MOVE: [X:-5.00 Y:230.00 Z:-75.00 E:118.00]\r\n'
dict_data, list_data = parse_gcode_move(data)


def set_point(flag,X,Y,Z,E):
    if flag == 1:
        data_to_send1 = "G1 X{} Y{} Z{} E{}\r\n".format(X, Y, Z, E)
        while(True):
            uart1.write(data_to_send1)
            uart1.write(data_to_send1)
            uart1.write(data_to_send1)
            time.sleep_ms(100)
            if uart1.any():
                # time.sleep_ms(200)
                data1 = uart1.read()  # 读取所有可用数据
                if (b'MOVE' in data1) and  (b']\r\n' in data1) and (len(data1)<70 and len(data1)>50):
                    print('data1:',data1)
                    # dict_data, list_data = parse_gcode_move(data1)
                    # if dict_data['X']!=0 or dict_data['Y']!=0 or dict_data['Z']!=0:
                        # print("uart1:",dict_data['X'],dict_data['Y'],dict_data['Z'])
                    break
    elif flag == 2:
        data_to_send2= "G1 X{} Y{} Z{} E{}\r\n".format(X, Y, Z, E)
        while(True):
            uart2.write(data_to_send2)
            uart2.write(data_to_send2)
            uart2.write(data_to_send2)
            time.sleep_ms(100)
            if uart2.any():
                # time.sleep_ms(200)
                data2 = uart2.read()  # 读取所有可用数据       
                if (b'MOVE' in data2) and (b']\r\n' in data2) and (len(data2)<70 and len(data2)>50):
                    print('data2:',data2)
                    # dict_data2, list_data2 = parse_gcode_move(data2)
                    # if dict_data2['X']!=0 or dict_data2['Y']!=0 or dict_data2['Z']!=0:
                        # print("uart2:",dict_data2['X'],dict_data2['Y'],dict_data2['Z'])
                    break

def play(flag,E,T):
    if flag == 1:
        set_point(1,-5,230,play_down_w,E)
        time.sleep_ms(T*250)
        set_point(1,-5,250,play_ready,E)
        time.sleep_ms(100)
    elif flag == 2:
        set_point(2,-4,174,play_down_w+10,E)
        time.sleep_ms(T*250)
        set_point(2,-4,174,play_ready,E)
        time.sleep_ms(100)


def playb(flag,E,T):
    if flag == 1:
        set_point(1,-5,280,play_down_b,E)
        time.sleep_ms(T*250)
        set_point(1,-5,250,play_ready,E)
        time.sleep_ms(100)
    elif flag == 2:
        set_point(2,-4,220,play_down_b+10,E)
        time.sleep_ms(T*250)
        set_point(2,-4,220,play_ready,E)
        time.sleep_ms(100)



def home_seting():
    flag_a=0
    flag_b=0
    data_to_send = "G28\r\n"
    print(data_to_send)
    uart1.write(data_to_send)
    uart2.write(data_to_send)
    while(True):
        if uart1.any():
            print(uart1.read())
            flag_a=1
            # break
        if uart2.any():
            print(uart2.read())
            flag_b=1
            # break
        if flag_a and flag_b:
            break


def go2point(flag,X,Y,Z,E):
    set_point(flag,X,Y,Z,E)
    print(flag,X,Y,Z,E)

def playpiano_1():
    distn = 0
    distl = 0
    for i in range(len(pitches)):
        for index in range(len(piano_map_list_2)):
            if pitches[i] in high:
                if pitches[i] == piano_map_list_2[index]:
                    distl = distn
                    distn = int((piano_map_disE_list_mm[index])/2)
                    distall = distn - distl
                    print(distall)
                    if distall >= -10:
                        print("run1")
                        go2point(2,-4,200,play_ready,E=int(100)/2)
                        dish = int((piano_map_disE_list_mm[index]-(key_len*20))/2)
                        if dish < 0 :
                            dish = 200
                        go2point(1,-5,250,play_ready,E=dish)
                        if index in black_key:
                            playb(1,E=int((piano_map_disE_list_mm[index]-(key_len*20))/2),T=int(durations[i]))
                            # time.sleep_ms(1000)
                            break
                        else:
                            play(1,E=int((piano_map_disE_list_mm[index]-(key_len*20))/2),T=int(durations[i]))
                            # time.sleep_ms(1000)
                            break

                    else:
                        print("run2")
                        go2point(1,-4,250,play_ready,E=int(400)/2)
                        go2point(2,-4,200,play_ready,E=int((piano_map_disE_list_mm[index]-(key_len*11))/2))
                        if index in black_key:
                            playb(2,E=int((piano_map_disE_list_mm[index]-(key_len*11))/2),T=int(durations[i]))
                            # time.sleep_ms(1000)
                            break
                        else:
                            play(2,E=int((piano_map_disE_list_mm[index]-(key_len*11))/2),T=int(durations[i]))
                            # time.sleep_ms(1000)
                            break
            else:
                if pitches[i] == piano_map_list_2[index]:
                    print("run3")
                    go2point(1,-4,250,play_ready,E=int(400)/2)
                    dst = int((piano_map_disE_list_mm[index]-(key_len*11))/2)
                    if dst < 0 :
                        dst = 50
                    go2point(2,-4,200,play_ready,E=dst)
                    if index in black_key:
                        playb(2,E=int((piano_map_disE_list_mm[index]-(key_len*11))/2),T=int(durations[i]))
                        # time.sleep_ms(1000)
                        break
                    else:
                        play(2,E=int((piano_map_disE_list_mm[index]-(key_len*11))/2),T=int(durations[i]))
                        # time.sleep_ms(1000)
                        break



home_seting()
set_point(1,-5,200,play_ready,0)
set_point(2,-4,200,play_ready,0)
time.sleep_ms(1000)


playpiano_1()

set_point(1,-5,200,play_ready,0)
set_point(2,-4,200,play_ready,0)
while True: 
    clock.tick()  # Update the FPS clock.
    img = sensor.snapshot()  # Take a picture and return the image.
    # print(clock.fps())  # Note: OpenMV Cam runs about half as fast when connected
