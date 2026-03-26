#include <iostream>
#include <thread>
#include <vector>
#include <pthread.h>
#include <chrono>
#include "concurrency/SPSCQueue.hpp"
#include "core/Matcher.hpp"
#include "core/Order.hpp"
#include "gateway/TcpGateway.hpp"

using namespace std::chrono_literals;


static int pin_thread_to_core(pthread_t thread, int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    return pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
}

int main() {
    SPSCQueue<OrderPayload, 65536> ingress_queue;
    SPSCQueue<ExecutionPayload, 1024> egress_queue;

    std::thread gateway_thread([&ingress_queue, &egress_queue]() {
        TcpGateway<100> gateway(ingress_queue, egress_queue, 9000);
        std::cout << "[Main] Gateway starting on port 9000..." << std::endl;
        gateway.run();
    });

    std::thread matcher_thread([&ingress_queue, &egress_queue]() {
        Matcher matcher(egress_queue);
        while (true) {
            ingress_queue.wait_for_data();
            size_t available = ingress_queue.available_to_read();
            for (size_t i = 0; i < available; i++){
                OrderPayload* ptr = ingress_queue.peek(i);
                matcher.process_payload(*ptr);
                std::cout << "Matched Order ID: " << ptr->order_id 
          << " | Price: " << ptr->price 
          << " | Batched Size: " << available << "\n";
            }
            ingress_queue.advance(available);
        }
    });

    pin_thread_to_core(gateway_thread.native_handle(), 1);
    pin_thread_to_core(matcher_thread.native_handle(), 2);

    gateway_thread.join();
    matcher_thread.join();
}


