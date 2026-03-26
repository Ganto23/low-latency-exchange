#pragma once
#include <new>
#include <utility>
#include <atomic>

//TODO: Consider using the Wait for event pattern when queue is empty and consumer waiting or spinning
template <typename T, size_t Capacity>
class SPSCQueue {
    static_assert((Capacity != 0) && ((Capacity & (Capacity - 1)) == 0), "Capacity must be a power of two");
    static_assert(std::atomic<size_t>::is_always_lock_free, "Atomics are secretly using locks!");
public:
    SPSCQueue() {
        tail.store(0, std::memory_order_relaxed);
        head.store(0, std::memory_order_relaxed);
        tail_cache = 0;
        head_cache = 0;

        for (size_t i = 0; i < Capacity; i++) {
            new (&buffer[i]) T();
        }
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }

    bool try_push(const T& value) {
        if (tail_local - head_cache == Capacity) {
            head_cache = head.load(std::memory_order_acquire);

            if (tail_local - head_cache == Capacity) {
                return false;
            }
        }

        buffer[tail_local & Mask] = value;
        tail_local++;
        tail.store(tail_local, std::memory_order_release);
        return true;
    }

    size_t try_push_batch(const T* items, size_t count) {
        size_t available = Capacity - (tail_local - head_cache);
        
        if (available < count) {
            head_cache = head.load(std::memory_order_acquire);
            available = Capacity - (tail_local - head_cache);
            if (available == 0) return 0;
        }

        size_t to_push = (count < available) ? count : available;
        
        for (size_t i = 0; i < to_push; ++i) {
            buffer[(tail_local + i) & Mask] = items[i];
        }

        tail_local += to_push;
        tail.store(tail_local, std::memory_order_release);
        return to_push;
    }

    bool try_pop(T& value) {
        if (head_local == tail_cache) {
            tail_cache = tail.load(std::memory_order_acquire);
            if (head_local == tail_cache) {
                return false;
            }
        }

        value = std::move(buffer[head_local & Mask]);
        head_local++;
        head.store(head_local, std::memory_order_release);
        return true;
    }

    T* peek(size_t offset = 0) {
        if (head_local == tail_cache) {
            tail_cache = tail.load(std::memory_order_acquire);
            if (head_local == tail_cache) {
                return nullptr;
            }
        }
        return &buffer[(head_local + offset) & Mask];
    }

    void advance(size_t n = 1) {
        head_local += n;
        head.store(head_local, std::memory_order_release);
    }

    void wait_for_data() {
        while (head_local == tail.load(std::memory_order_acquire)) {
            asm volatile("yield" ::: "memory"); 
        }
    }

    size_t available_to_read() {
        size_t available = tail_cache - head_local;
        if (available == 0) {
            tail_cache = tail.load(std::memory_order_acquire);
            available = tail_cache - head_local;
        }
        return available;
    }

private:
    alignas(128) T buffer[Capacity];

    alignas(128) std::atomic<size_t> tail{0};
    size_t tail_local{0};
    size_t head_cache{0}; 

    alignas(128) std::atomic<size_t> head{0};
    size_t head_local{0};
    size_t tail_cache{0};

    alignas(128) char _padding[128];

    static constexpr size_t Mask = Capacity - 1;
};