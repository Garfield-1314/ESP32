import pretty_midi


def adjust_grouped_note_gaps(input_path, output_path, gap_seconds):
    """按音符组调整间隔（保留和弦）"""
    midi = pretty_midi.PrettyMIDI(input_path)
    
    for instrument in midi.instruments:
        # 按开始时间分组（相同时间为同一和弦）
        groups = {}
        for note in instrument.notes:
            if note.start not in groups:
                groups[note.start] = []
            groups[note.start].append(note)
        
        # 获取排序后的时间点
        times = sorted(groups.keys())
        if len(times) < 2:
            continue
        
        # 计算组间间隔
        prev_end = groups[times[0]][0].start + max(n.duration for n in groups[times[0]])
        for t in times[1:]:
            # 调整整组开始时间
            new_start = prev_end + gap_seconds
            duration = max(n.duration for n in groups[t])
            
            for note in groups[t]:
                note.start = new_start
            
            prev_end = new_start + duration
    
    midi.write(output_path)


# 使用示例
adjust_grouped_note_gaps('Song1.mid', 'output.mid', gap_seconds=(1/6))

