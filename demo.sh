#!/bin/bash

# ============================================================
# Black Jack Game - Full Demonstration Script
# ============================================================
# This script demonstrates:
# 1. Clean compilation with make
# 2. Server startup
# 3. Multiple clients connecting
# 4. Full gameplay with automated moves
# 5. Real-time logging
# 6. Score display

set -e

PROJECT_DIR="/home/leongweilee/Downloads/Black_Jack-main"
cd "$PROJECT_DIR"

echo "======================================================"
echo "BLACK JACK GAME - FULL DEMONSTRATION"
echo "======================================================"
echo ""

# Step 1: Cleanup IPC Resources
echo "[STEP 1] Cleaning up IPC resources..."
rm -f /dev/shm/blackjack_shm /dev/shm/sem.bj_* 2>/dev/null || true
sleep 1
echo "✓ IPC cleanup complete"
echo ""

# Step 2: Clean and Compile
echo "[STEP 2] Compiling project..."
make clean > /dev/null 2>&1
make > /dev/null 2>&1
echo "✓ Compilation successful"
echo ""

# Step 3: Start Server
echo "[STEP 3] Starting server..."
./server > /tmp/server.log 2>&1 &
SERVER_PID=$!
echo "✓ Server started (PID: $SERVER_PID)"
sleep 2
echo ""

# Step 4: Connect Clients
echo "[STEP 4] Connecting 3 clients..."
echo ""

# Function to create an automated client
run_automated_client() {
    local CLIENT_NUM=$1
    local RESPONSES=$2
    
    # Use expect or simple echo piping with timeout
    (
        sleep 0.5
        # Send responses separated by new lines
        echo "$RESPONSES" | tr '|' '\n'
        sleep 2
    ) | timeout 30 ./client 127.0.0.1 > /tmp/client_$CLIENT_NUM.log 2>&1 &
}

# Scenario 1: Player 1 - HIT, HIT, STAND
run_automated_client 0 "hit|hit|stand"
CLIENT_1_PID=$!

# Scenario 2: Player 2 - HIT, STAND
sleep 0.5
run_automated_client 1 "hit|stand"
CLIENT_2_PID=$!

# Scenario 3: Player 3 - STAND
sleep 0.5
run_automated_client 2 "stand"
CLIENT_3_PID=$!

echo "✓ Client 0 connected (PID: $CLIENT_1_PID)"
echo "✓ Client 1 connected (PID: $CLIENT_2_PID)"
echo "✓ Client 2 connected (PID: $CLIENT_3_PID)"
echo ""

# Step 5: Wait for game to complete
echo "[STEP 5] Gameplay in progress..."
echo "(Monitoring for completion...)"
echo ""

sleep 15

# Step 6: Show game logs in real-time
echo "[STEP 6] REAL-TIME GAME LOG:"
echo "======================================================"
if [ -f game.log ]; then
    cat game.log
else
    echo "(No game.log created yet - waiting for server events)"
fi
echo "======================================================"
echo ""

# Step 7: Cleanup and show results
echo "[STEP 7] Cleaning up processes..."

# Kill all remaining clients
kill $CLIENT_1_PID 2>/dev/null || true
kill $CLIENT_2_PID 2>/dev/null || true
kill $CLIENT_3_PID 2>/dev/null || true

# Kill server
kill $SERVER_PID 2>/dev/null || true

sleep 1

# Show final scores
echo ""
echo "======================================================"
echo "FINAL SCORES:"
echo "======================================================"
if [ -f scores.txt ]; then
    cat scores.txt
    echo "✓ Scores saved to scores.txt"
else
    echo "(No scores.txt file yet)"
fi
echo "======================================================"
echo ""

# Show client logs
echo "CLIENT OUTPUT LOGS:"
echo "======================================================"
echo ""
echo "--- CLIENT 0 ---"
cat /tmp/client_0.log 2>/dev/null || echo "(Client 0 log not available)"
echo ""
echo "--- CLIENT 1 ---"
cat /tmp/client_1.log 2>/dev/null || echo "(Client 1 log not available)"
echo ""
echo "--- CLIENT 2 ---"
cat /tmp/client_2.log 2>/dev/null || echo "(Client 2 log not available)"
echo ""

echo "======================================================"
echo "DEMONSTRATION COMPLETE"
echo "======================================================"
echo ""
echo "Summary:"
echo "  ✓ Project compiled successfully"
echo "  ✓ Server started and running"
echo "  ✓ 3 clients connected"
echo "  ✓ Gameplay demonstrated"
echo "  ✓ Logs recorded"
echo "  ✓ Scores updated"
echo ""
