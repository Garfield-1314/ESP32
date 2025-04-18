
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

P1 = [5, 4, 3, 4, 3, 4, 3, 4, 4, 4, 3, 4, 3, 5, 4, 5, 3, 4, 3, 4, 2, 6, 3, 4, 2, 2, 3, 5, 2, 2, 3, 5, 3, 4, 3, 4, 3, 4, 5, 4, 4, 4, 3, 4, 3, 5, 4, 5]

P1M = ['A4', 'B4', 'R', 'R', 'R', 'C5', 'B4', 
        'C5', 'E5', 'B4', 'R', 'E4', 'A4', 'G4', 'A4', 'C5', 
        'G4', 'R', 'E4', 'F4', 'E4', 'F4', 'C5', 'E4', 'R',
        'C5', 'B4', 'F#4', 'F#4', 'B4', 'B4', 'R', 'A4',
        'B4', 'C5', 'B4', 'C5', 'E5', 'B4', 'R', 'E4', 
        'A4', 'G4', 'A4', 'C5', 'G4', 'R', 'E4', 'F4', 
        'C5', 'B4', 'B4', 'C5', 'D5', 'E5', 'C5', 'C5', 'C5',
        'B4', 'A4', 'B4', 'G#4', 'A4', 'C5', 'D5', 'E5',
        'D5', 'E5', 'G5', 'D5', 'R', 'G4', 'C5', 'B4', 'C5', 
        'E5', 'E5', 'R', 'A4', 'B4', 'C5', 'B4', 'C5', 'D5', 
        'C5', 'G4', 'G4', 'F5', 'E5', 'D5', 'C5', 'E5', 'E5', 
        'A5', 'G5', 'E5', 'D5', 'C5', 'D5', 'C5', 'D5', 'D5', 
        'G5', 'E5', 'E5', 'A5', 'G5', 'E5', 'D5', 'C5', 'D5', 
        'C5', 'D5', 'D5', 'B4', 'A4', 'A4', 'B4', 'C5', 'B4', 
        'C5', 'E5', 'B4', 'R', 'E4', 'A4', 'G4', 'A4', 'C5', 
        'G4', 'R', 'E4', 'F4', 'E4', 'F4', 'C5', 'E4', 'E4', 
        'C5', 'R', 'R', 'B4', 'F#4', 'F#4', 'B4', 'B4', 'R', 
        'A4', 'B4', 'C5', 'B4', 'C5', 'E5', 'B4', 'R', 'E4', 
        'A4', 'G4', 'A4', 'C5', 'G4', 'R', 'E4', 'F4', 'C5', 
        'B4', 'B4', 'C5', 'D5', 'E5', 'C5', 'C5', 'C5', 'B4', 
        'A4', 'B4', 'G#4']

P1N = ['eighth', 'eighth', 'quarter', 'quarter', 'quarter', 
        'quarter', 'eighth', 'quarter', 'quarter', 'half', 'quarter', 
        'quarter', 'quarter', 'eighth', 'quarter', 'quarter', 'half', 
        'quarter', 'quarter', 'quarter', 'eighth', 'quarter', 'quarter', 
        'half', 'quarter', 'quarter', 'quarter', 'eighth', 'quarter', 
        'quarter', 'half', 'quarter', 'eighth', 'eighth', 'quarter', 
        'eighth', 'quarter', 'quarter', 'half', 'quarter', 'quarter', 
        'quarter', 'eighth', 'quarter', 'quarter', 'half', 'quarter', 
        'quarter', 'quarter', 'eighth', 'eighth', 'quarter', 'quarter', 
        'quarter', 'eighth', 'eighth', 'half', 'eighth', 'eighth', 'quarter', 
        'quarter', 'quarter', 'half', 'eighth', 'eighth', 'quarter', 'eighth',
        'quarter', 'quarter', 'half', 'quarter', 'quarter', 'quarter', 
        'eighth', 'quarter', 'quarter', 'half', 'quarter', 'eighth', 'eighth', 
        'quarter', 'eighth', 'eighth', 'quarter', 'quarter', 'eighth', 'half', 
        'quarter', 'quarter', 'quarter', 'quarter', 'half', 'quarter', 'half', 
        'half', 'eighth', 'eighth', 'half', 'quarter', 'eighth', 'eighth', 
        'quarter', 'quarter', 'half', 'quarter', 'half', 'half', 'eighth', 
        'eighth', 'half', 'quarter', 'eighth', 'eighth', 'quarter', 'quarter',
        'half', 'eighth', 'eighth', 'quarter', 'eighth', 'quarter', 'quarter', 
        'half', 'quarter', 'quarter', 'quarter', 'eighth', 'quarter', 'quarter',
        'half', 'quarter', 'quarter', 'quarter', 'eighth', 'quarter', 'quarter', 
        'half', 'quarter', 'quarter', 'eighth', 'eighth', 'quarter', 'eighth', 
        'quarter', 'quarter', 'half', 'quarter', 'eighth', 'eighth', 'quarter',
        'eighth', 'quarter', 'quarter', 'half', 'quarter', 'quarter', 'quarter',
            'eighth', 'quarter', 'quarter', 'half', 'quarter', 'quarter', 'quarter', 
            'eighth', 'eighth', 'quarter', 'quarter', 'quarter', 'eighth', 'eighth',
            'half', 'eighth', 'eighth', 'quarter', 'quarter', 'quarter']



