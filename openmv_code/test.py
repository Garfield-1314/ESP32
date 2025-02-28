
import os
import time
from pyb import UART
import sensor
import time
sensor.reset()  # Reset and initialize the sensor.
sensor.set_pixformat(sensor.RGB565)  # Set pixel format to RGB565 (or GRAYSCALE)
sensor.set_framesize(sensor.QVGA)  # Set frame size to QVGA (320x240)
sensor.skip_frames(time=2000)  # Wait for settings take effect.
clock = time.clock()  # Create a clock object to track the FPS.
# UART 3, and baudrate.
uart = UART(3, 115200, timeout_char=200)


def read_musicxml(file_path):
    try:
        with open(file_path, 'r') as file:
            return file.read()
    except Exception as e:
        print("Error reading file:", e)
        return None

def note_to_midi(note_str):
    """将音符字符串转换为MIDI编号"""
    step = note_str[0]
    alter_part = []
    i = 1
    while i < len(note_str) and note_str[i] in ('#', 'b'):
        alter_part.append(note_str[i])
        i += 1
    octave = int(note_str[i:])
    
    # 计算升降号数值
    alter = 0
    for c in alter_part:
        alter += 1 if c == '#' else -1
    
    # MIDI计算公式
    step_order = {'C':0, 'D':2, 'E':4, 'F':5, 'G':7, 'A':9, 'B':11}
    return (octave + 1) * 12 + step_order[step] + alter

def extract_pitch_info(mxml_content):
    pitch_info = {
        'low': {'notes': [], 'highest': None, 'lowest': None},
        'high': {'notes': [], 'highest': None, 'lowest': None},
        'all': [],
        'overall_highest': None,
        'overall_lowest': None
    }
    
    # 初始化跟踪变量
    low_midi = {'max': -float('inf'), 'min': float('inf')}
    high_midi = {'max': -float('inf'), 'min': float('inf')}
    overall_midi = {'max': -float('inf'), 'min': float('inf')}

    note_start = mxml_content.find('<note>')
    while note_start != -1:
        note_end = mxml_content.find('</note>', note_start)
        note_info = mxml_content[note_start:note_end]
        
        if '<pitch>' in note_info:
            try:
                # 提取音高信息
                pitch_value = note_info.split('<pitch>')[1].split('</pitch>')[0]
                
                # 解析各元素
                step = pitch_value.split('<step>')[1].split('</step>')[0].strip()
                alter = int(pitch_value.split('<alter>')[1].split('</alter>')[0].strip()) if '<alter>' in pitch_value else 0
                octave = int(pitch_value.split('<octave>')[1].split('</octave>')[0].strip())
                
                # 生成音符字符串
                alter_str = ''
                if alter > 0:
                    alter_str = '#' * alter
                elif alter < 0:
                    alter_str = 'b' * abs(alter)
                
                note_str = f"{step}{alter_str}{octave}"
                midi = note_to_midi(note_str)
                
                # 全局记录
                pitch_info['all'].append(note_str)
                if midi > overall_midi['max']:
                    overall_midi['max'] = midi
                    pitch_info['overall_highest'] = note_str
                if midi < overall_midi['min']:
                    overall_midi['min'] = midi
                    pitch_info['overall_lowest'] = note_str
                
                # 分音区处理
                if octave < 5:  # 低音区
                    pitch_info['low']['notes'].append(note_str)
                    if midi > low_midi['max']:
                        low_midi['max'] = midi
                        pitch_info['low']['highest'] = note_str
                    if midi < low_midi['min']:
                        low_midi['min'] = midi
                        pitch_info['low']['lowest'] = note_str
                else:  # 高音区
                    pitch_info['high']['notes'].append(note_str)
                    if midi > high_midi['max']:
                        high_midi['max'] = midi
                        pitch_info['high']['highest'] = note_str
                    if midi < high_midi['min']:
                        high_midi['min'] = midi
                        pitch_info['high']['lowest'] = note_str
                        
            except Exception as e:
                print(f"解析音符时出错: {e}")

        note_start = mxml_content.find('<note>', note_end)
    
    return pitch_info

# 使用示例
file_path = 'far memory.xml'
xml_content = read_musicxml(file_path)

if xml_content:
    result = extract_pitch_info(xml_content)
    print(result['all'])

    print("全局范围:")
    print(f"最高音: {result['overall_highest']}")
    print(f"最低音: {result['overall_lowest']}\n")
    
    print("低音区:")
    print(f"音符列表: {result['low']['notes']}")
    print(f"最高音: {result['low']['highest']}")
    print(f"最低音: {result['low']['lowest']}\n")
    
    print("高音区:")
    print(f"音符列表: {result['high']['notes']}")
    print(f"最高音: {result['high']['highest']}")
    print(f"最低音: {result['high']['lowest']}")
