#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "game.h"

void test_board_creation(void) {
    printf("[TEST] Tworzenie i zwalnianie planszy...\n");
    Board *b = board_create(10, 10);
    assert(b != NULL);
    assert(b->width == 10);
    assert(b->height == 10);
    assert(b->ship_count == 0);
    assert(b->ships_alive == 0);
    assert(b->grid[0][0] == CELL_EMPTY);
    assert(b->ship_ids[0][0] == -1);

    board_destroy(b);
    printf("  -> OK\n");
}

void test_ship_placement(void) {
    printf("[TEST] Rozstawianie statkow i walidacja kolizji...\n");
    Board *b = board_create(10, 10);

    // Prawidłowe postawienie 4-masztowca w (0, 0) poziomo
    bool ok = board_place_ship(b, 0, 0, 4, DIR_HORIZONTAL, true);
    assert(ok == true);
    assert(b->ship_count == 1);
    assert(b->ships_alive == 1);
    assert(b->grid[0][0] == CELL_SHIP);
    assert(b->grid[0][1] == CELL_SHIP);
    assert(b->grid[0][2] == CELL_SHIP);
    assert(b->grid[0][3] == CELL_SHIP);
    assert(b->grid[0][4] == CELL_EMPTY);

    // Próba postawienia statku poza krawędź planszy
    ok = board_place_ship(b, 8, 0, 3, DIR_HORIZONTAL, true);
    assert(ok == false);

    // Próba postawienia statku dotykającego bezpośrednio (wiersz 1, kolumna 0) przy strict_spacing=true
    ok = board_place_ship(b, 0, 1, 2, DIR_HORIZONTAL, true);
    assert(ok == false);

    // Prawidłowe postawienie 2-masztowca z odstępem (wiersz 2, kolumna 0)
    ok = board_place_ship(b, 0, 2, 2, DIR_HORIZONTAL, true);
    assert(ok == true);
    assert(b->ship_count == 2);

    board_destroy(b);
    printf("  -> OK\n");
}

void test_shooting_mechanics(void) {
    printf("[TEST] Mechanika strzalow (pudlo, trafienie, zatopienie, wygrana)...\n");
    Board *b = board_create(10, 10);

    // Stawiamy 2 statki: 2-masztowiec w (0, 0) i 1-masztowiec w (5, 5)
    board_place_ship(b, 0, 0, 2, DIR_HORIZONTAL, true);
    board_place_ship(b, 5, 5, 1, DIR_HORIZONTAL, true);
    assert(b->ships_alive == 2);

    // 1. Pudło
    ShotResult res = board_receive_shot(b, 9, 9, NULL);
    assert(res == SHOT_MISS);
    assert(b->grid[9][9] == CELL_MISS);

    // 2. Ponowny strzał w to samo pole -> INVALID
    res = board_receive_shot(b, 9, 9, NULL);
    assert(res == SHOT_INVALID);

    // 3. Trafienie w pierwszy segment 2-masztowca
    res = board_receive_shot(b, 0, 0, NULL);
    assert(res == SHOT_HIT);
    assert(b->grid[0][0] == CELL_HIT);
    assert(b->ships_alive == 2);

    // 4. Zatopienie 2-masztowca
    Ship sunk_ship;
    res = board_receive_shot(b, 1, 0, &sunk_ship);
    assert(res == SHOT_SUNK);
    assert(sunk_ship.size == 2);
    assert(b->grid[0][0] == CELL_SUNK);
    assert(b->grid[0][1] == CELL_SUNK);
    assert(b->ships_alive == 1);

    // Oznaczenie obrysu zatopionego statku
    board_mark_sunk_perimeter(b, &sunk_ship);
    assert(b->grid[1][0] == CELL_MISS); // pole pod zatopionym statkiem powinno byc oznaczone jako woda
    assert(b->grid[1][1] == CELL_MISS);

    // 5. Trafienie i zatopienie ostatniego statku -> SHOT_WIN
    res = board_receive_shot(b, 5, 5, NULL);
    assert(res == SHOT_WIN);
    assert(b->ships_alive == 0);
    assert(board_all_sunk(b) == true);

    board_destroy(b);
    printf("  -> OK\n");
}

int main(void) {
    printf("=== URUCHOMIENIE TESTOW JEDNOSTKOWYCH: MODUL SILNIKA GRY ===\n\n");
    test_board_creation();
    test_ship_placement();
    test_shooting_mechanics();
    printf("\n>>> WSZYSTKIE TESTY ZAKONCZONE SUKCESEM! <<<\n");
    return 0;
}
