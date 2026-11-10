#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include "config.h"

// Stan pojedynczego pola na planszy
typedef enum {
    CELL_EMPTY = 0,     // Puste pole (morze)
    CELL_SHIP,          // Segment statku (ukryty)
    CELL_MISS,          // Pudło (strzał w wodę)
    CELL_HIT,           // Trafiony segment statku
    CELL_SUNK           // Cały statek zatopiony
} CellState;

// Orientacja statku
typedef enum {
    DIR_HORIZONTAL = 0,
    DIR_VERTICAL
} Direction;

// Wynik oddanego strzału
typedef enum {
    SHOT_INVALID = 0,   // Strzał poza planszę lub w powtórzone pole
    SHOT_MISS,          // Pudło
    SHOT_HIT,           // Trafienie segmentu
    SHOT_SUNK,          // Trafienie i zatopienie całego statku
    SHOT_WIN            // Zatopienie ostatniego statku (koniec gry)
} ShotResult;

// Pojedynczy statek
typedef struct {
    int id;             // Unikalny identyfikator statku
    int x;              // Współrzędna X dziobu (0 .. width-1)
    int y;              // Współrzędna Y dziobu (0 .. height-1)
    int size;           // Długość (liczba masztów)
    Direction dir;      // Orientacja (pozioma / pionowa)
    int hits_left;      // Liczba nieuszkodzonych segmentów
    bool is_sunk;       // Flaga zatopienia
} Ship;

// Dynamiczna plansza gry
typedef struct {
    int width;          // Szerokość planszy (liczba kolumn)
    int height;         // Wysokość planszy (liczba wierszy)
    CellState **grid;   // Dynamiczna siatka [y][x] z widocznym stanem pól
    int **ship_ids;     // Dynamiczna siatka [y][x] z indeksami statków (-1 = brak)
    Ship *ships;        // Dynamiczna tablica statków
    int ship_count;     // Aktualna liczba postawionych statków
    int ship_capacity;  // Zaalokowana pojemność tablicy statków
    int ships_alive;    // Liczba statków jeszcze pływających
} Board;

/* Tworzenie i zwalnianie planszy */
Board* board_create(int width, int height);
void board_destroy(Board *board);
void board_clear(Board *board);

/* Walidacja współrzędnych i rozstawianie */
bool board_is_valid_coord(const Board *board, int x, int y);
bool board_can_place_ship(const Board *board, int x, int y, int size, Direction dir, bool strict_spacing);
bool board_place_ship(Board *board, int x, int y, int size, Direction dir, bool strict_spacing);

/* Mechanika strzałów */
ShotResult board_receive_shot(Board *board, int x, int y, Ship *out_sunk_ship);
void board_mark_sunk_perimeter(Board *board, const Ship *ship);

/* Pomocnicze */
bool board_all_sunk(const Board *board);
const char* shot_result_to_string(ShotResult res);

#endif // GAME_H
