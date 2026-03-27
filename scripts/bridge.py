import asyncio
import websockets
import json
import socket
import struct

MCAST_GRP = '239.0.0.1'
MCAST_PORT = 10000
MD_FORMAT = "<QQIIBB" # Seq, TS, Price, Qty, Type, Side

connected_clients = set()

class MulticastProtocol(asyncio.DatagramProtocol):
    def __init__(self, queue):
        self.queue = queue

    def datagram_received(self, data, addr):
        if len(data) == 26:
            seq, ts, price, qty, mtype, side = struct.unpack(MD_FORMAT, data)
            message = {
                "sequence": seq,
                "timestamp": ts,
                "price": price,
                "quantity": qty,
                "type": "TRADE" if mtype == 0 else "BBO_UPDATE",
                "side": "BUY" if side == 1 else "SELL"
            }
            # Instantly drop the formatted dict into the async queue
            self.queue.put_nowait(message)

async def websocket_handler(websocket):
    connected_clients.add(websocket)
    print(f"[Bridge] Browser connected! Total clients: {len(connected_clients)}")
    try:
        await websocket.wait_closed()
    finally:
        connected_clients.remove(websocket)
        print(f"[Bridge] Browser disconnected. Total clients: {len(connected_clients)}")

async def broadcaster(queue):
    while True:
        message = await queue.get()
        if connected_clients:
            json_str = json.dumps(message)
            # websockets.broadcast is highly optimized for blasting data to many clients
            websockets.broadcast(connected_clients, json_str)

async def main():
    queue = asyncio.Queue()
    loop = asyncio.get_running_loop()
    
    # 1. Setup the UDP Multicast Socket (exactly like md_client.py)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('', MCAST_PORT))
    
    mreq = struct.pack("4s4s", socket.inet_aton(MCAST_GRP), socket.inet_aton("127.0.0.1"))
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

    # 2. Start the non-blocking UDP listener
    await loop.create_datagram_endpoint(
        lambda: MulticastProtocol(queue),
        sock=sock
    )
    print(f"[Bridge] Listening to Multicast on {MCAST_GRP}:{MCAST_PORT}")

    # 3. Start the WebSocket server and the broadcaster loop
    async with websockets.serve(websocket_handler, "0.0.0.0", 8080):
        print("[Bridge] WebSocket Server running on ws://0.0.0.0:8080")
        await broadcaster(queue)

if __name__ == "__main__":
    asyncio.run(main())