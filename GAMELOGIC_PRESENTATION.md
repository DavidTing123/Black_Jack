# Game Logic & Scheduling - 1 Minute Presentation

## Overview (30 seconds)
Hi, I'm [Name], and I'll explain the **Game Logic and Scheduling** components of our Black Jack game. These handle the core rules and turn coordination for multiplayer gameplay.

---

## GAME LOGIC (15 seconds)

### Core Responsibilities:
1. **Card Management**
   - Deck initialization with 52 cards (shuffled)
   - Thread-safe card drawing with `sem_wait/sem_post` on `deck_mutex`
   - Automatic reshuffle when deck runs low

2. **Hand Calculation** 
   - `calculate_points()` - Converts cards to point values (2-10 = face, J/Q/K = 10, Ace = 11 or 1)
   - Smart Ace handling: Adjusts from 11→1 if total exceeds 21
   - Example: [Ace, King] = 21 (Blackjack), but [Ace, 5, 7] = 13 (Ace becomes 1)

3. **Round Management**
   - `reset_game_round()` - Clears player hands, deals 2 new cards to each
   - Tracks round number and handles deck reshuffling
   - Determines winner based on highest score ≤21

4. **Win/Loss Logic**
   - Any hand >21 = **Bust** (automatic loss)
   - Highest score ≤21 = **Winner**  
   - Scores persisted to `scores.txt` via `scores.c`

---

## SCHEDULING (15 seconds)

### Turn Coordination:
Running as a **separate pthread**, the scheduler ensures fair turn-based play:

1. **Active Player Loop**
   - Manages `current_turn` variable shared via memory
   - Locks with `sem_wait(&turn_sem)` to prevent race conditions

2. **Three Automatic Turn Passes:**
   - **Timeout**: If player inactive >20 seconds → auto-STAND
   - **Bust**: If points >21 → skip to next player
   - **Standing**: Player chose STAND → move to next

3. **Find Next Active Player**
   - Skips disconnected/busted players
   - Loops through all players to find valid next turn
   - When no players remain → Round ends, determine winner

4. **Round Progression**
   - All players take turns in order (0→1→2→...)
   - After all finish or bust → `determine_winner()`
   - Shows results and prompts for "play again?"

---

## Key Code Snippets

```c
// Smart Ace Handling
int calculate_points(const int *cards, int count) {
    int points = 0, aces = 0;
    for (int i = 0; i < count; i++) {
        int val = cards[i];
        if (val == 1) { aces++; points += 11; }  // Ace starts as 11
        else if (val >= 10) { points += 10; }    // Face cards = 10
        else { points += val; }
    }
    // IF bust, convert Aces from 11→1
    while (points > 21 && aces > 0) { points -= 10; aces--; }
    return points;
}
```

```c
// Scheduler finds next valid player (skip busted/standing)
int find_next_active_player(GameState* gs, int current) {
    int next = current;
    do {
        next = (next + 1) % MAX_PLAYERS;
        PlayerState *np = &gs->players[next];
        if (np->active && np->connected && !np->standing && np->points <= 21) {
            return next;
        }
    } while (loops < MAX_PLAYERS);
    return -1;
}
```

---

## Why This Design?

✅ **Semaphore Protection** - Prevents race conditions when multiple processes access deck
✅ **Auto Timeout** - Games don't hang waiting for AFK players
✅ **Fair Turn Order** - All players treated equally, no player skipped
✅ **Resilient** - Handles disconnects, busts, and stalls automatically

---

## Data Structures Used

| Structure | Purpose |
|-----------|---------|
| `GameState.deck[]` | 52-card array (values 1-13, Ace=1, Jack=11, etc) |
| `GameState.deck_mutex` | Semaphore protecting deck access |
| `GameState.turn_sem` | Semaphore protecting turn state |
| `PlayerState.cards[]` | Current player's hand |
| `PlayerState.points` | Calculated hand value |
| `PlayerState.standing` | Boolean: Player chose STAND |

---

## Summary

**Game Logic** implements the actual Black Jack rules (card math, betting, win conditions) while **Scheduling** manages the complex coordination of 3-5 concurrent players taking turns fairly, handling timeouts, and detecting round completion.

**Total game flow**: Initialize deck → Deal cards → Loop (each player takes turn) → Detect winner → Ask to play again

---

*That's Game Logic & Scheduling. Thank you!*
