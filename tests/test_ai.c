#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "game.h"
#include "ai.h"

void test_default_fleet_generation(void) {
    printf("[TEST] Generowanie floty w zaleznosci od rozmiaru planszy...\n");
    int fleet[16];
    
    // Plansza 8x8 -> 6 statków
    int count = ai_get_default_fleet(8, 8, fleet, 16);
    assert(count == 6);
    assert(fleet[0] == 3);

    // Plansza standardowa 10x10 -> 10 statków (1x4, 2x3, 3x2, 4x1)
    count = ai_get_default_fleet(10, 10, fleet, 16);
    assert(count == 10);
    assert(fleet[0] == 4);
    assert(fleet[1] == 3);
    assert(fleet[9] == 1);

    printf("  -> OK\n");
}

void test_auto_placement(void) {
    printf("[TEST] Automatyczne losowe rozstawianie statkow z zachowaniem odstepow...\n");
    Board *b = board_create(10, 10);

    int fleet[10];
    int count = ai_get_default_fleet(10, 10, fleet, 10);
    assert(count == 10);

    bool ok = ai_auto_place_ships(b, fleet, count);
    assert(ok == true);
    assert(b->ship_count == 10);
    assert(b->ships_alive == 10);

    // Weryfikacja: żaden statek nie może dotykać innego
    for (int y = 0; y < b->height; y++) {
        for (int x = 0; x < b->width; x++) {
            if (b->grid[y][x] == CELL_SHIP) {
                int my_id = b->ship_ids[y][x];
                assert(my_id >= 0);

                // Sprawdzamy sąsiadów
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = x + dx;
                        int ny = y + dy;
                        if (board_is_valid_coord(b, nx, ny)) {
                            if (b->grid[ny][nx] == CELL_SHIP) {
                                int neighbor_id = b->ship_ids[ny][nx];
                                // Jeśli sąsiad jest statkiem, musi być TYM SAMYM statkiem
                                assert(neighbor_id == my_id);
                            }
                        }
                    }
                }
            }
        }
    }

    board_destroy(b);
    printf("  -> OK\n");
}

void test_ai_hunting_mechanics(void) {
    printf("[TEST] Logika ostrzalu Hunt & Target...\n");
    Board *player_board = board_create(10, 10);
    Board *tracking_board = board_create(10, 10);

    // Stawiamy 3-masztowiec w (3, 3) poziomo: (3,3), (4,3), (5,3)
    board_place_ship(player_board, 3, 3, 3, DIR_HORIZONTAL, true);

    AIState ai;
    ai_init(&ai, AI_MEDIUM);
    assert(ai.targeting == false);

    // Symulacja trafienia pierwszego segmentu w (3, 3)
    ShotResult res = board_receive_shot(player_board, 3, 3, NULL);
    assert(res == SHOT_HIT);
    tracking_board->grid[3][3] = CELL_HIT;
    ai_notify_result(&ai, 3, 3, res, tracking_board);

    // AI powinno teraz być w trybie targeting i mieć cele w kolejce
    assert(ai.targeting == true);
    assert(ai.target_count > 0);

    // Sprawdzamy kolejny ruch bota - musi być jednym z sąsiadów (3,3)
    int shot_x = -1, shot_y = -1;
    ai_make_move(&ai, tracking_board, &shot_x, &shot_y);
    assert((shot_x == 4 && shot_y == 3) || 
           (shot_x == 2 && shot_y == 3) || 
           (shot_x == 3 && shot_y == 4) || 
           (shot_x == 3 && shot_y == 2));

    board_destroy(player_board);
    board_destroy(tracking_board);
    printf("  -> OK\n");
}

int main(void) {
    printf("=== URUCHOMIENIE TESTOW JEDNOSTKOWYCH: MODUL AI ===\n\n");
    test_default_fleet_generation();
    test_auto_placement();
    test_ai_hunting_mechanics();
    printf("\n>>> TESTY MODULU AI ZAKONCZONE SUKCESEM! <<<\n");
    return 0;
}
