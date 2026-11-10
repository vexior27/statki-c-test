#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ai.h"

static bool s_random_seeded = false;

static void ensure_random_seeded(void) {
    if (!s_random_seeded) {
        srand((unsigned int)time(NULL));
        s_random_seeded = true;
    }
}

// Sprawdza, czy pole jest jeszcze nieostrzelane z perspektywy gracza/bota.
// Nieostrzelane pole to woda (CELL_EMPTY) LUB niewykryty statek (CELL_SHIP).
// Pola już ostrzelane to CELL_MISS, CELL_HIT oraz CELL_SUNK.
static inline bool is_cell_unshot(CellState state) {
    return (state == CELL_EMPTY || state == CELL_SHIP);
}

void ai_init(AIState *ai, AIDifficulty diff) {
    ensure_random_seeded();
    if (!ai) return;

    ai->difficulty = diff;
    ai->targeting = false;
    ai->target_count = 0;
    ai->first_hit = (AICoord){-1, -1};
    ai->last_hit = (AICoord){-1, -1};
}

void ai_reset_targets(AIState *ai) {
    if (!ai) return;
    ai->targeting = false;
    ai->target_count = 0;
    ai->first_hit = (AICoord){-1, -1};
    ai->last_hit = (AICoord){-1, -1};
}

static bool ai_target_in_queue(const AIState *ai, int x, int y) {
    for (int i = 0; i < ai->target_count; i++) {
        if (ai->targets[i].x == x && ai->targets[i].y == y) {
            return true;
        }
    }
    return false;
}

static void ai_push_target(AIState *ai, const Board *tracking_board, int x, int y) {
    if (ai->target_count >= AI_MAX_TARGETS) return;
    if (!board_is_valid_coord(tracking_board, x, y)) return;
    if (!is_cell_unshot(tracking_board->grid[y][x])) return;
    if (ai_target_in_queue(ai, x, y)) return;

    ai->targets[ai->target_count++] = (AICoord){x, y};
}

void ai_make_move(AIState *ai, const Board *tracking_board, int *out_x, int *out_y) {
    if (!ai || !tracking_board || !out_x || !out_y) return;

    // 1. Tryb TARGETING (dobijanie wykrytego statku)
    if (ai->difficulty >= AI_MEDIUM && ai->targeting) {
        while (ai->target_count > 0) {
            // Pobieramy ostatnio dodany cel (LIFO - kontynuacja kierunku)
            AICoord target = ai->targets[--ai->target_count];

            if (board_is_valid_coord(tracking_board, target.x, target.y) &&
                is_cell_unshot(tracking_board->grid[target.y][target.x])) {
                *out_x = target.x;
                *out_y = target.y;
                return;
            }
        }
        // Jeśli kolejka celów się wyczerpała, wracamy do trybu polowania
        ai->targeting = false;
    }

    // 2. Tryb HUNTING (poszukiwanie nowych statków)
    int w = tracking_board->width;
    int h = tracking_board->height;

    // Zbieranie wszystkich nieostrzelanych komórek (zarówno CELL_EMPTY jak i CELL_SHIP)
    AICoord unshot_cells[w * h];
    int unshot_count = 0;

    // Komórki o optymalnej parzystości (wzór szachownicy dla poziomu HARD)
    AICoord parity_cells[w * h];
    int parity_count = 0;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (is_cell_unshot(tracking_board->grid[y][x])) {
                unshot_cells[unshot_count++] = (AICoord){x, y};
                if ((x + y) % 2 == 0) {
                    parity_cells[parity_count++] = (AICoord){x, y};
                }
            }
        }
    }

    if (unshot_count == 0) {
        *out_x = 0;
        *out_y = 0;
        return;
    }

    // Wybór komórki zgodnie z poziomem trudności
    if (ai->difficulty == AI_HARD && parity_count > 0) {
        int idx = rand() % parity_count;
        *out_x = parity_cells[idx].x;
        *out_y = parity_cells[idx].y;
    } else {
        int idx = rand() % unshot_count;
        *out_x = unshot_cells[idx].x;
        *out_y = unshot_cells[idx].y;
    }
}

