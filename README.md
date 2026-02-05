# Black_Jack
This project is a server based on a hybrid concurrency model, developed in C under a Linux environment. It supports 3to5 players participating in the game through a command-line interface, using a combination of processes (fork) and threads (pthread) architecture to achieve player session isolation and concurrent execution of internal server tasks.

## How to Run

1.  **Clean and Rebuild** (Ensures fresh compile):
    ```bash
    make clean && make
    ```

2.  **Run the Server**:
    ```bash
    ./server
    ```
    The server will automatically fork 3 player processes. The game runs automatically until all players stand, at which point it terminates.

