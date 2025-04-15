import xml.etree.ElementTree as ET

def parse_musicxml(xml_content):
    """解析MusicXML并生成数据结构"""
    P = []  # 每个小节的音符总数
    M = []  # 音高存储
    N = []  # 类型存储
    
    root = ET.fromstring(xml_content)
    
    for measure in root.findall('.//part/measure'):
        note_count = 0
        for note in measure.findall('note'):
            # 解析音符类型
            note_type = note.findtext('type')
            is_rest = note.find('rest') is not None
            
            # 处理音高
            if is_rest:
                pitch = "R"
            else:
                pitch_elem = note.find('pitch')
                step = pitch_elem.findtext('step')
                octave = pitch_elem.findtext('octave')
                alter = pitch_elem.find('alter')
                accidental = ''
                if alter is not None:
                    accidental = '#' if int(alter.text) > 0 else 'b'
                pitch = f"{step}{accidental}{octave}"
            
            # 存储到对应变量
            M.append(pitch)
            N.append(note_type)
            note_count += 1
        
        P.append(note_count)
    
    return P, M, N

# 使用示例
if __name__ == "__main__":
    with open('P2.xml', 'r', encoding='utf-8') as f:
        xml_content = f.read()
    
    P, M, N = parse_musicxml(xml_content)
    
    print(P)
    print(len(P))

    print(M)
    print(len(M))

    print(N)
    print(len(N))
