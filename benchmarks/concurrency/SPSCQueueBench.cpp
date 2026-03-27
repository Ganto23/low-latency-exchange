#include <benchmark/benchmark.h>
#include "concurrency/SPSCQueue.hpp"
#include "core/Order.hpp" // For OrderPayload
#include <pthread.h>
#include <sched.h>
#include <array>

// Maps Google Benchmark threads directly to your isolated cores
inline void PinToIsolatedCore(int thread_idx) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    
    // Thread 0 -> Core 1, Thread 1 -> Core 2, Thread 2 -> Core 3
    int core_id = (thread_idx % 3) + 1; 
    
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

// --- Test 1: Best Case Latency (Push/Pop on same thread/core) ---
static void BM_SPSCQueue_Uncontended(benchmark::State& state) {
    PinToIsolatedCore(state.thread_index());
    SPSCQueue<OrderPayload, 1024> queue;
    OrderPayload dummy_in{123, 50, 100, true, ActionType::New, 0};
    
    for (auto _ : state) {
        queue.try_push(dummy_in);
        OrderPayload dummy_out;
        queue.try_pop(dummy_out);
        benchmark::DoNotOptimize(dummy_out);
    }
}
BENCHMARK(BM_SPSCQueue_Uncontended)->Threads(1);

// --- Test 2: Cross-Core Producer/Consumer (Standard Pop) ---
static void BM_SPSCQueue_ProducerConsumer(benchmark::State& state) {
    PinToIsolatedCore(state.thread_index());
    // Make the queue massive so the producer never blocks on a full queue
    static SPSCQueue<OrderPayload, 65536> queue; 

    if (state.thread_index() == 0) { // PRODUCER (Core 1)
        OrderPayload dummy_in{123, 50, 100, true, ActionType::New, 0};
        for (auto _ : state) {
            while (!queue.try_push(dummy_in)) {
                asm volatile("yield" ::: "memory");
            }
        }
    } else { // CONSUMER (Core 2)
        OrderPayload dummy_out;
        for (auto _ : state) {
            while (!queue.try_pop(dummy_out)) {
                asm volatile("yield" ::: "memory");
            }
            benchmark::DoNotOptimize(dummy_out);
        }
    }
}
BENCHMARK(BM_SPSCQueue_ProducerConsumer)->Threads(2);

// --- Test 3: Cross-Core Producer/Consumer (Zero-Copy Batched) ---
static void BM_SPSCQueue_BatchedZeroCopy(benchmark::State& state) {
    PinToIsolatedCore(state.thread_index());
    static SPSCQueue<OrderPayload, 65536> queue;
    
    if (state.thread_index() == 0) { // PRODUCER (Core 1)
        std::array<OrderPayload, 8> batch; // Push 8 at a time
        batch.fill({123, 50, 100, true, ActionType::New, 0});
        for (auto _ : state) {
            while (queue.try_push_batch(batch.data(), 8) == 0) {
                asm volatile("yield" ::: "memory");
            }
        }
    } else { // CONSUMER (Core 2)
        for (auto _ : state) {
            // Wait until we have exactly our batch of 8 to keep benchmark iterations 1:1
            while (queue.available_to_read() < 8) {
                asm volatile("yield" ::: "memory");
            }
            
            for (size_t i = 0; i < 8; i++) {
                OrderPayload* ptr = queue.peek(i);
                benchmark::DoNotOptimize(ptr);
            }
            queue.advance(8);
        }
    }
}
BENCHMARK(BM_SPSCQueue_BatchedZeroCopy)->Threads(2);

BENCHMARK_MAIN();