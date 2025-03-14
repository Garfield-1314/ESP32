# 通信协议格式：
# [帧头(2)] [数据长度(1)] [数据(n)] [校验和(1)] [帧尾(2)]
# 字节索引：0 1        2         3~n+2     n+3       n+4 n+5

# 协议常量定义
HEADER = b'\xAA\xBB'  # 2字节帧头
FOOTER = b'\xCC\xDD'  # 2字节帧尾
MAX_DATA_LEN = 255    # 最大数据长度

# 校验和计算（异或校验）
def calculate_checksum(data):
    checksum = 0
    for byte in data:
        checksum ^= byte
    return checksum

# ----------------- 发送端代码 -----------------
def send_packet(uart, data):
    # 检查数据长度
    if len(data) > MAX_DATA_LEN:
        raise ValueError("Data too long")
    
    # 构造数据包
    packet = HEADER
    packet += bytes([len(data)])  # 数据长度字节
    packet += data
    packet += bytes([calculate_checksum(data)])
    packet += FOOTER
    
    # 发送数据
    uart.write(packet)

# 使用示例
# uart = UART(3, 115200)  # 初始化串口
# send_packet(uart, b'Hello OpenMV')

# ----------------- 接收端代码 -----------------
class PacketReceiver:
    def __init__(self):
        self.state = 0       # 状态机
        self.data_len = 0    # 数据长度
        self.data = bytearray()
        self.checksum = 0    # 接收校验和
        self.packet = None   # 接收完成的数据包

    def process(self, uart):
        while uart.any():
            byte = uart.read(1)
            if not byte:
                continue

            # 状态机处理
            if self.state == 0:  # 等待帧头1
                if byte == HEADER[0:1]:
                    self.state = 1
            elif self.state == 1:  # 等待帧头2
                if byte == HEADER[1:2]:
                    self.state = 2
                else:
                    self.state = 0
            elif self.state == 2:  # 获取数据长度
                self.data_len = byte[0]
                self.data = bytearray()
                self.state = 3
            elif self.state == 3:  # 接收数据
                self.data.append(byte[0])
                if len(self.data) == self.data_len:
                    self.state = 4
            elif self.state == 4:  # 校验和验证
                expected = calculate_checksum(self.data)
                if byte[0] == expected:
                    self.state = 5
                else:
                    self.reset()
            elif self.state == 5:  # 等待帧尾1
                if byte == FOOTER[0:1]:
                    self.state = 6
                else:
                    self.reset()
            elif self.state == 6:  # 等待帧尾2
                if byte == FOOTER[1:2]:
                    self.packet = self.data
                    self.reset()
                    return True  # 完整包接收完成
                else:
                    self.reset()
            else:
                self.reset()
        return False

    def reset(self):
        self.state = 0
        self.data_len = 0
        self.data = bytearray()
        self.checksum = 0

# receiver = PacketReceiver()
# 使用示例
# receiver = PacketReceiver()
# while True:
#     if receiver.process(uart):
#         print("Received:", receiver.packet)