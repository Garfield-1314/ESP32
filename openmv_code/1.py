import xml.etree.ElementTree as ET

def parse_musicxml(file_path):
    """解析MusicXML文件并返回结构化数据"""
    score_data = {
        'parts': {},
        'max_measure': 0
    }
    
    try:
        tree = ET.parse(file_path)
        root = tree.getroot()
        
        for part in root.findall('part'):
            part_id = part.get('id')
            score_data['parts'][part_id] = {}
            
            for measure in part.findall('measure'):
                measure_num = int(measure.get('number'))
                score_data['max_measure'] = max(score_data['max_measure'], measure_num)
                
                # 初始化小节存储
                if measure_num not in score_data['parts'][part_id]:
                    score_data['parts'][part_id][measure_num] = []
                
                # 解析音符
                for note in measure.findall('note'):
                    note_data = {}
                    
                    if note.find('rest') is not None:
                        note_data['type'] = 'rest'
                        note_data['value'] = note.findtext('type').capitalize()
                    else:
                        pitch = note.find('pitch')
                        step = pitch.findtext('step')
                        octave = pitch.findtext('octave')
                        alter = pitch.findtext('alter')
                        
                        # 处理升降号
                        accidental = ''
                        if alter:
                            accidental = {
                                '1': '#',
                                '-1': '♭',
                                '2': '##',
                                '-2': '𝄫'
                            }.get(alter, '')
                        
                        note_data['type'] = 'note'
                        note_data['value'] = f"{step}{accidental}{octave}"
                        note_data['duration'] = note.findtext('type').capitalize()
                    
                    score_data['parts'][part_id][measure_num].append(note_data)
        
        return score_data
    
    except Exception as e:
        print(f"解析错误: {str(e)}")
        return None

def get_measure_notes(score_data, measure_num, part_id=None):
    """获取指定小节的音符数据"""
    try:
        measure_num = int(measure_num)
        results = {}
        
        if part_id:
            # 获取特定声部
            if measure_num in score_data['parts'][part_id]:
                return {part_id: score_data['parts'][part_id][measure_num]}
            return {}
        else:
            # 获取所有声部
            return {
                pid: part_data.get(measure_num, [])
                for pid, part_data in score_data['parts'].items()
            }
    
    except (KeyError, ValueError):
        return {}

# 使用示例
if __name__ == "__main__":
    xml_file = "P1.xml"
    score = parse_musicxml(xml_file)
    
    if score:
        # 查询第5小节所有声部
        print("=== 第5小节 ===")
        m5 = get_measure_notes(score, 1,'P1')
        for part, notes in m5.items():
            print(f"\n声部 {part}:")
            for note in notes:
                if note['type'] == 'rest':
                    print(f"  Rest {note['value']}")
                else:
                    print(f"  {note['value']} {note['duration']}")
        