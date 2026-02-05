// src/shared_mem.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "../include/game_state.h"

// Forward declaration if not in header, but it is in game_logic.c usually. 
// For now, we initialize simple state here.

GameState* init_shared_memory() {
    // 1. Create shared memory object
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        return NULL;
    }

    // 2. Set size
    if (ftruncate(shm_fd, sizeof(GameState)) == -1) {
        perror("ftruncate failed");
        return NULL;
    }

    // 3. Map into memory
    GameState *gs = mmap(NULL, sizeof(GameState), 
                         PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (gs == MAP_FAILED) {
        perror("mmap failed");
        return NULL;
    }

    // 4. Initialize Semaphores
    // Note: sem_open is better for unrelated processes, but here we can stick pointers 
    // in SHM if we use sem_init(..., 1, ...) for process-shared, 
    // BUT mapped memory addresses might differ. 
    // Named semaphores are safer for cross-process if we don't store the pointers in SHM directly 
    // but reopen them. However, standard pattern: use named semaphores and store names? 
    // Or just open them in main and children.
    
    // For simplicity with the struct provided which has sem_t* pointers: 
    // Putting pointers to local sem_open'd semaphores in SHM is risky if address space differs.
    // However, unrelated processes usually mapped the SHM to different addresses.
    // Better strategy for this struct: 
    // The struct has sem_t *turn_sem. A pointer in SHM points to 'somewhere'. 
    // If we want process-shared unnamed semaphores, they must be IN the SHM struct itself, not pointers.
    // The struct definition in header has `sem_t *turn_sem`. This suggests the AUTHOR intended named semaphores 
    // or didn't think about address spaces. 
    // We will use named semaphores and ignore the pointer fields in SHM for cross-process invocation, 
    // OR we change the struct. 
    // Let's assume we stick to the header. We will use named semaphores. 
    // Each process should open the named semaphore. 
    // But init only needs to CREATE them.
    
    // Unlink old semaphores to ensure fresh start
    sem_unlink(SEM_TURN);
    sem_unlink(SEM_DECK);
    sem_unlink(SEM_SCORE);

    sem_t *sem_turn = sem_open(SEM_TURN, O_CREAT, 0666, 1);
    sem_t *sem_deck = sem_open(SEM_DECK, O_CREAT, 0666, 1);
    sem_t *sem_score = sem_open(SEM_SCORE, O_CREAT, 0666, 1);

    if (sem_turn == SEM_FAILED || sem_deck == SEM_FAILED || sem_score == SEM_FAILED) {
        perror("sem_open failed");
        return NULL;
    }

    // We can store these if the struct is just internal. 
    // But since it's shared, putting local pointers is bad.
    // However, if we fork(), the child inherits the address space copy (COW), 
    // so the pointers MIGHT be valid if they point to library memory? No.
    // Named semaphores return a pointer to the semaphore.
    // Let's just create them here. The server main will hold them. 
    // We will populate the struct pointers just in case, but rely on reopening if needed.
    
    gs->turn_sem = sem_turn;
    gs->deck_sem = sem_deck;
    gs->score_sem = sem_score;

    return gs;
}

void cleanup_shared_memory(GameState *gs) {
    if (gs) {
        // Destroy semaphores logic. 
        // In named semaphores, we close and unlink.
        sem_close(gs->turn_sem);
        sem_close(gs->deck_sem);
        sem_close(gs->score_sem);
        
        munmap(gs, sizeof(GameState));
    }
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_TURN);
    sem_unlink(SEM_DECK);
    sem_unlink(SEM_SCORE);
}
