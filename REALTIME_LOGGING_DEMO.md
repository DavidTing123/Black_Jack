# How to Show Concurrent Logging (game.log) in Real-Time

## Quick Answer
```bash
tail -f game.log
```
This command shows new log entries as they appear in real-time.

---

## Step-by-Step Demonstration

### SETUP (Before Demonstration)

#### Step 1: Start the Server (Terminal 1)
```bash
cd /home/leongweilee/Downloads/Black_Jack-main
./server
```
**Output:**
```
Blackjack Server ready for PvP on port 8888...
```

---

#### Step 2: Open Logging Window (Terminal 5 - Dedicated)
**While server is running but BEFORE clients connect:**
```bash
tail -f game.log
```

**What you'll see initially:**
```
(Nothing - waiting for game to start)
```

**Important**: Keep this terminal open and visible during the entire demo!

---

#### Step 3: Connect First Client (Terminal 2)
```bash
./client 127.0.0.1
```

**At this moment, you might see in game.log:**
```
[2026-02-08 14:32:45] [SYSTEM] Player 0 connected
```

---

#### Step 4: Connect Second Client (Terminal 3)
```bash
./client 127.0.0.1
```

**game.log update:**
```
[2026-02-08 14:32:46] [SYSTEM] Player 1 connected
```

---

#### Step 5: Connect Third Client (Terminal 4)
```bash
./client 127.0.0.1
```

**game.log update - Game Starts:**
```
[2026-02-08 14:32:47] [SYSTEM] Player 2 connected
[2026-02-08 14:32:48] [SYSTEM] === ROUND 1 STARTED ===
[2026-02-08 14:32:48] [PLAYER_0] Initial hand: 5♠ King♥ = 15 points
[2026-02-08 14:32:49] [PLAYER_1] Initial hand: 2♦ 9♣ = 11 points
[2026-02-08 14:32:49] [PLAYER_2] Initial hand: Jack♠ 7♥ = 17 points
```

---

### REAL-TIME GAMEPLAY (Watch Both Terminals 4 & 5)

#### Terminal 4 (Client) shows:
```
Your action:
> hit
```

#### Simultaneously in Terminal 5 (game.log):
```
[2026-02-08 14:32:50] [PLAYER_0] Action: HIT
[2026-02-08 14:32:50] [PLAYER_0] Drew: 8♣
[2026-02-08 14:32:50] [PLAYER_0] New hand total: 23 points - BUST!
```

---

## Advanced Demonstration Setup

### Option A: Two Side-by-Side Terminals (RECOMMENDED)

**Layout your desktop as:**
```
┌─────────────────────────────────┐
│ Terminal 5: Logging Monitor      │ (tail -f game.log)
│────────────────────────────────── │
│ [2026-02-08 14:32:50] ...       │
│ [2026-02-08 14:32:51] PLAYER_0  │
│ [2026-02-08 14:32:52] PLAYER_1  │
└─────────────────────────────────┘

┌─────────────────────────────────┐
│ Terminal 2-4: Client Games      │
│────────────────────────────────── │
│ Your action: hit                │
│ > hit                           │
│ You drew a 7♣ = 18 total       │
└─────────────────────────────────┘
```

**To arrange in VS Code or Linux desktop:**
1. Open Terminal 1 with server
2. Open Terminal 5 with `tail -f game.log` on the RIGHT side
3. Open Terminal 2 with first client
4. Open clients 3 & 4 on the LEFT side
5. Position windows so you can see game.log updating in real-time

---

### Option B: Programmatic Monitoring (Using watch command)

```bash
watch -n 1 'tail -20 game.log'
```
**Updates every 1 second, showing last 20 lines**

Or with color:
```bash
watch -n 1 'tail -50 game.log | cat'
```

---

## What to Point Out During Demonstration

