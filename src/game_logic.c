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
    memset(gs->players, 0, sizeof(gs->players)); // Robust zero-init
    gs->current_turn = 0;
    gs->game_active = false;
    gs->game_over = false;
    gs->winner = -1;
    gs->round_number = 0;
    srand(time(NULL));
    init_deck(gs);
}

// Unified turn management helper
int find_next_player(GameState *gs, int current) {
    int next = current;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        next = (next + 1) % MAX_PLAYERS;
        PlayerState *np = &gs->players[next];
        // Must be connected, active, not standing, and not busted
        if (np->connected && np->active && !np->standing && np->points <= 21) {
            return next;
        }
    }
    return -1; // No valid next player
}

// --- NEW FUNCTIONS FOR MULTIPLE ROUNDS ---

void reset_player_state(PlayerState *p) {
    p->card_count = 0;
    p->points = 0;
    p->standing = false;
    p->last_active = 0; // Reset timer
    memset(p->cards, 0, sizeof(p->cards));
}

void reset_game_round(GameState *gs) {
    // Reset all players
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (gs->players[i].connected) {
            reset_player_state(&gs->players[i]);
            gs->players[i].active = true;
        }
    }
    
    // Reset game state
    gs->current_turn = 0;
    gs->game_over = false;
    gs->game_active = true;
    gs->winner = -1;
    gs->round_number++;
    
    // Initialize first player's timer
    gs->players[0].last_active = time(NULL);
    
    // Reinitialize deck if running low
    if (gs->deck_idx > DECK_SIZE - 20) {
        init_deck(gs);
    }
    
    // Deal initial cards to connected players
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (gs->players[i].connected) {
            PlayerState *p = &gs->players[i];
            p->cards[p->card_count++] = draw_card(gs);
            p->cards[p->card_count++] = draw_card(gs);
            p->points = calculate_points(p->cards, p->card_count);
            // Automatically stand if 21 (Blackjack)
            if (p->points >= 21) {
                p->standing = true;
            } else {
                p->standing = false;
            }
        }
    }
}

bool ask_players_to_continue(GameState *gs, int sock, int my_id) {
    char buffer[1024];
    
    // Send continue prompt to this client
    sprintf(buffer, "MESSAGE: Do you want to play another round? (yes/no)\n");
    send(sock, buffer, strlen(buffer), 0);
    
    // Receive response from this client
    memset(buffer, 0, sizeof(buffer));
    recv(sock, buffer, sizeof(buffer) - 1, 0);
    
    // Remove newline if present
    buffer[strcspn(buffer, "\n")] = 0;
    
    // Store the player's vote
    bool wants_to_continue = false;
    if (strncasecmp(buffer, "yes", 3) == 0) {
        gs->players[my_id].connected = true;  // Keep player connected
        wants_to_continue = true;
    } else {
        gs->players[my_id].connected = false; // Player wants to quit
        wants_to_continue = false;
    }
    
    return wants_to_continue;
}

// CHECK IF ALL PLAYERS HAVE FINISHED THEIR TURNS
bool all_players_done(GameState *gs) {
    int active_players = 0;
    int finished_players = 0;
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (gs->players[i].connected && gs->players[i].active) {
            // A player is active if they have cards but haven't finished
            active_players++;
            if (gs->players[i].standing || gs->players[i].points >= 21) {
                finished_players++;
            }
        }
    }
    
    return (active_players > 0 && active_players == finished_players);
}

void determine_winner(GameState *gs) {
    extern void log_game_end(int winner);
    extern void record_loss(int player_id);
    extern void update_score(int player_id, int score);
    
    int max_points = -1;
    int winner = -1;

    // Find the player with highest score <= 21
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (gs->players[i].connected && gs->players[i].active &&
            gs->players[i].points <= 21 && 
            gs->players[i].points > max_points) {
            max_points = gs->players[i].points;
            winner = i;
        }
    }
    
    // If no one stayed <= 21, check who was connected at all
    if (winner == -1) {
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (gs->players[i].connected && gs->players[i].active) {
                winner = i; // Arbitrary winner if all bust (first player)
                break;
            }
        }
    }

    gs->winner = winner;
    gs->game_over = true;
    gs->game_active = false;
    
    if (winner != -1) {
        log_game_end(winner);
        update_score(winner, gs->players[winner].points);
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (gs->players[i].connected && gs->players[i].active && i != winner) {
                record_loss(i);
            }
        }
    }
}

// --- 2. MAIN CLIENT HANDLER (UPDATED FOR MULTIPLE ROUNDS) ---