else:
    print("文件读取失败")

result['all'] = ["Ab2"]

#钢琴长122.5cm
#白键52个
#黑键36个
piano_map_list = [
    "A0", "Bb0", "B0",
    "C1", "Db1", "D1", "Eb1", "E1", "F1", "Gb1", "G1", "Ab1", "A1", "Bb1", "B1",    #C1
    "C2", "Db2", "D2", "Eb2", "E2", "F2", "Gb2", "G2", "Ab2", "A2", "Bb2", "B2",    #C2  
    "C3", "Db3", "D3", "Eb3", "E3", "F3", "Gb3", "G3", "Ab3", "A3", "Bb3", "B3",    #C3
    "C4", "Db4", "D4", "Eb4", "E4", "F4", "Gb4", "G4", "Ab4", "A4", "Bb4", "B4",    #C4
    "C5", "Db5", "D5", "Eb5", "E5", "F5", "Gb5", "G5", "Ab5", "A5", "Bb5", "B5",    #C5
    "C6", "Db6", "D6", "Eb6", "E6", "F6", "Gb6", "G6", "Ab6", "A6", "Bb6", "B6",    #C6
    "C7", "Db7", "D7", "Eb7", "E7", "F7", "Gb7", "G7", "Ab7", "A7", "Bb7", "B7",    #C7
    "C8"                                                                            #C8
]

piano_map_disE_list_mm = [
    # A₀组（键1-3）
    0,    # 1. A₀ (白键)
    12,   # 2. A♯₀/B♭₀ (黑键)
    24,   # 3. B₀ (白键)
    
    # 第1八度：C₁→B₁（键4-15）
    47,   # 4. C₁ (白键)
    59,   # 5. C♯₁/D♭₁ (黑键)
    71,   # 6. D₁ (白键)
    83,   # 7. D♯₁/E♭₁ (黑键)
    94,   # 8. E₁ (白键)
    106,  # 9. F₁ (白键)
    118,  # 10. F♯₁/G♭₁ (黑键)
    130,  # 11. G₁ (白键)
    142,  # 12. G♯₁/A♭₁ (黑键)
    153,  # 13. A₁ (白键)
    165,  # 14. A♯₁/B♭₁ (黑键)
    177,  # 15. B₁ (白键),
    
    # 第2八度：C₂→B₂（键16-27）
    201,  # 16. C₂ (白键)
    212,  # 17. C♯₂/D♭₂ (黑键)
    224,  # 18. D₂ (白键)
    236,  # 19. D♯₂/E♭₂ (黑键)
    248,  # 20. E₂ (白键)
    260,  # 21. F₂ (白键)
    271,  # 22. F♯₂/G♭₂ (黑键)
    283,  # 23. G₂ (白键)
    295,  # 24. G♯₂/A♭₂ (黑键)
    307,  # 25. A₂ (白键)
    319,  # 26. A♯₂/B♭₂ (黑键)
    330,  # 27. B₂ (白键),
    
    # 第3八度：C₃→B₃（键28-39）
    354,  # 28. C₃ (白键)
    366,  # 29. C♯₃/D♭₃ (黑键)
    378,  # 30. D₃ (白键)
    389,  # 31. D♯₃/E♭₃ (黑键)
    401,  # 32. E₃ (白键)
    413,  # 33. F₃ (白键)
    425,  # 34. F♯₃/G♭₃ (黑键)
    437,  # 35. G₃ (白键)
    448,  # 36. G♯₃/A♭₃ (黑键)
    460,  # 37. A₃ (白键)
    472,  # 38. A♯₃/B♭₃ (黑键)
    484,  # 39. B₃ (白键),
    
    # 第4八度：C₄→B₄（中央C在此！键40-51）
    507,  # 40. C₄ (中央C, 白键)
    519,  # 41. C♯₄/D♭₄ (黑键)
    531,  # 42. D₄ (白键)
    543,  # 43. D♯₄/E♭₄ (黑键)
    555,  # 44. E₄ (白键)
    566,  # 45. F₄ (白键)
    578,  # 46. F♯₄/G♭₄ (黑键)
    590,  # 47. G₄ (白键)
    602,  # 48. G♯₄/A♭₄ (黑键)
    614,  # 49. A₄ (白键)
    625,  # 50. A♯₄/B♭₄ (黑键)
    637,  # 51. B₄ (白键),
    
    # 第5八度：C₅→B₅（键52-63）
    661,  # 52. C₅ (白键)
    673,  # 53. C♯₅/D♭₅ (黑键)
    684,  # 54. D₅ (白键) 
    696,  # 55. D♯₅/E♭₅ (黑键)
    708,  # 56. E₅ (白键)
    720,  # 57. F₅ (白键)
    732,  # 58. F♯₅/G♭₅ (黑键)
    743,  # 59. G₅ (白键)
    755,  # 60. G♯₅/A♭₅ (黑键)
    767,  # 61. A₅ (白键)
    779,  # 62. A♯₅/B♭₅ (黑键)
    791,  # 63. B₅ (白键),
    
    # 第6八度：C₆→B₆（键64-75）
    814,  # 64. C₆ (白键)
    826,  # 65. C♯₆/D♭₆ (黑键)
    838,  # 66. D₆ (白键)
    850,  # 67. D♯₆/E♭₆ (黑键)
    861,  # 68. E₆ (白键)
    873,  # 69. F₆ (白键)
    885,  # 70. F♯₆/G♭₆ (黑键)
    897,  # 71. G₆ (白键)
    909,  # 72. G♯₆/A♭₆ (黑键)
    920,  # 73. A₆ (白键)
    932,  # 74. A♯₆/B♭₆ (黑键)
    944,  # 75. B₆ (白键),
    
    # 第7八度：C₇→B₇（键76-87）
    968,  # 76. C₇ (白键)
    979,  # 77. C♯₇/D♭₇ (黑键)
    991,  # 78. D₇ (白键)
    1003, # 79. D♯₇/E♭₇ (黑键)
    1015, # 80. E₇ (白键)
    1027, # 81. F₇ (白键)
    1038, # 82. F♯₇/G♭₇ (黑键)
    1045, # 83. G₇ (白键) → 修正为1050（原计算错误）
    1062, # 84. G♯₇/A♭₇ (黑键)
    1074, # 85. A₇ (白键)
    1085, # 86. A♯₇/B♭₇ (黑键)
    1097, # 87. B₇ (白键),
    
    # 最高音C₈（键88）
    1110  # 88. C₈ (白键)
]


