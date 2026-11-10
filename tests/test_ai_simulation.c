#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "game.h"
#include "ai.h"

static void simulate_bot_game(AIDifficulty diff, const char *diff_name) {
    Board *b = board_create(10, 10);
    int fleet[10];
    int count = ai_get_default_fleet(10, 10, fleet, 10);
    assert(ai_auto_place_ships(b, fleet, count));

    int total_ships = b->ships_alive;
    assert(total_ships == 10);

    AIState ai;
    ai_init(&ai, diff);

    int shots = 0;
    int hits = 0;
    int sunk = 0;

    while (b->ships_alive > 0 && shots < 100) {
        int x = -1, y = -1;
        ai_make_move(&ai, b, &x, &y);
        shots++;

        Ship sunk_ship;
        ShotResult res = board_receive_shot(b, x, y, &sunk_ship);

        assert(res != SHOT_INVALID); // bot nie może strzelać poza planszę ani w to samo pole

        if (res == SHOT_HIT) {
            hits++;
        } else if (res == SHOT_SUNK) {
            hits++;
            sunk++;
            board_mark_sunk_perimeter(b, &sunk_ship);
        } else if (res == SHOT_WIN) {
            hits++;
            sunk++;
            break;
        }

        ai_notify_result(&ai, x, y, res, b);
    }

    printf("  [Poziom %s] Statki zatopione: %d/10 w %d strzalach (trafienia: %d)\n",
           diff_name, sunk, shots, hits);

    // Bot MUSI zatopić wszystkie statki i trafić co najmniej we wszystkie maszty (20)
    assert(b->ships_alive == 0);
    assert(sunk == 10);
    assert(hits == 20); // 1x4 + 2x3 + 3x2 + 4x1 = 20 masztów

    board_destroy(b);
}

int main(void) {
    printf("=== TEST SYMULACJI: OSTRZAL STATKOW PRZEZ BOTA ===\n\n");
    simulate_bot_game(AI_EASY, "LATWY");
    simulate_bot_game(AI_MEDIUM, "SREDNI (Hunt & Target)");
    simulate_bot_game(AI_HARD, "TRUDNY (Szachownica + Hunt & Target)");
    printf("\n>>> POTWIERDZONE: BOT BEZBLEDNIE TRAFIA I ZATAPIA WSZYSTKIE STATKI! <<<\n");
    return 0;
}
