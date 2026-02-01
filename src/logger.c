#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include "../include/game_state.h"

#define LOG_FILE "game.log"
#define MAX_LOG_ENTRIES 1000

typedef struct {
    char timestamp[20];
    int player_id;
    char event[50];
    char details[100];
} LogEntry;

typedef struct {
    LogEntry entries[MAX_LOG_ENTRIES];
    int head;
    int tail;
    int count;
    pthread_mutex_t lock;
    sem_t sem;
} LogQueue;

LogQueue log_queue;

void init_logger() {
    memset(&log_queue, 0, sizeof(LogQueue));
    pthread_mutex_init(&log_queue.lock, NULL);
    sem_init(&log_queue.sem, 0, 0);
    
    // Clear log file
    FILE *f = fopen(LOG_FILE, "w");
    if (f) fclose(f);
}

void log_event(int player_id, const char *event, const char *details) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
    LogEntry entry;
    strftime(entry.timestamp, sizeof(entry.timestamp), "%Y-%m-%d %H:%M:%S", t);
    entry.player_id = player_id;
    strncpy(entry.event, event, sizeof(entry.event) - 1);
    strncpy(entry.details, details, sizeof(entry.details) - 1);
    
    pthread_mutex_lock(&log_queue.lock);
    
    if (log_queue.count < MAX_LOG_ENTRIES) {
        log_queue.entries[log_queue.tail] = entry;
        log_queue.tail = (log_queue.tail + 1) % MAX_LOG_ENTRIES;
        log_queue.count++;
        sem_post(&log_queue.sem);
    }
    
    pthread_mutex_unlock(&log_queue.lock);
}

void* logger_thread_func(void* arg) {
    printf("[LOGGER] Thread started\n");
    
    while (1) {
        sem_wait(&log_queue.sem);
        
        pthread_mutex_lock(&log_queue.lock);
        
        if (log_queue.count > 0) {
            LogEntry entry = log_queue.entries[log_queue.head];
            log_queue.head = (log_queue.head + 1) % MAX_LOG_ENTRIES;
            log_queue.count--;
            
            pthread_mutex_unlock(&log_queue.lock);
            
            // Write to file
            FILE *f = fopen(LOG_FILE, "a");
            if (f) {
                fprintf(f, "[%s] Player %d: %s - %s\n",
                        entry.timestamp, entry.player_id,
                        entry.event, entry.details);
                fclose(f);
            }
        } else {
            pthread_mutex_unlock(&log_queue.lock);
        }
    }
    
    return NULL;
}