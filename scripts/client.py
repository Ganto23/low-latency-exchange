import socket
import struct

# Network packing formats matching C++ #pragma pack(1)
ORDER_FORMAT = "<QIIBBI"  # Total: 22 bytes
EXEC_FORMAT = "<QIIB"     # Total: 17 bytes

# Enums
ACTION_NEW, ACTION_CANCEL = 0, 1
SIDE_SELL, SIDE_BUY = 0, 1
STATUS_MAP = {0: "Filled", 1: "Partial", 2: "Accepted", 3: "Canceled"}

def connect_and_trade(ip="127.0.0.1", port=9000):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        # Disable Nagle's algorithm for instant transmission
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        sock.connect((ip, port))
        print(f"Connected to Matcher at {ip}:{port}\n")

        # ==========================================
        # 1. SEND THE MAKER ORDER (Resting in book)
        # ==========================================
        # ID: 100, Price: 50, Qty: 10, Buy, New, Checksum: 0
        maker_order = struct.pack(ORDER_FORMAT, 100, 50, 10, SIDE_BUY, ACTION_NEW, 0)
        sock.sendall(maker_order)
        
        # Catch the "Accepted" response from the engine
        response_1 = sock.recv(17)
        id1, px1, qty1, status1 = struct.unpack(EXEC_FORMAT, response_1)
        print(f"-> [Maker Update] ID: {id1} | Qty: {qty1} @ {px1} | Status: {STATUS_MAP.get(status1)}")
        
        # ==========================================
        # 2. SEND THE TAKER ORDER (Aggressive hit)
        # ==========================================
        # ID: 101, Price: 50, Qty: 10, Sell, New, Checksum: 0
        taker_order = struct.pack(ORDER_FORMAT, 101, 50, 10, SIDE_SELL, ACTION_NEW, 0)
        sock.sendall(taker_order)

        # When the trade hits, the Matcher spits out TWO executions back-to-back!
        
        # Catch the Fill for the Maker (Order 100)
        response_2 = sock.recv(17)
        id2, px2, qty2, status2 = struct.unpack(EXEC_FORMAT, response_2)
        print(f"-> [Maker Fill]   ID: {id2} | Qty: {qty2} @ {px2} | Status: {STATUS_MAP.get(status2)}")

        # Catch the Fill for the Taker (Order 101)
        response_3 = sock.recv(17)
        id3, px3, qty3, status3 = struct.unpack(EXEC_FORMAT, response_3)
        print(f"-> [Taker Fill]   ID: {id3} | Qty: {qty3} @ {px3} | Status: {STATUS_MAP.get(status3)}")

if __name__ == "__main__":
    connect_and_trade("127.0.0.1", 9000)