P2 = [4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 3, 4, 3, 3, 3, 3, 3, 3, 3, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 3, 4]

P2M = ['R', 'R', 'R', 'R', 'A3', 'C4', 'E4', 'E3', 'G3', 'B3', 'F3', 
        'A3', 'C4', 'C3', 'E3', 'G3', 'D3', 'F3', 'A3', 'A2', 'C3', 'E3', 'B2',
        'D#3', 'F#3', 'E3', 'G#3', 'B3', 'A3', 'C4', 'E4', 'E3', 'G3', 'B3',
        'F3', 'A3', 'C4', 'C3', 'E3', 'G3', 'D3', 'F3', 'A3', 'F3', 'A2', 'C3', 
        'E3', 'D3', 'A3', 'E3', 'B3', 'A2', 'C3', 'E3', 'C3', 'E3', 'G3', 'G2', 
        'B2', 'D3', 'A2', 'C3', 'E3', 'E2', 'G2', 'B2', 'F2', 'A2', 'C3', 'C3', 
        'E3', 'G3', 'D3', 'F3', 'A3', 'F3', 'E3', 'G#3', 'B3', 'A3', 'C4', 'E4', 
        'C4', 'F3', 'A3', 'C4', 'A3', 'G3', 'B3', 'D4', 'B3', 'C3', 'E3', 'G3',
        'A2', 'C3', 'E3', 'C3', 'F2', 'A2', 'C3', 'A2', 'G2', 'B2', 'D3', 'B2', 
        'A2', 'C3', 'E3', 'A3', 'C4', 'E4', 'E3', 'G3', 'B3', 'F3', 'A3', 'C4',
        'C3', 'E3', 'G3', 'D3', 'F3', 'A3', 'A2', 'C3', 'E3', 'B2', 'D#3', 'F#3',
        'E3', 'G#3', 'B3', 'A3', 'C4', 'E4', 'E3', 'G3', 'B3', 'F3', 'A3', 'C4', 
        'C3', 'E3', 'G3', 'D3', 'F3', 'A3', 'F3', 'A2', 'C3', 'E3', 'D3', 'A3',
        'E3', 'B3']

P2N = ['quarter', 'quarter', 'quarter', 'quarter', 'quarter', 'quarter', 'half', 
'quarter', 'quarter', 'half', 'quarter', 'quarter', 'half', 'quarter', 'quarter', 
'half', 'quarter', 'quarter', 'half', 'quarter', 'quarter', 'half', 'quarter', 
'quarter', 'half', 'quarter', 'quarter', 'half', 'quarter', 'quarter', 'half', 
'quarter', 'quarter', 'half', 'quarter', 'quarter', 'half', 'quarter', 'quarter',
 'half', 'quarter', 'quarter', 'quarter', 'quarter', 'quarter', 'quarter', 'half', 
 'quarter', 'quarter', 'quarter', 'quarter', 'quarter', 'quarter', 'half', 'quarter', 
 'quarter', 'half', 'quarter', 'quarter', 'half', 'quarter', 'quarter', 'half',
  'quarter', 'quarter', 'half', 'quarter', 'quarter', 'half', 'quarter', 'quarter', 
  'half', 'quarter', 'quarter', 'quarter', 'quarter', 'quarter', 'quarter', 'half',
   'quarter', 'quarter', 'quarter', 'quarter', 'quarter', 'quarter', 'quarter', 
   'quarter', 'quarter', 'quarter', 'quarter', 'quarter', 'quarter', 'quarter', 
   'half', 'quarter', 'quarter', 'quarter', 'quarter', 'quarter', 'quarter', 
   'quarter', 'quarter', 'quarter', 'quarter', 'quarter', 'quarter', 'quarter', 
   'quarter', 'half', 'quarter', 'quarter', 'half', 'quarter', 'quarter', 'half',
    'quarter', 'quarter', 'half', 'quarter', 'quarter', 'half', 'quarter', 'quarter', 
    'half', 'quarter', 'quarter', 'half', 'quarter', 'quarter', 'half', 'quarter', 
    'quarter', 'half', 'quarter', 'quarter', 'half', 'quarter', 'quarter', 'half', 
    'quarter', 'quarter', 'half', 'quarter', 'quarter', 'half', 'quarter', 'quarter', 
    'quarter', 'quarter', 'quarter', 'quarter', 'half', 'quarter', 'quarter', 'quarter', 'quarter']