void ai_notify_result(AIState *ai, int x, int y, ShotResult result, const Board *tracking_board) {
    if (!ai || !tracking_board) return;

    if (result == SHOT_HIT) {
        ai->targeting = true;

        if (ai->first_hit.x == -1) {
            ai->first_hit = (AICoord){x, y};
        }
        ai->last_hit = (AICoord){x, y};

        // Jeżeli znamy orientację statku (więcej niż 1 trafienie)
        if (ai->first_hit.x != -1 && (ai->first_hit.x != x || ai->first_hit.y != y)) {
            if (ai->first_hit.x == x) {
                // Orientacja pionowa - sprawdzamy w górę i w dół
                ai_push_target(ai, tracking_board, x, y - 1);
                ai_push_target(ai, tracking_board, x, y + 1);
                ai_push_target(ai, tracking_board, ai->first_hit.x, ai->first_hit.y - 1);
                ai_push_target(ai, tracking_board, ai->first_hit.x, ai->first_hit.y + 1);
            } else if (ai->first_hit.y == y) {
                // Orientacja pozioma - sprawdzamy w lewo i w prawo
                ai_push_target(ai, tracking_board, x - 1, y);
                ai_push_target(ai, tracking_board, x + 1, y);
                ai_push_target(ai, tracking_board, ai->first_hit.x - 1, ai->first_hit.y);
                ai_push_target(ai, tracking_board, ai->first_hit.x + 1, ai->first_hit.y);
            }
        } else {
            // Pierwsze trafienie - dodajemy 4 prostopadłych sąsiadów
            ai_push_target(ai, tracking_board, x + 1, y);
            ai_push_target(ai, tracking_board, x - 1, y);
            ai_push_target(ai, tracking_board, x, y + 1);
            ai_push_target(ai, tracking_board, x, y - 1);
        }
    } else if (result == SHOT_SUNK || result == SHOT_WIN) {
        // Statek został zatopiony
        ai->first_hit = (AICoord){-1, -1};
        ai->last_hit = (AICoord){-1, -1};

        // Weryfikacja czy w kolejce pozostały jeszcze jakieś nieostrzelane pola
        int valid_left = 0;
        for (int i = 0; i < ai->target_count; i++) {
            AICoord c = ai->targets[i];
            if (board_is_valid_coord(tracking_board, c.x, c.y) &&
                is_cell_unshot(tracking_board->grid[c.y][c.x])) {
                valid_left++;
            }
        }

        if (valid_left == 0) {
            ai->targeting = false;
            ai->target_count = 0;
        }
    }
}

int ai_get_default_fleet(int board_w, int board_h, int *out_sizes, int max_ships) {
    if (!out_sizes || max_ships <= 0) return 0;

    int count = 0;
    int max_dim = (board_w < board_h) ? board_w : board_h;

    if (max_dim <= 8) {
        // Mała plansza (np. 6x6, 8x8) -> 6 statków: 1x3, 2x2, 3x1
        int fleet[] = {3, 2, 2, 1, 1, 1};
        int f_count = sizeof(fleet) / sizeof(fleet[0]);
        for (int i = 0; i < f_count && count < max_ships; i++) {
            out_sizes[count++] = fleet[i];
        }
    } else if (max_dim >= 14) {
        // Duża plansza (np. 14x14, 16x16) -> 1x5, 2x4, 3x3, 4x2, 5x1
        int fleet[] = {5, 4, 4, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1};
        int f_count = sizeof(fleet) / sizeof(fleet[0]);
        for (int i = 0; i < f_count && count < max_ships; i++) {
            out_sizes[count++] = fleet[i];
        }
    } else {
        // Standardowa plansza 10x10 -> 10 statków: 1x4, 2x3, 3x2, 4x1
        int fleet[] = {4, 3, 3, 2, 2, 2, 1, 1, 1, 1};
        int f_count = sizeof(fleet) / sizeof(fleet[0]);
        for (int i = 0; i < f_count && count < max_ships; i++) {
            out_sizes[count++] = fleet[i];
        }
    }

    return count;
}

bool ai_auto_place_ships(Board *board, const int *ship_sizes, int ship_count) {
    if (!board || !ship_sizes || ship_count <= 0) return false;
    ensure_random_seeded();

    // Sortowanie rozmiarów statków malejąco (największe stawiamy najpierw)
    int sizes[ship_count];
    for (int i = 0; i < ship_count; i++) sizes[i] = ship_sizes[i];

    for (int i = 0; i < ship_count - 1; i++) {
        for (int j = i + 1; j < ship_count; j++) {
            if (sizes[i] < sizes[j]) {
                int tmp = sizes[i];
                sizes[i] = sizes[j];
                sizes[j] = tmp;
            }
        }
    }

    const int MAX_TOTAL_RETRIES = 100;
    const int MAX_SHIP_ATTEMPTS = 500;

    for (int retry = 0; retry < MAX_TOTAL_RETRIES; retry++) {
        board_clear(board);
        bool all_placed = true;

        for (int i = 0; i < ship_count; i++) {
            int size = sizes[i];
            bool placed = false;

            for (int attempt = 0; attempt < MAX_SHIP_ATTEMPTS; attempt++) {
                Direction dir = (rand() % 2 == 0) ? DIR_HORIZONTAL : DIR_VERTICAL;
                int max_x = (dir == DIR_HORIZONTAL) ? (board->width - size + 1) : board->width;
                int max_y = (dir == DIR_VERTICAL) ? (board->height - size + 1) : board->height;

                if (max_x <= 0 || max_y <= 0) continue;

                int x = rand() % max_x;
                int y = rand() % max_y;

                if (board_place_ship(board, x, y, size, dir, true)) {
                    placed = true;
                    break;
                }
            }

            if (!placed) {
                all_placed = false;
                break;
            }
        }

        if (all_placed) {
            return true;
        }
    }

    return false;
}
