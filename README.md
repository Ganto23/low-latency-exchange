# 🚀 Ultra-Low Latency Matching Engine (Raspberry Pi 5)

A high-performance, multi-core Limit Order Book (LOB) and Exchange architecture designed for sub-microsecond deterministic matching. This project demonstrates hardware-level optimization, lock-free concurrency, and high-throughput market data distribution.

---

## 📑 Table of Contents
* [Performance Summary](#-performance-benchmarks)
* [System Architecture](#%EF%B8%8F-system-architecture)
* [Core Component: The Matcher](#-the-matcher-sub-1ns-discovery)
* [Core Component: Memory Management](#-memory-management-zero-allocation)
* [Core Component: Concurrency](#-concurrency-the-lock-free-pipeline)
* [Market Data & Web Bridge](#-market-data--web-bridge)
* [Quick Start](#-quick-start)

---

## 📊 Performance Benchmarks
*Benchmarked on Raspberry Pi 5 (ARM Cortex-A76 @ 2.4GHz) using Google Benchmark.*

| Component | Operation | Latency | Why it Matters |
| :--- | :--- | :--- | :--- |
| **Bitboard Index** | BBO Discovery | **0.64 ns** | Enables $O(1)$ lookup regardless of book depth. |
| **FreeList** | Node Allocation | **4.87 ns** | 4.5x faster than standard heap (`malloc`). |
| **SPSC Queue** | Cross-Core IPC | **8.04 ns** | Minimal overhead for inter-thread communication. |
| **Hot Path** | Full Match Cycle | **18.10 ns** | Deterministic "Tick-to-Trade" internal latency. |
| **Chaos Stress** | 1M Order Flow | **35.30 ns** | Maintains performance under extreme volatility. |

---

## 🏗️ System Architecture
The engine is architected to eliminate "jitter" (latency variance) by pinning specific tasks to isolated CPU cores, preventing context switching and cache pollution.

* **Core 1 (The Hot Path):** Executes the Matching Engine. No I/O, no syscalls—just pure logic.
* **Core 2 (The Broadcaster):** Handles binary UDP Multicast for Market Data.
* **Core 3 (The Gateway):** Manages a TCP E-poll server for high-concurrency client order entry.
* **Core 0 (The Orchestrator):** Manages OS interrupts, the Python WebSocket Bridge, and the Web UI.

---

## 🎯 The Matcher: Sub-1ns Discovery
The Matcher uses a **Dual-Level Bitboard Index** to represent the presence of orders across the price ladder. 
* **The Optimization:** By utilizing ARM-native `clz` (Count Leading Zeros) instructions, the engine identifies the Best Bid or Best Offer in **0.64 nanoseconds**.
* **The Result:** Unlike standard `std::map` implementations which are $O(\log n)$, this approach is $O(1)$, ensuring the engine remains fast even if the book contains thousands of price levels.

---

## 🧠 Memory Management: Zero-Allocation
To achieve deterministic latency, the engine employs a **Pre-allocated Index-Based FreeList**.
* **The Optimization:** All memory required for 1,000,000 orders is allocated in a contiguous block at startup. 
* **The Result:** The "Hot Path" never calls the OS kernel for memory (`malloc`/`new`). Allocation takes **4.87ns**, effectively eliminating the risk of "Stop-the-World" garbage collection or heap fragmentation common in slower systems.

---

## ⚡ Concurrency: The Lock-Free Pipeline
Data moves between CPU cores via **Single-Producer Single-Consumer (SPSC) Ring Buffers**.
* **The Optimization:** The queues use **Cache-Line Padding** to prevent "False Sharing" (where two cores fight over the same cache line). 
* **The Result:** At **8.04ns** per message, the overhead of moving data from the Gateway to the Matcher is practically non-existent, allowing the system to process millions of messages per second.

---

## 🌐 Market Data & Web Bridge
Real-time transparency is handled via a **UDP Multicast to WebSocket Bridge**.
1. **Publisher (C++):** Blasts raw binary structs over UDP Multicast (Core 2).
2. **Bridge (Python):** An asynchronous `asyncio` service that consumes the multicast stream and translates it to JSON.
3. **Web UI:** A low-latency React/JS dashboard providing a live view of the Order Book and Trade Tape.

---

## 🚀 Quick Start

### Hardware Requirements
* Raspberry Pi 5 (or any ARMv8/x86_64 Linux machine).
* Dedicated cores for taskset pinning.

### Build & Launch
The included orchestrator script handles CPU frequency scaling (Performance Mode), compilation, and core affinity pinning.

```bash
# Clone the repository
git clone [https://github.com/YourUsername/low-latency-exchange](https://github.com/YourUsername/low-latency-exchange)
cd low-latency-exchange

# Launch the full stack
./scripts/run_exchange.sh