#include <benchmark/benchmark.h>
#include <pthread.h>
#include <atomic>
#include "concurrency/SPSCQueue.hpp"
#include "core/Matcher.hpp"
#include "core/Order.hpp"

// Global instances
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
            
            // CRITICAL: If your Matcher doesn't have a way to clear the book,
            // the FreeList will eventually empty. 
            matcher.process_payload(*p); 
            
            benchmark::DoNotOptimize(p);
            ingress_queue.advance(1);
        }
    }
}

// Limit the number of iterations so we don't outrun the FreeList capacity
BENCHMARK(BM_InternalPipeline_Latency)->Threads(2)->Iterations(500000); 
BENCHMARK_MAIN();