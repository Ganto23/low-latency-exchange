#include <iostream>
#include <thread>
#include <vector>
#include <pthread.h>
#include <chrono>
#include "concurrency/SPSCQueue.hpp"
#include "core/Matcher.hpp"
#include "core/Order.hpp"

using namespace std::chrono_literals;

SPSCQueue<OrderPayload, 1024> ingress_queue;
SPSCQueue<ExecutionPayload, 1024> egress_queue;

static int pin_thread_to_core(pthread_t thread, int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    return pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
}

int main() {
    std::thread gateway_thread([]() {
        uint64_t id = 0;
        while (id < 1000){
            ingress_queue.try_push({id, 5, 6, true, ActionType::New});
            id++;
            std::this_thread::sleep_for(2000ms);
        }
    });

    std::thread matcher_thread([]() {
        Matcher matcher;
        while (true) {
            ingress_queue.wait_for_data();
            size_t available = ingress_queue.available_to_read();
            for (size_t i = 0; i < available; i++){
                OrderPayload* ptr = ingress_queue.peek(i);
                matcher.process_payload(*ptr);
            }
            ingress_queue.advance(available);
        }
    });

    int rc = pin_thread_to_core(gateway_thread.native_handle(), 1);
    rc = pin_thread_to_core(matcher_thread.native_handle(), 2);

    gateway_thread.join();
    matcher_thread.join();
}


