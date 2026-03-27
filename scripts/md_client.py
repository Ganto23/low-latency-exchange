import socket
import struct

# The binary format matching your C++ #pragma pack(1) MDMessage
# Q = uint64 (8 bytes), I = uint32 (4 bytes), B = uint8 (1 byte)
# Format: Seq(Q), TS(Q), Price(I), Qty(I), Type(B), Side(B)
MD_FORMAT = "<QQIIBB" 

MCAST_GRP = '239.0.0.1'
MCAST_PORT = 10000

# Mapping enums from C++
TYPE_MAP = {0: "TRADE", 1: "BBO_UPDATE"}
SIDE_MAP = {0: "SELL", 1: "BUY"}

def listen_to_market_data():
    # 1. Create a UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    # 2. Bind to the port
    sock.bind(('', MCAST_PORT))

    # 3. Join the Multicast Group
    # This tells the network card to start picking up packets for 239.0.0.1
    mreq = struct.pack("4s4s", socket.inet_aton(MCAST_GRP), socket.inet_aton("127.0.0.1"))
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

    print(f"Listening for Market Data on {MCAST_GRP}:{MCAST_PORT}...")

    try:
        while True:
            data, addr = sock.recvfrom(1024)
            if len(data) == 26:
                seq, ts, price, qty, mtype, side = struct.unpack(MD_FORMAT, data)
                
                print(f"[SEQ:{seq}] {TYPE_MAP.get(mtype)} | "
                      f"{SIDE_MAP.get(side)} {qty} @ {price} | TS: {ts}")
            else:
                print(f"Received malformed packet of size {len(data)}")
    except KeyboardInterrupt:
        print("\nStopping Market Data Listener.")

if __name__ == "__main__":
    listen_to_market_data()