import socket
import struct
import time
import random
import sys

# The exact 22-byte format your C++ engine demands
ORDER_FORMAT = "<QIIBBI" 
FAIR_VALUE = 50 

class ChaosBot:
    def __init__(self, bot_id):
        self.bot_id = bot_id
        self.order_id_counter = int(bot_id) * 10000 
        
    def generate_action(self):
        self.order_id_counter += 1
        is_buy = random.choice([True, False])
        qty = random.randint(1, 5) * 10 
        
        # 30% Aggressive (Taker), 70% Passive (Maker)
        is_aggressive = random.random() < 0.30
        
        if is_buy:
            price = random.randint(FAIR_VALUE, FAIR_VALUE + 5) if is_aggressive else random.randint(FAIR_VALUE - 5, FAIR_VALUE - 1)
        else:
            price = random.randint(FAIR_VALUE - 5, FAIR_VALUE) if is_aggressive else random.randint(FAIR_VALUE + 1, FAIR_VALUE + 5)
            
        return {
            "order_id": self.order_id_counter,
            "is_buy": is_buy,
            "price": price,
            "quantity": qty
        }

def main():
    bot_id = sys.argv[1] if len(sys.argv) > 1 else "1"
    bot = ChaosBot(bot_id)
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # Disable Nagle's algorithm for instant transmission just like client.py
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    
    try:
        sock.connect(('127.0.0.1', 9000))
        print(f"[Bot {bot_id}] TCP Connected to Gateway. Entering chaos loop...")
    except ConnectionRefusedError:
        print(f"[Bot {bot_id}] Connection refused. Is the C++ engine running?")
        sys.exit(1)
        
    # Make the socket non-blocking so we can drain it without freezing the bot
    sock.setblocking(False)
    
    try:
        while True:
            action = bot.generate_action()
            
            # Map Python boolean to C++ uint8_t
            side = 1 if action["is_buy"] else 0
            
            # Pack the 22 bytes: ID, Px, Qty, Side, Action(0=New), Checksum(0)
            payload = struct.pack(
                ORDER_FORMAT, 
                action["order_id"], 
                action["price"], 
                action["quantity"], 
                side,
                0, 
                0  
            )
            
            # Fire it at the exchange
            try:
                sock.sendall(payload)
            except BlockingIOError:
                pass # Socket buffer temporarily full
                
            side_str = 'BUY' if action['is_buy'] else 'SELL'
            print(f"[Bot {bot_id}] Fired {side_str} {action['quantity']} @ ${action['price']}")
            
            # DRAIN THE PIPE: Quietly read all C++ execution reports and discard them
            # If we don't do this, the OS buffer fills up and the C++ engine kicks us out
            try:
                while True:
                    sock.recv(4096)
            except BlockingIOError:
                pass # Buffer is empty, move on
            except Exception:
                pass
            
            # Sleep a random amount between 100ms and 800ms
            time.sleep(random.uniform(0.1, 0.8))
            
    except KeyboardInterrupt:
        print(f"\n[Bot {bot_id}] Shutting down.")
    finally:
        sock.close()

if __name__ == "__main__":
    main()