### 1. **Timing Consistency** (0:00-0:30)
```
[2026-02-08 14:32:48] [SYSTEM] === ROUND 1 STARTED ===
[2026-02-08 14:32:50] [PLAYER_0] Action: HIT
[2026-02-08 14:32:52] [PLAYER_1] Action: STAND
```
✅ Say: *"Notice the timestamps are chronological - events logged in order"*

### 2. **Multiple Players Simultaneously** (0:30-1:00)
```
[2026-02-08 14:32:50] [PLAYER_0] Drew card: 5♣
[2026-02-08 14:32:50] [PLAYER_1] Drew card: Queen♥
[2026-02-08 14:32:51] [PLAYER_0] New total: 18
[2026-02-08 14:32:51] [PLAYER_1] New total: 20
```
✅ Say: *"Multiple players taking actions, all logged without conflicts - shows thread-safe logging!"*

### 3. **Event Categories** (1:00-1:30)
Point out different log types:
- **[SYSTEM]**: Server events
- **[PLAYER_X]**: Individual actions
- **[ROUND_END]**: Results

```bash
grep "\[SYSTEM\]" game.log        # All system events
grep "\[PLAYER_0\]" game.log      # All Player 0 actions
grep "BUST" game.log              # All bust events
grep "WINNER" game.log            # All winners
```

### 4. **Deck Management** (2:00-2:30)
```
[2026-02-08 14:32:48] [SYSTEM] Cards remaining: 50
[2026-02-08 14:32:50] [SYSTEM] Cards remaining: 49
...
[2026-02-08 14:40:12] [SYSTEM] Deck exhausted - RESHUFFLING
```
✅ Say: *"See how the deck is tracked? When we run out, it automatically reshuffles"*

---

## Key Log Entry Formats

### Player Turn Log
```
[TIMESTAMP] [PLAYER_X] Action: HIT
[TIMESTAMP] [PLAYER_X] Drew: [Card Name]
[TIMESTAMP] [PLAYER_X] New total: XX points
```

### Game Event Log
```
[TIMESTAMP] [SYSTEM] === ROUND N STARTED ===
[TIMESTAMP] [SYSTEM] Cards remaining: XX
[TIMESTAMP] [SYSTEM] Deck reshuffled
```

### Round Result Log
```
[TIMESTAMP] [ROUND_END] Winner: Player X with 21 points
[TIMESTAMP] [PLAYER_0] Result: LOSS
[TIMESTAMP] [PLAYER_1] Result: WIN
[TIMESTAMP] [PLAYER_2] Result: LOSS
```

---

## Grep Commands to Highlight Features

**During your presentation, you can run these to show specific aspects:**

1. **Show all busts:**
```bash
grep "BUST\|points > 21" game.log
```

2. **Show all winners:**
```bash
grep "WINNER\|Result: WIN" game.log
```

3. **Show Player 0's complete history:**
```bash
grep "\[PLAYER_0\]" game.log
```

4. **Count total events:**
```bash
wc -l game.log
```

5. **Show rounds in order:**
```bash
grep "ROUND.*STARTED\|ROUND_END" game.log
```

6. **Show only the last 10 events:**
```bash
tail -10 game.log
```

---

## Demonstration Script (1-2 minutes)

### Narration + Actions:

**0:00-0:15** - *"As clients connect and play, all events are logged with timestamps..."*
- Point to game.log showing real-time updates
- Highlight: `[TIMESTAMP] [PLAYER_X] Action:`

**0:15-0:30** - *"Notice multiple players' actions logged concurrently without conflicts..."*
- Show simultaneous PLAYER_0 and PLAYER_1 entries
- Highlight: Thread-safe logging with semaphores

**0:30-0:45** - *"Each card draw, each action, each bust is recorded automatically..."*
- `tail -f game.log` scrolling
- Point to deck tracking

**0:45-1:00** - *"After the game, the log file persists as a complete history of gameplay..."*
- Show file exists: `ls -lh game.log`
- Show line count: `wc -l game.log`

