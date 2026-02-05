#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <semaphore.h>
#include "game_state.h"

// External declaration for the logger
extern void log_event(const char* type, const char* details);

// --- 1. HELPER LOGIC ---

void shuffle_deck(int *deck, int size) {
    for (int i = 0; i < size; i++) {
        int j = rand() % size;
        int temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

void init_deck(GameState *gs) {
    int idx = 0;
    for (int s = 0; s < 4; s++) {
        for (int v = 1; v <= 13; v++) {
            gs->deck[idx++] = v;
        }
    }
    gs->deck_idx = 0;
    shuffle_deck(gs->deck, DECK_SIZE);
}

/**
 * MEMBER 4: Thread-safe card drawing
 * Uses sem_wait and sem_post to prevent race conditions
 */
int draw_card(GameState *gs) {
    sem_wait(&gs->deck_mutex); // --- LOCK ---

    if (gs->deck_idx >= DECK_SIZE) {
        init_deck(gs);
    }
    int card = gs->deck[gs->deck_idx++];

    sem_post(&gs->deck_mutex); // --- UNLOCK ---
    return card;
}

int calculate_points(const int *cards, int count) {
    int points = 0, aces = 0;
    for (int i = 0; i < count; i++) {
        int val = cards[i];
        if (val == 1) { aces++; points += 11; }
        else if (val >= 10) { points += 10; }
        else { points += val; }
    }
    while (points > 21 && aces > 0) { points -= 10; aces--; }
    return points;
}

const char* get_card_name(int val) {
    static char buf[16];
    if (val == 1) return "Ace";
    if (val == 11) return "Jack";
    if (val == 12) return "Queen";
    if (val == 13) return "King";
    sprintf(buf, "%d", val);
    return buf;
}

void init_game_state_struct(GameState *gs) {
    gs->current_turn = 0;
    gs->game_active = true;
    srand(time(NULL));
    init_deck(gs);
}

// --- 2. MAIN CLIENT HANDLER ---

void handle_client(int sock, int id, GameState *gs) {
    char buffer[1024], out_buf[2048], card_list[256];
    PlayerState *p = &gs->players[id];

    // Wait for PvP start
    send(sock, "MESSAGE: Waiting for Player 2 to join...\n", 42, 0);
    while (gs->connected_count < 2) { usleep(100000); }

    // Initial Deal
    p->standing = false;
    p->card_count = 0;
    p->cards[p->card_count++] = draw_card(gs);
    p->cards[p->card_count++] = draw_card(gs);
    p->points = calculate_points(p->cards, p->card_count);

    while (!gs->game_over) {
        // Prepare Card List String
        memset(card_list, 0, sizeof(card_list));
        for(int i=0; i < p->card_count; i++) {
            char val[8];
            sprintf(val, "%d%s", p->cards[i], (i == p->card_count-1 ? "" : ","));
            strcat(card_list, val);
        }

        // --- SEND THE STATE BLOCK ---
        sprintf(out_buf, 
            "STATE: turn=%d player_id=%d cards=%s points=%d standing=%s\n",
            gs->current_turn, id, card_list, p->points, p->standing ? "true" : "false");
        send(sock, out_buf, strlen(out_buf), 0);

        if (gs->current_turn != id) {
            sprintf(out_buf, "MESSAGE: Not Player %d's turn. Waiting...\n", id);
            send(sock, out_buf, strlen(out_buf), 0);
            while(gs->current_turn != id && !gs->game_over) { usleep(200000); }
            continue; 
        }

        // --- PLAYER ACTION ---
        if (!p->standing && p->points <= 21) {
            send(sock, "MESSAGE: Player's turn! hit or stand?\nYour action: ", 52, 0);
            memset(buffer, 0, sizeof(buffer));
            if (recv(sock, buffer, 1024, 0) <= 0) break;

            if (strncasecmp(buffer, "hit", 3) == 0) {
                p->cards[p->card_count++] = draw_card(gs);
                p->points = calculate_points(p->cards, p->card_count);
                if (p->points > 21) p->standing = true;
            } else {
                p->standing = true;
            }
        }

        // SWITCH TURN
        sem_wait(&gs->turn_sem);
        gs->current_turn = (id == 0) ? 1 : 0;
        if (gs->players[0].standing && gs->players[1].standing) gs->game_over = true;
        sem_post(&gs->turn_sem);
    }

    // --- GAME OVER SUMMARY ---
    sprintf(out_buf, "STATE: Game Over\nMESSAGE: Winner is Player %d with %d points\n", 
            (gs->players[0].points > gs->players[1].points && gs->players[0].points <= 21) ? 0 : 1,
            (gs->players[0].points > gs->players[1].points && gs->players[0].points <= 21) ? gs->players[0].points : gs->players[1].points);
    send(sock, out_buf, strlen(out_buf), 0);
    
    close(sock);
}