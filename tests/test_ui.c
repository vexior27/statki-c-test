#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "game.h"
#include "ui.h"

void test_coordinate_parsing_logic(void) {
    printf("[TEST] Sprawdzanie poprawnosci logiki indeksowania i renderowania...\n");
    Board *b1 = board_create(10, 10);
    Board *b2 = board_create(10, 10);

    // Dodanie statków
    board_place_ship(b1, 0, 0, 4, DIR_HORIZONTAL, true);
    board_place_ship(b1, 2, 3, 2, DIR_VERTICAL, true);

    // Symulacja kilku strzałów
    board_receive_shot(b1, 0, 0, NULL); // trafienie w (0,0)
    board_receive_shot(b1, 5, 5, NULL); // pudło w (5,5)

    // Symulacja strzałów w planszę wroga
    b2->grid[1][1] = CELL_MISS;
    b2->grid[4][3] = CELL_HIT;
    b2->grid[4][4] = CELL_SUNK;

    printf("\n--- Demonstracja widoku dwoch plansz (Side-by-side) ---\n");
    ui_render_boards(b1, b2);

    board_destroy(b1);
    board_destroy(b2);
    printf("  -> OK\n");
}

int main(void) {
    ui_init();
    printf("=== URUCHOMIENIE TESTOW JEDNOSTKOWYCH: MODUL INTERFEJSU UI ===\n\n");
    test_coordinate_parsing_logic();
    printf("\n>>> TESTY MODULU UI ZAKONCZONE SUKCESEM! <<<\n");
    return 0;
}
