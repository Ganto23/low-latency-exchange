#include <benchmark/benchmark.h>
#include <pthread.h>
#include <atomic>
#include "concurrency/SPSCQueue.hpp"
#include "core/Matcher.hpp"
#include "core/Order.hpp"
#include "LatencyRecorder.hpp"
#include <chrono>
#include <thread>

// Global instances
static LatencyRecorder<5000> g_recorder;

static SPSCQueue<OrderPayload, 65536> ingress_queue;
static SPSCQueue<ExecutionPayload, 1024> dummy_egress;
static SPSCQueue<MarketDataEvent, 4096> dummy_md;
static Matcher matcher(dummy_egress, dummy_md);

inline void PinToIsolatedCore(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

static void BM_InternalPipeline_Latency(benchmark::State& state) {
    if (state.thread_index() == 0) {
        // --- PRODUCER (Gateway) ---
        PinToIsolatedCore(1);
        OrderPayload order{1, 100, 10, true, ActionType::New, 0};
        uint64_t iter_count = 0;

        for (auto _ : state) {
            while (!ingress_queue.try_push(order)) {
                asm volatile("yield" ::: "memory");
            }
            
            // Periodically send a "Reset" or just clear the book 
            // to prevent FreeList exhaustion if your Matcher supports it.
            // If not, we'll just let the benchmark run for fewer iterations.
            iter_count++;
        }
    } 
    else {
        // --- CONSUMER (Matcher) ---
        PinToIsolatedCore(2);
        for (auto _ : state) {
            while (ingress_queue.available_to_read() == 0) {
                asm volatile("yield" ::: "memory");
            }
            
            OrderPayload* p = ingress_queue.peek(0);

            auto start = std::chrono::high_resolution_clock::now();
            
            // CRITICAL: If your Matcher doesn't have a way to clear the book,
            // the FreeList will eventually empty. 
            matcher.process_payload(*p); 

            auto end = std::chrono::high_resolution_clock::now();

            uint64_t diff = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            g_recorder.record(diff);

            ExecutionPayload temp_exec;
            while (dummy_egress.try_pop(temp_exec)) {
            }

            MarketDataEvent temp_md;
            while (dummy_md.try_pop(temp_md)) {
            }
            
            benchmark::DoNotOptimize(p);
            ingress_queue.advance(1);
        }
    }

    if (state.thread_index() == 0) {
        // We give the consumer a tiny bit of time to finish its last few iterations
        // before the main thread prints the report
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        g_recorder.report("Matcher Hot Path (Tick-to-Trade)");
        g_recorder.save_to_csv("latency_data.csv");
    }
}

// Limit the number of iterations so we don't outrun the FreeList capacity
BENCHMARK(BM_InternalPipeline_Latency)->Threads(2)->Iterations(500000); 
BENCHMARK_MAIN();