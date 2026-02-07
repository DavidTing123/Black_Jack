# Black Jack Game - Demonstration Guide

## Overview
This guide provides step-by-step instructions for demonstrating the multiplayer Black Jack game for your project presentation.

---

## Demonstration Checklist

- [ ] 1. Compilation using make
- [ ] 2. Running the server
- [ ] 3. Connecting 3 client players
- [ ] 4. Playing full rounds of gameplay
- [ ] 5. Showing concurrent logging (game.log)
- [ ] 6. Displaying updated scores (scores.txt)
- [ ] 7. Student presentations of respective components

---

## Step-by-Step Instructions

### STEP 1: Clean Compilation ✓

**Purpose**: Verify the project compiles successfully from source.

**Commands**:
```bash
cd /home/leongwelee/Downloads/Black_Jack-main
make clean
make all
```

**Expected Output**:
```
rm -f server client src/*.o game.log
Cleanup complete.
gcc -pthread -Wall -Wextra -g -I./include -c src/server.c -o src/server.o
[... more compilation lines ...]
Compilation successful!
```

**What to point out**:
- No compilation errors or warnings
- Both `server` and `client` binaries are generated

---

### STEP 2: Start the Server

**Purpose**: Initialize the game server and prepare it to accept client connections.

**Command** (Terminal 1):
```bash
./server
```

**Expected Output**:
```
Blackjack Server ready for PvP on port 8888...
```

**What to point out**:
- Server is listening on port 8888
- Ready to accept client connections
- Background scheduler thread is running (synchronizes rounds)

---

### STEP 3: Connect Client 1 (Player 0)

**Purpose**: Connect the first player to the game.

**Command** (Terminal 2):
```bash
./client 127.0.0.1
```

**Expected Output**:
```
[CLIENT] Connected to server.
[Waiting for other players...]
```

**What to point out**:
- Successfully connected via socket to server
- IPC shared memory is being accessed
- Waiting for other players to join

---

### STEP 4: Connect Client 2 (Player 1)

**Purpose**: Add a second player.

**Command** (Terminal 3):
```bash
./client 127.0.0.1
```

**Expected Output**:
```
[CLIENT] Connected to server.
[Waiting for other players...]
```

---

### STEP 5: Connect Client 3 (Player 2)

**Purpose**: Add the third player (game can start now with minimum 2-3 players).

**Command** (Terminal 4):
```bash
./client 127.0.0.1
```

**Expected Output**:
```
[CLIENT] Connected to server.
=== ROUND 1 STARTING ===
Player 0 Initial Hand: [Card1] [Card2] = XX points

Your action:
```

**What to point out**:
- All 3 players are now connected
- Each player has received their initial 2-card hand
- Semaphores have synchronized all players
- Game state is managed in shared memory

---

### STEP 6: Play Demonstration Rounds

#### Round 1 - Player 0's Turn:
When prompted:
```
Your action:
>
```

**Type**: `hit` (Demonstrate drawing additional cards)
```
Your action:
> hit
You drew a [Card]. Your hand: [Cards] = XX points

Your action:
>
```

**Type**: `hit` again (Optional - get closer to 21)
```
You drew another [Card]. Your hand: [Cards] = XX total

Your action:
>
```

**Type**: `stand` (End your turn)
```
Your turn has ended. Waiting for other players...
```

#### Round 1 - Player 1's Turn:
When it's their turn, they can similarly `hit` or `stand`.

#### Round 1 - Player 2's Turn:
When it's their turn, they can play their turn.

#### After All Players Finish:
```
=== ROUND 1 RESULTS ===
Player 0: [Cards] = 19 points - WIN
Player 1: [Cards] = 20 points - LOSE  
Player 2: [Cards] = 21 points - BLACKJACK!

Winner: Player 2 (21 points)
```

**What to point out**:
- Turn-based gameplay (one player per round)
- Semaphores ensure no race conditions
- Proper card handling and point calculations
- Ace adjustment (11 or 1)
- Bust detection (>21)

---

### STEP 7: Play Another Round (Optional)

After results, each client will be prompted:
```
Play another round? (yes/no)
>
```

**Type**: `yes` to continue playing
```
yes

=== ROUND 2 STARTING ===
[Game continues...]
```

**Type**: `no` to exit
```
no
[CLIENT] Game ended. Final scores:
Player 0: X wins, Y losses
Player 1: ...
[CLIENT] Disconnecting...
```

