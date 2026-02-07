#ifndef SCORES_H
#define SCORES_H

// Initialize and shutdown score system
void init_score_system();
void shutdown_score_system();

// Update scores
void update_score(int player_id, int score);
void record_loss(int player_id);

#endif
