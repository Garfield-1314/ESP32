import xml.etree.ElementTree as ET

# 加载 XML 文件
tree = ET.parse('far memory.xml')  # 替换为你的 XML 文件路径
root = tree.getroot()

# 提取所有的 part 元素
parts = root.findall('.//part')  # 找到所有部分

# 假设我们提取前两个 part
p1 = parts[0]  # 第一个 part
p2 = parts[1]  # 第二个 part

# 创建一个列表来存储所有的音高和时值
pitches = []
durations = []

# 提取 p1 中的 measures 和音符
measures_p1 = p1.findall('.//measure')  # 找到 p1 部分的所有小节
for measure in measures_p1:
    notes = measure.findall('.//note')  # 找到小节中的所有音符
    for note in notes:
        pitch = note.find('pitch')
        duration = note.find('duration')
        
        # 获取音符的音高和时值
        if pitch is not None:
            step = pitch.find('step')
            octave = pitch.find('octave')
            alter = pitch.find('alter')
            alter_text = alter.text if alter is not None else None
            if step is not None and octave is not None:
                alter_sign = '升' if alter_text == '1' else '降' if alter_text == '-1' else '无'
                pitch_info = f"{step.text}{octave.text}{alter_sign}"
                pitches.append(pitch_info)  # 将音高信息添加到列表
                durations.append(duration.text if duration is not None else '未知')  # 将时值添加到列表

# 提取 p2 中的 measures 和音符
measures_p2 = p2.findall('.//measure')  # 找到 p2 部分的所有小节
for measure in measures_p2:
    notes = measure.findall('.//note')  # 找到小节中的所有音符
    for note in notes:
        pitch = note.find('pitch')
        duration = note.find('duration')
        
        # 获取音符的音高和时值
        if pitch is not None:
            step = pitch.find('step')
            octave = pitch.find('octave')
            alter = pitch.find('alter')
            alter_text = alter.text if alter is not None else None
            if step is not None and octave is not None:
                alter_sign = '升' if alter_text == '1' else '降' if alter_text == '-1' else '无'
                pitch_info = f"{step.text}{octave.text}{alter_sign}"
                pitches.append(pitch_info)  # 将音高信息添加到列表
                durations.append(duration.text if duration is not None else '未知')  # 将时值添加到列表

# 直接打印所有的音高和时值
print("音高:")
print(pitches,len(pitches))

print("\n时值:")
print(durations,len(durations))
