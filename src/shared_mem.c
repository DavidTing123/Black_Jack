#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include "shared_mem.h"

/**
 * Creates and maps a shared memory segment for the GameState.
 * Also initializes all semaphores for process synchronization.
 */
GameState* setup_shared_memory() {
    // 1. Open (or create) the shared memory object
    int shm_fd = shm_open("/blackjack_shm", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("[ERROR] shm_open failed");
        return NULL;
    }

    // 2. Set the size of the shared memory segment
    if (ftruncate(shm_fd, sizeof(GameState)) == -1) {
        perror("[ERROR] ftruncate failed");
        close(shm_fd);
        return NULL;
    }

    // 3. Map the segment into this process's memory space
    GameState *gs = mmap(NULL, sizeof(GameState), 
                         PROT_READ | PROT_WRITE, 
                         MAP_SHARED, shm_fd, 0);
    
    if (gs == MAP_FAILED) {
        perror("[ERROR] mmap failed");
        close(shm_fd);
        return NULL;
    }

    // --- MEMBER 4 SYNCHRONIZATION INITIALIZATION ---
    // All semaphores use '1' as the second argument to indicate 
    // they are shared across processes (POSIX requirement).

    // Protects the deck and card drawing
    sem_init(&gs->deck_mutex, 1, 1); 

    // Used to manage player turns (initialized to 0 if used for blocking)
    sem_init(&gs->turn_sem, 1, 1); 

    // Protects score updates and winner calculation
    sem_init(&gs->score_sem, 1, 1); 

    // File descriptor is no longer needed after mapping
    close(shm_fd);
    return gs;
}

/**
 * Unmaps the shared memory and removes the object from the system.
 */
void cleanup_shared_memory(GameState *gs) {
    if (gs != NULL) {
        // Destroy all semaphores to release system resources
        sem_destroy(&gs->deck_mutex);
        sem_destroy(&gs->turn_sem);
        sem_destroy(&gs->score_sem);
        
        // Unmap the memory from the current process
        munmap(gs, sizeof(GameState));
        
        // Remove the named shared memory object
        if (shm_unlink("/blackjack_shm") == 0) {
            printf("[INFO] Shared memory and all semaphores cleaned up.\n");
        } else {
            perror("[WARNING] shm_unlink failed");
        }
    }
}