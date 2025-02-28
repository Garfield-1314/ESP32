# 假设你已经将 XML 数据以文本方式存储在文件中
# 下面是读取文件并打印音高与时值的代码

# 打开 MusicXML 文件（在 OpenMV 中，文件应当放置在 sd 卡或者文件系统中）
try:
    with open('far memory.xml', 'r') as file:  # 请确保路径正确
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

print("\n所有时值:")
print(durations)

print("\n高音:")
print(high)

print("\n低音:")
print(low)

print("\n高音对应的时值:")
print(high_durations)

print("\n低音对应的时值:")
print(low_durations)