---

## STEP 8: Show Concurrent Logging

**Purpose**: Demonstrate real-time logging of game events.

**While game is running** (Terminal 5):
```bash
tail -f game.log
```

**Expected Output**:
```
[2026-02-08 14:32:45] [SYSTEM] Game Started - 3 players connected
[2026-02-08 14:32:46] [PLAYER_0] Drew card: 5♠
[2026-02-08 14:32:46] [PLAYER_0] Drew card: King♥  Total: 15
[2026-02-08 14:32:48] [PLAYER_0] Action: HIT
[2026-02-08 14:32:48] [PLAYER_0] Drew card: 3♣  Total: 18
[2026-02-08 14:32:50] [PLAYER_0] Action: STAND
[2026-02-08 14:32:51] [PLAYER_1] Drew card: 10♦
[2026-02-08 14:32:51] [PLAYER_1] Drew card: Jack♠  Total: 20
...
```

**What to point out**:
- Each action is timestamped
- Logging happens concurrently (multiple clients running)
- Event types: System, Player Actions, Results
- Thread-safe logging (no garbled output)

---

## STEP 9: Display Updated Scores

**After a game ends** (Any Terminal):
```bash
cat scores.txt
```

**Expected Output**:
```
Player 0: Wins=1, Losses=0, Total_Score=19
Player 1: Wins=0, Losses=1, Total_Score=20
Player 2: Wins=1, Losses=0, Total_Score=21
```

**What to point out**:
- Persistent storage of player statistics
- Scores accumulate across multiple rounds
- File is updated after each round
- Thread-safe file operations with semaphores

---

## STEP 10: Component Responsibilities

Each team member should present their component:

### Team Member 1 - [Name]:
**Component**: Server Architecture & Client Handler
- Explain TCP socket setup
- Client connection handling
- Process forking for each client
- Message protocol between server and client

### Team Member 2 - [Name]:
**Component**: Shared Memory & Synchronization
- `setup_shared_memory()` - Creates shared memory
- Semaphore usage for synchronization
- Game state structure in shared memory
- Race condition prevention

### Team Member 3 - [Name]:
**Component**: Game Logic
- Card shuffling and dealing
- Hand value calculation
- Ace handling (11 vs 1)
- Win/loss determination
- Game round scheduling

### Team Member 4 - [Name]:
**Component**: Logging & Persistence
- Concurrent logging to game.log
- Score file management (scores.txt)
- Thread-safe file operations
- Timestamp formatting

---

## Troubleshooting

### Issue: "Port already in use"
```bash
# Kill any existing server process
pkill -f "./server"
# Wait a few seconds
sleep 2
./server
```

### Issue: "IPC error" or "shared memory already exists"
```bash
# Clean up IPC resources
rm -f /dev/shm/blackjack_shm /dev/shm/sem.bj_*
```

### Issue: "Connection refused"
- Make sure server is running first
- Verify port 8888 is available
- Check firewall isn't blocking localhost

### Issue: Game doesn't advance
- Check if all clients are connected
- Verify minimum 2-3 players requirement
- Check if a client is frozen (might be waiting for input)

---

## Demonstration Timeline

| Time | Activity | Duration |
|------|----------|----------|
| 0:00 | Explain project architecture | 1 min |
| 1:00 | Run `make clean && make` | 1 min |
| 2:00 | Start server | 30 sec |
| 2:30 | Connect 3 clients | 1 min |
| 3:30 | Play 2-3 rounds manually | 5 min |
| 8:30 | Show game.log with `tail -f` | 1 min |
| 9:30 | Show scores.txt | 1 min |
| 10:30 | Team member presentations | 5 min |
| **15:30** | **Total** | **~15 minutes** |

---

## Key Points to Emphasize

✅ **Concurrency**: Multiple clients playing simultaneously
✅ **Synchronization**: Semaphores prevent race conditions  
✅ **IPC**: Shared memory allows inter-process communication
✅ **Networking**: TCP sockets for client-server communication
✅ **Persistence**: Scores saved to file across sessions
✅ **Logging**: Real-time event tracking with timestamps

---

## Notes

- The demonstration can be run with 2-5 players
- Longer games with more rounds show synchronization better
- Shared memory is cleaned up automatically on server shutdown
- Each client session is independent but shares game state
