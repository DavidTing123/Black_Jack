#ifndef LOGGER_H
#define LOGGER_H

// Initialize and shutdown logger
void init_logger();
void shutdown_logger();

// Core logging function
void log_event(const char* type, const char* details);

// Specific event loggers
void log_player_connect(int id);
void log_player_disconnect(int id);
void log_card_dealt(int id, int val);
void log_player_action(int id, const char* act, int pts);
void log_game_start(int count);
void log_game_end(int winner);
void log_game_round(int round_num);
void log_player_bust(int id);
void log_player_stand(int id, int points);

#endif
