# 定义转换函数
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

# 原始输入数据（OpenMV兼容的元组格式）
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

# 转换所有音符
result = []
for note in piano_map_list_2:
    result.append(convert_note_to_weight(note))

# 打印结果（OpenMV通过串口输出）
print("Converted weights:", result)