piano_start_point_x = 0
piano_start_point_y = 174
piano_start_point_z = 20
piano_start_point_e = 0
       
def set_point(X,Y,Z,E):
    while(True):
        data_to_send = "G0 X{} Y{} Z{} E{}\r\n".format(X, Y, Z, E)
        time.sleep_ms(100)
        data_to_send = "G0 X{} Y{} Z{} E{}\r\n".format(X, Y, Z, E)
        time.sleep_ms(100)
        uart.write(data_to_send)
        if uart.any():
            data = uart.read()  # 读取所有可用数据
            time.sleep_ms(100)
            # print(data)
            break

def ready2play(E):
    set_point(0,174,-45,E)

def play(E):
    set_point(0,174,-65,E)
    time.sleep_ms(100)
    set_point(0,174,-45,E)
    time.sleep_ms(100)
    print("play")

def home_seting():
   data_to_send = "G28\r\n"
   print(data_to_send)
   uart.write(data_to_send)
   while(True):
    if uart.any():
        print(uart.read())
        break


def go2point(X,Y,Z,E):
    set_point(X,Y,Z,E)
    print(X,Y,Z,E)
    

def playpiano():
    nums=0
    for i in range(len(result["all"])):
        for index in range(len(piano_map_list)):
            if result["all"][i] == piano_map_list[index]:
                go2point(0,174,-45,E=int((piano_map_disE_list_mm[index]-295)/2))
                nums=nums+1
                print(nums)
                play(E=int((piano_map_disE_list_mm[index]-295)/2))
                break
    return nums

# playpiano()
# home_seting()
# set_point(0,174,-65,0)
time.sleep_ms(1000)
ready2play(0)
# while True: 
#    set_point(0,174,120,0)
#    if uart.any():
#         print(uart.read())
#    time.sleep_ms(1000)


# nums=playpiano()
# print(nums)
for index in range(len(piano_map_list)):
    if "Ab2"== piano_map_list[index]:
        go2point(0,174,-45,E=int((piano_map_disE_list_mm[index]-295)/2))
        while True:
            a=1

while True:
    set_point(0,174,-45,0)
    # set_point(0,174,120,piano_map_disE_list_mm[48]/2)
    while True: 
        clock.tick()  # Update the FPS clock.
        img = sensor.snapshot()  # Take a picture and return the image.
        # print(clock.fps())  # Note: OpenMV Cam runs about half as fast when connected
    # to the IDE. The FPS should increase once disconnected.    


                if index==1|index==4|index==6|index==9|index==11|index==13|index==16|index==18|index==21|index==23|index==25|index==28|index==30|index==33|index==35|index==37|index==40|index==42|index==45|index==47|index==49|index==52|index==54|index==57|index==59|index==61|index==64|index==66|index==69|index==71|index==73|index==76|index==78|index==81|index==82|index==84 :
                    playb(E=int((piano_map_disE_list_mm[index]-(key_len*13+key_len*14)/2)/2))
                    print(index)
                    break