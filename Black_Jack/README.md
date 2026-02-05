# Multiplayer Black Jack Game

A multiplayer console-based Black Jack game (Server/Client architecture) developed in C.

## 🃏 Game Rules

**Objective**: Beat the other players by getting a hand value closest to **21** without going over (Bust).

1.  **Card Values**:
    -   **2-10**: Worth their face value.
    -   **J, Q, K**: Worth **10** points each.
    -   **Ace (A)**: Worth **11** or **1** (automatically adjusts to avoid busting).
2.  **Gameplay**:
    -   The game is turn-based.
    -   Players are dealt **2 initial cards**.
    -   On your turn, you can choose to:
        -   **Hit**: Take another card. The turn passes to the next player.
        -   **Stand**: Keep your current hand and stop playing.
3.  **Winning**:
    -   If you exceed 21 points, you **Bust** and are out.
    -   When all players have stood or busted, the game ends.
    -   The player with the highest score (<= 21) is declared the **Winner**.

## 🛠️ How to Compile

This project uses a `Makefile` for easy compilation. Open your terminal in the project folder and run:

```bash
make all
```

This will create two executable files: `server` and `client`.

## 🚀 How to Run

You will need to run the server first, and then open separate terminals for each player.

### 1. Start the Server
Open a terminal and run:
```bash
./server
```
You will see: `Blackjack Server running on port 8888...`

### 2. Start Players (Clients)
Open a **new terminal window** implementation for each player you want to join.

**Player 0:**
```bash
./client
```
**Player 1:**
```bash
./client
```
*(Repeat for up to 5 players)*

## 🎮 Controls

When it is your turn, the game will prompt you:
```text
Your turn! Hand: 15 points. (hit/stand)
```

Type one of the following commands and press Enter:
-   **`hit`** : Draw a new card.
-   **`stand`** : End your turn for the game.

## 🧪 Technical Details
-   **Architecture**: Client-Server (TCP Sockets).
-   **Concurrency**: Hybrid model using `fork()` for client handling and `pthread` for internal tasks.
-   **IPC**: Uses Shared Memory and Named Semaphores to synchronize game state between processes.

# Multi-Process Blackjack Game (C/POSIX)

A networked, multi-process Blackjack game developed for systems programming. The project features shared memory for game state persistence, POSIX semaphores for synchronization, and a client-server architecture.

## 🚀 Features
- **Multi-Player Support**: Handles multiple concurrent clients using `fork()`.
- **Shared Game State**: All processes access a central deck and game state via `mmap` shared memory.
- **Synchronization**: Uses POSIX semaphores to prevent race conditions during card drawing and score updates.
- **Persistent Gameplay**: Supports a "Play Again" loop allowing multiple rounds per session.
- **Logging**: All game events (Hit, Stand, Win, Loss) are recorded in `game.log`.

## 🛠️ Requirements
- Linux-based OS (MiniOS, Ubuntu, etc.)
- GCC Compiler
- POSIX Libraries (`pthread`, `rt`)

## 📦 Compilation
Use the provided Makefile to compile the server and client:
```bash
make clean
make all

Connect Clients (Open new terminals for each player):
Bash
./client 127.0.0.1
