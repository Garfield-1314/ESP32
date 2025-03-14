# musicxml_parser.py

def parse_musicxml(file_path):
    try:
        with open(file_path, 'r') as file:
            xml_content = file.read()
    except Exception as e:
        print("无法打开文件:", e)
        return [], [], [], [], [], []

    pitches = []
    durations = []
    high = []
    low = []
    high_durations = []
    low_durations = []

    if xml_content:
        start_index = 0
        while True:
            pitch_start = xml_content.find('<pitch>', start_index)
            if pitch_start == -1:
                break
            pitch_end = xml_content.find('</pitch>', pitch_start)
            pitch_data = xml_content[pitch_start + 7:pitch_end]

            step_start = pitch_data.find('<step>') + 6
            step_end = pitch_data.find('</step>', step_start)
            step = pitch_data[step_start:step_end]

            octave_start = pitch_data.find('<octave>') + 8
            octave_end = pitch_data.find('</octave>', octave_start)
            octave = int(pitch_data[octave_start:octave_end])

            alter_start = pitch_data.find('<alter>') + 7
            alter_end = pitch_data.find('</alter>', alter_start)
            alter = pitch_data[alter_start:alter_end] if alter_start != -1 else None

            alter_sign = '#' if alter == '1' else 'b' if alter == '-1' else ''
            pitch_info = f"{step}{alter_sign}{octave}"
            pitches.append(pitch_info)

            duration_start = xml_content.find('<duration>', pitch_end) + 10
            duration_end = xml_content.find('</duration>', duration_start)
            duration = xml_content[duration_start:duration_end]
            durations.append(duration)

            if octave >= 4:
                high.append(pitch_info)
                high_durations.append(duration)
            elif octave < 4:
                low.append(pitch_info)
                low_durations.append(duration)

            start_index = pitch_end + 1

    return pitches, durations, high, low, high_durations, low_durations