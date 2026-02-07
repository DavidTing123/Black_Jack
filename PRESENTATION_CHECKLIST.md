# Black Jack Game - Quick Reference & Checklist

## Pre-Demonstration Setup

```bash
cd /home/leongweilee/Downloads/Black_Jack-main

# Option 1: Automated Demonstration (runs everything)
bash demo.sh

# Option 2: Manual Demonstration (step-by-step)
# Follow DEMO_GUIDE.md for detailed instructions
```

---

## 6 Demonstration Requirements (Checklist)

### ✓ Requirement 1: Compilation using make
```bash
make clean
make all
```
**Verification**: Both `server` and `client` binaries exist with no errors

---

### ✓ Requirement 2: Running server and connecting 3 clients

**Terminal 1 - Server**:
```bash
./server
# Output: "Blackjack Server ready for PvP on port 8888..."
```

**Terminals 2-4 - Clients**:
```bash
./client 127.0.0.1
./client 127.0.0.1
./client 127.0.0.1
```
**Verification**: Each client shows "[CLIENT] Connected to server"

---

### ✓ Requirement 3: Full rounds of gameplay

**Player Actions**:
```
Your action: hit    # Draw a card
Your action: stand  # End your turn
```

**Example Round**:
```
Player 0: Hit → Hit → Stand (18 points)
Player 1: Hit → Stand (20 points)  
Player 2: Stand (19 points)
Result: Player 1 WINS with 20 points!
```

---

### ✓ Requirement 4: Concurrent logging (game.log)

**View in real-time**:
```bash
tail -f game.log
```

**Expected entries**:
```
[2026-02-08 14:32:45] [SYSTEM] Game Started - 3 players connected
[2026-02-08 14:32:48] [PLAYER_0] Drew card: King♥  Total: 15
[2026-02-08 14:32:50] [PLAYER_0] Action: HIT
[2026-02-08 14:32:51] [PLAYER_0] Drew card: 3♣  Total: 18
```

---

### ✓ Requirement 5: Updated scores.txt

**View after game ends**:
```bash
cat scores.txt
```

**Expected format**:
```
Player 0: Wins=1, Losses=0, Total_Score=19
Player 1: Wins=1, Losses=1, Total_Score=40
Player 2: Wins=0, Losses=2, Total_Score=18
```

---

### ✓ Requirement 6: Student Presentations

Each team member presents their component:

| Member | Component | Duration |
|--------|-----------|----------|
| Member 1 | Server Architecture & Networking | 2 min |
| Member 2 | Shared Memory & Synchronization | 2 min |
| Member 3 | Game Logic & Scheduling | 2 min |
| Member 4 | Logging & Persistence | 1 min |

---

## Critical System Files

| File | Purpose | Key Feature |
|------|---------|-------------|
| [src/server.c](src/server.c) | Main server | Accepts client connections, fork() per client |
| [src/client.c](src/client.c) | Client interface | TCP socket communication |
| [src/shared_mem.c](src/shared_mem.c) | IPC setup | mmap shared memory, POSIX semaphores |
| [src/game_logic.c](src/game_logic.c) | Game rules | Card dealing, win/loss logic, Ace handling |
| [src/logger.c](src/logger.c) | Event logging | Thread-safe logging to game.log |
| [src/scheduler.c](src/scheduler.c) | Round management | Synchronizes turn-based gameplay |
| [src/scores.c](src/scores.c) | Score persistence | Reads/writes scores.txt |
| [include/game_state.h](include/game_state.h) | Shared data structure | Central game state in shared memory |

---

## Key Architecture Points

### 🔗 Networking
- TCP sockets on port 8888
- Client-server model
- Messages: Game state, player actions, results

### 🔀 Concurrency  
- Server: fork() creates child process per client
- Scheduler: pthread handles round timing
- Synchronization: POSIX semaphores prevent race conditions

### 💾 IPC (Inter-Process Communication)
- Shared Memory: Game state lives in mmap region
- Semaphores: Coordinate client access
- File locks: Protect scores.txt updates

### 📊 Game State
```c
struct GameState {
    Player players[5];              // Up to 5 players
    int connected_count;            // Current player count
    Card deck[52];                  // Card deck
    int deck_index;                 // Current card position
    sem_t deck_sem;                 // Protect deck access
    sem_t score_sem;                // Protect score updates
    bool game_over;                 // Round end flag
    time_t game_start;              // Round timestamp
}
```

---

## Quick Troubleshooting

| Problem | Solution |
|---------|----------|
| "Port already in use" | `pkill -f "./server"` wait 2 seconds |
| "IPC error" | `rm -f /dev/shm/blackjack_shm /dev/shm/sem.bj_*` |
| "Connection refused" | Ensure server started with `./server` |
| Client freezes | Check if waiting for input, type `hit` or `stand` |
| game.log not appearing | Check logger enabled in logger.c |

---

## Presentation Timeline

```
5:00 min - Project Overview & Architecture
1:30 min - Demonstrate make compilation  
1:30 min - Start server & connect clients
3:00 min - Play 2-3 game rounds
1:00 min - Show game.log (tail -f)
1:00 min - Show scores.txt
2:00 min - Team presentations
_______________
15:00 min - TOTAL
```

---

## Command Quick Reference

```bash
# Cleanup & Compile
make clean && make all

# Run Server
./server

# Run Client (separate terminal)
./client 127.0.0.1

# View Game Log
tail -f game.log

# View Scores
cat scores.txt

# Clean IPC
rm -f /dev/shm/blackjack_shm /dev/shm/sem.bj_*

# Kill Server
pkill -f "./server"
```

---

## Notes for Instructors/Presenters

- **Minimum players**: 2-3 to start game
- **Maximum players**: 5 (MAX_PLAYERS in header)
- **Deck**: Standard 52-card deck (handles reshuffle when depleted)
- **Ace handling**: Automatically adjusts (11 if possible, else 1)
- **Bust**: Any hand >21 is automatic loss
- **Blackjack**: Exactly 21 on first 2 cards (bonus - shows differently)
- **Thread safety**: All critical sections protected by semaphores
- **Score persistence**: Survives server restarts (until scores.txt cleared)

---

## Files Generated During Execution

| File | Content |
|------|---------|
| `game.log` | Timestamped game events (real-time) |
| `scores.txt` | Win/loss records per player |
| `server` | Server binary |
| `client` | Client binary |
| `/tmp/server.log` | Server debug output |
| `/tmp/client_*.log` | Individual client outputs |

---

**Good luck with your presentation!** 🎴
