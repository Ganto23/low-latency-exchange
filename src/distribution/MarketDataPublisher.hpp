#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../concurrency/SPSCQueue.hpp"
#include "../core/Order.hpp"
#include <chrono>
#include <iostream>

class MarketDataPublisher {
public:
    MarketDataPublisher(SPSCQueue<MarketDataEvent, 4096>& queue, 
                        const std::string& mcast_group, int port) 
        : md_queue(queue), sequence(0) {
        
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(mcast_group.c_str());
        addr.sin_port = htons(port);

        // --- CRITICAL FOR LOCAL Pi TESTING ---
        // 1. Force the OS to loop multicast packets back to local listeners
        int loop = 1;
        setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

        // 2. Force the packets to go out over the local loopback interface (127.0.0.1)
        struct in_addr local_interface;
        local_interface.s_addr = inet_addr("127.0.0.1");
        setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &local_interface, sizeof(local_interface));
    }

    void run() {
        MarketDataEvent event;
        while (running) {
            if (md_queue.try_pop(event)) {
                // THE DEBUG PRINT
                std::cout << "[Publisher] Popped event type " << (int)event.type 
                          << " | Px: " << event.price << " | Qty: " << event.quantity 
                          << " -> Sending to Multicast!" << std::endl;

                auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
                uint64_t ts = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();

                MDMessage msg = {
                    ++sequence,
                    ts,
                    event.price,
                    event.quantity,
                    static_cast<uint8_t>(event.type),
                    static_cast<uint8_t>(event.side)
                };

                sendto(fd, &msg, sizeof(msg), 0, (struct sockaddr*)&addr, sizeof(addr));
            } else {
                asm volatile("yield" ::: "memory");
            }
        }
    }

private:
    int fd;
    struct sockaddr_in addr;
    SPSCQueue<MarketDataEvent, 4096>& md_queue;
    uint64_t sequence;
    bool running = true;
};