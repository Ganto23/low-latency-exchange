#!/bin/bash

# 1. Catch Ctrl+C to cleanly kill all background processes
cleanup() {
    echo -e "\n[Orchestrator] Shutting down exchange services..."
    # Kill all background jobs started by this script
    kill $(jobs -p) 2>/dev/null || true
    echo "[Orchestrator] All systems offline."
    exit
}
trap cleanup SIGINT

echo "=================================================="
echo "       STARTING HFT MATCHING ENGINE               "
echo "=================================================="

# 2. Maximize Raspberry Pi CPU Performance
echo "[Orchestrator] Forcing CPU into Performance Mode (requires sudo)..."
echo "performance" | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor > /dev/null

# 3. Compile the latest C++ code
echo "[Orchestrator] Building C++ Matching Engine..."
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_TELEMETRY=ON ..
make -j4
cd ..

# 4. Start the Services in the background (&)
echo "[Orchestrator] Starting C++ Matching Engine (Core 1, 2, 3)..."
taskset -c 1,2,3 ./build/src/matching_engine &
sleep 1 # Give the engine a second to bind its ports

echo "[Orchestrator] Starting Python Multicast Bridge (Core 0)..."
taskset -c 0 python3 scripts/bridge.py &

echo "[Orchestrator] Starting Web Dashboard Server..."
cd web
taskset -c 0 python3 -m http.server 3000 > /dev/null 2>&1 &
cd ..

# 5. Get the Pi's IP and print the dashboard link
PI_IP=$(hostname -I | awk '{print $1}')

echo "=================================================="
echo "🚀 EXCHANGE IS LIVE!"
echo "💻 Access the Dashboard here: http://$PI_IP:3000"
echo "🛑 Press Ctrl+C in this terminal to safely shut down."
echo "=================================================="

# 6. Wait infinitely so the script doesn't exit and kill the background jobs
wait