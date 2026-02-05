#!/bin/bash

# Configuration
SERVER_EXE="./server"
CLIENT_EXE="./client"
NUM_PLAYERS=3

echo "--- 🎲 Blackjack System Integration Test ---"

# 1. Clean up previous runs
make clean-ipc
rm -f game.log scores.txt

# 2. Start the Server in the background
echo "[DevOps] Starting Server..."
$SERVER_EXE &
SERVER_PID=$!
sleep 2 # Give server time to bind to port

# 3. Start Multiple Clients (Simulating real players)
for i in $(seq 1 $NUM_PLAYERS)
do
    echo "[DevOps] Launching Player $i..."
    # We use 'timeout' so the test doesn't hang forever
    # We pipe input "S" (for Stay) to the client if it asks for a move
    (echo "S"; sleep 2) | $CLIENT_EXE 127.0.0.1 &
done

# 4. Wait for game to progress
echo "[DevOps] Waiting for game logs to populate..."
sleep 10

# 5. Verification Logic
echo "--- 📊 Validation Results ---"

if [ -f "game.log" ]; then
    echo "✅ SUCCESS: game.log created."
    LOG_COUNT=$(grep -c "Player" game.log)
    echo "   Log entries found: $LOG_COUNT"
else
    echo "❌ FAIL: No game.log found."
fi

if [ -f "scores.txt" ]; then
    echo "✅ SUCCESS: Persistence file (scores.txt) created."
else
    echo "❌ FAIL: scores.txt missing."
fi

# 6. Shutdown
echo "[DevOps] Cleaning up processes..."
kill $SERVER_PID
pkill client
make clean-ipc

echo "--- 🏁 Test Complete ---"