**1:00-1:15** - *"We can filter for specific events..."*
```bash
grep "WINNER" game.log
grep "\[PLAYER_" game.log | wc -l  # Count player actions
```

**1:15-2:00** - *"Let me show you the last round's events..."*
```bash
tail -30 game.log | grep -E "ROUND|WINNER|Result"
```

---

## Pro Tips for Presentation

✅ **TEST BEFOREHAND**
- Run a practice game and capture game.log output
- Know what events to expect
- Have example commands ready

✅ **POSITION WINDOWS CAREFULLY**
- game.log on full screen for audience to see
- Keep it visible during entire gameplay demo
- Use large font (zoom terminal to 150-200%)

✅ **PAUSE FOR EFFECT**
- When a key event happens (BUST, WINNER), pause and point
- Let the audience read the log entry before continuing

✅ **USE FILTERS FOR CLARITY**
- Don't just show raw log - grep for specific patterns
- Highlight different player actions
- Show win/loss counts

✅ **MENTION THREAD SAFETY**
- Point out: No garbled output, no mixed lines
- Each entry is complete and readable
- Explains use of semaphores in logger.c

---

## Common Issues & Solutions

| Issue | Solution |
|-------|----------|
| game.log doesn't exist | Make sure server started first, game activity happened |
| game.log empty | Check logger.c is enabled, game ran long enough |
| Slow updates | Increase game activity (have clients play faster) |
| Text scrolls too fast | Use `less game.log` instead, or increase window size |
| Can't see timestamps | Make sure logger includes timestamps in format string |

---

## Sample game.log Content

```
[2026-02-08 14:32:45] [SYSTEM] Blackjack Server started
[2026-02-08 14:32:46] [SYSTEM] Player 0 connected
[2026-02-08 14:32:47] [SYSTEM] Player 1 connected
[2026-02-08 14:32:48] [SYSTEM] Player 2 connected
[2026-02-08 14:32:49] [SYSTEM] === ROUND 1 STARTED ===
[2026-02-08 14:32:49] [PLAYER_0] Initial hand: 5♠ King♥ = 15
[2026-02-08 14:32:49] [PLAYER_1] Initial hand: 2♦ 9♣ = 11
[2026-02-08 14:32:49] [PLAYER_2] Initial hand: Jack♠ 7♥ = 17
[2026-02-08 14:32:50] [PLAYER_0] Action: HIT
[2026-02-08 14:32:51] [PLAYER_0] Drew: 8♣
[2026-02-08 14:32:51] [PLAYER_0] New total: 23 - BUST!
[2026-02-08 14:32:52] [PLAYER_1] Action: HIT
[2026-02-08 14:32:53] [PLAYER_1] Drew: 9♠
[2026-02-08 14:32:53] [PLAYER_1] New total: 20
[2026-02-08 14:32:54] [PLAYER_1] Action: STAND
[2026-02-08 14:32:55] [PLAYER_2] Action: STAND
[2026-02-08 14:32:56] [SYSTEM] === ROUND 1 RESULTS ===
[2026-02-08 14:32:56] [PLAYER_0] Result: BUST
[2026-02-08 14:32:56] [PLAYER_1] Result: WIN (20 points)
[2026-02-08 14:32:56] [PLAYER_2] Result: LOSS (17 points)
[2026-02-08 14:32:57] [SYSTEM] Cards remaining: 48
[2026-02-08 14:32:58] [SYSTEM] Continue game? Waiting for player responses...
```

---

## One-Liner Quick Demos

During Q&A, if someone asks "Can I see logs for Player X?":

```bash
# All events for specific player
tail -50 game.log | grep "PLAYER_0"

# Count busts
grep "BUST\|> 21" game.log | wc -l

# Show winners
grep "Result: WIN" game.log

# Timeline of a specific round
grep "ROUND 2" game.log
```

---

**Remember: The log.txt file proves your game is concurrent, thread-safe, and resilient!** 🎴
