#ifndef AI_H
#define AI_H

#include <stdbool.h>
#include "game.h"

#define AI_MAX_TARGETS 128

typedef enum {
    AI_EASY = 1,        // Czysto losowy ostrzał
    AI_MEDIUM,          // Inteligentne polowanie (Hunt & Target)
    AI_HARD             // Optymalizacja parzystości pól + Hunt & Target
} AIDifficulty;

typedef struct {
    int x;
    int y;
} AICoord;

typedef struct {
    AIDifficulty difficulty;
    bool targeting;                     // true gdy bot tropi trafiony statek
    AICoord targets[AI_MAX_TARGETS];    // Kolejka pól do sprawdzenia
    int target_count;                   // Liczba pól w kolejce
    AICoord first_hit;                  // Pierwsze trafienie obecnego statku
    AICoord last_hit;                   // Ostatnie trafienie
} AIState;

/* Inicjalizacja stanu bota */
void ai_init(AIState *ai, AIDifficulty diff);

/* Resetowanie kolejki celów */
void ai_reset_targets(AIState *ai);

/* Wygenerowanie współrzędnych kolejnego strzału bota */
void ai_make_move(AIState *ai, const Board *tracking_board, int *out_x, int *out_y);

/* Powiadomienie bota o wyniku strzału (aktualizacja maszyny stanów bota) */
void ai_notify_result(AIState *ai, int x, int y, ShotResult result, const Board *tracking_board);

/* Automatyczne, losowe rozstawienie floty na planszy z zachowaniem reguł */
bool ai_auto_place_ships(Board *board, const int *ship_sizes, int ship_count);

/* Zwraca domyślny zestaw statków dopasowany do rozmiaru planszy */
int ai_get_default_fleet(int board_w, int board_h, int *out_sizes, int max_ships);

#endif // AI_H