void handle_client(int sock, int id, GameState *gs) {
    char buffer[1024], out_buf[2048], card_list[256];
    PlayerState *p = &gs->players[id];
    
    // Initialize player
    p->player_id = id;
    p->connected = true;
    p->active = true;
    reset_player_state(p);
    
    // Wait for PvP start
    send(sock, "MESSAGE: Waiting for Player 2 to join...\n", 42, 0);
    while (gs->connected_count < 2) { 
        usleep(100000); 
    }
    
    // Set initial round number
    if (id == 0) {
        gs->round_number = 1;
        reset_game_round(gs);
    } else {
        // Wait for player 0 to initialize the game
        while (gs->round_number == 0) {
            usleep(100000);
        }
    }
    
    // MAIN GAME LOOP - Handles multiple rounds
    bool continue_playing = true;
    
    while (continue_playing && gs->players[id].connected) {
        // Send round info
        sprintf(out_buf, "MESSAGE: Starting Round %d\n", gs->round_number);
        send(sock, out_buf, strlen(out_buf), 0);
        
        // Reset player state for this round
        reset_player_state(p);
        
        // Deal initial cards if not already dealt
        if (p->card_count == 0) {
            p->cards[p->card_count++] = draw_card(gs);
            p->cards[p->card_count++] = draw_card(gs);
            p->points = calculate_points(p->cards, p->card_count);
            if (p->points >= 21) {
                p->standing = true;
            }
        }
        
        // GAME ROUND LOOP
        while (!gs->game_over && gs->players[id].connected) {
            // Prepare Card List String
            memset(card_list, 0, sizeof(card_list));
            for(int i = 0; i < p->card_count; i++) {
                const char* name = get_card_name(p->cards[i]);
                strcat(card_list, name);
                if (i < p->card_count - 1) {
                    strcat(card_list, ",");
                }
            }

            // --- SEND THE STATE BLOCK ---
            sprintf(out_buf, 
                "STATE: turn=%d player_id=%d cards=%s points=%d standing=%s\n",
                gs->current_turn, id, card_list, p->points, 
                p->standing ? "true" : "false");
            send(sock, out_buf, strlen(out_buf), 0);

            if (gs->current_turn != id) {
                sprintf(out_buf, "MESSAGE: Not Player %d's turn. Waiting...\n", id);
                send(sock, out_buf, strlen(out_buf), 0);
                while(gs->current_turn != id && !gs->game_over && gs->players[id].connected) { 
                    usleep(200000); 
                }
                continue; 
            }

            // --- PLAYER ACTION ---
            if (!p->standing && p->points < 21 && p->card_count < 5) {
                send(sock, "MESSAGE: Player's turn! hit or stand?\nYour action: ", 52, 0);
                memset(buffer, 0, sizeof(buffer));
                int bytes_received = recv(sock, buffer, 1024, 0);
                if (bytes_received <= 0) {
                    gs->players[id].connected = false;
                    printf("[SERVER] Player %d disconnected.\n", id);
                    break;
                }

                // Remove newline
                buffer[strcspn(buffer, "\n")] = 0;
                
                if (strncasecmp(buffer, "hit", 3) == 0) {
                    p->cards[p->card_count++] = draw_card(gs);
                    p->points = calculate_points(p->cards, p->card_count);
                    // Stand if 21+, or if 5 cards reached
                    if (p->points >= 21 || p->card_count >= 5) {
                        p->standing = true;
                    }
                } else if (strncasecmp(buffer, "stand", 5) == 0) {
                    p->standing = true;
                }
            }

            // --- DYNAMIC TURN SWITCHING ---
            sem_wait(&gs->turn_sem);

            int next_player = find_next_player(gs, gs->current_turn);
            
            if (next_player != -1) {
                gs->current_turn = next_player;
                gs->players[next_player].last_active = time(NULL); // Reset timer for next
            }
            
            // Check if game should end
            if (all_players_done(gs)) {
                gs->game_over = true;
            }

            sem_post(&gs->turn_sem);

            sem_post(&gs->turn_sem);
        }

        // --- GAME OVER SUMMARY ---
        if (gs->game_over && gs->players[id].connected) {
            // Ensure winner is determined (Scheduler might have done it, or we do it)
            if (gs->winner == -1 && gs->game_over) {
                determine_winner(gs);
            }
            
            int winner = gs->winner;
            if (winner != -1) {
                sprintf(out_buf, "STATE: Game Over\nMESSAGE: Winner is Player %d with %d points\n", 
                        winner, gs->players[winner].points);
                send(sock, out_buf, strlen(out_buf), 0);
            }
            
            // Wait a moment before asking to continue
            usleep(500000);
            
            // Ask if player wants to continue
            bool wants_to_continue = ask_players_to_continue(gs, sock, id);
            
            if (!wants_to_continue) {
                sprintf(out_buf, "MESSAGE: Player %d is leaving. Thanks for playing!\n", id);
                send(sock, out_buf, strlen(out_buf), 0);
                continue_playing = false;
                gs->players[id].connected = false;
            } else {
                // Wait for all players to decide
                sprintf(out_buf, "MESSAGE: Waiting for other players to decide...\n");
                send(sock, out_buf, strlen(out_buf), 0);
                
                // Count how many players want to continue
                int players_continuing = 0;
                for (int i = 0; i < gs->connected_count; i++) {
                    if (gs->players[i].connected) players_continuing++;
                }
                
                // Wait for all players to respond
                usleep(1000000);
                
                if (players_continuing < 2) {
                    sprintf(out_buf, "MESSAGE: Not enough players to continue. Game ending.\n");
                    send(sock, out_buf, strlen(out_buf), 0);
                    continue_playing = false;
                } else {
                    // Reset for next round
                    gs->game_over = false;
                    
                    // Wait for player 0 to reset the game
                    if (id == 0) {
                        reset_game_round(gs);
                    } else {
                        while (gs->game_over) {
                            usleep(100000);
                        }
                    }
                    
                    // Reset this player's state for new round
                    reset_player_state(p);
                }
            }
        }
        
        // Check if we should exit the loop
        if (!gs->players[id].connected) {
            continue_playing = false;
        }
    }
    
    // Player is leaving
    gs->players[id].connected = false;
    gs->players[id].active = false;
    
    // Decrease connected count
    sem_wait(&gs->score_sem);
    if (gs->connected_count > 0) {
        gs->connected_count--;
    }
    sem_post(&gs->score_sem);
    
    printf("[SERVER] Player %d disconnected. Remaining players: %d\n", id, gs->connected_count);
    
    close(sock);
}