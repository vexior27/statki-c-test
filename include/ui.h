#ifndef UI_H
#define UI_H

#include <stdbool.h>
#include "game.h"

// Tryby gry wybierane w menu
typedef enum {
    MODE_VS_AI = 1,
    MODE_LAN_HOST,
    MODE_LAN_JOIN,
    MODE_SETTINGS,
    MODE_EXIT
} GameMode;

// Inicjalizacja konsoli (włączenie kolorów ANSI w Windows 10/11)
void ui_init(void);

// Czyszczenie ekranu terminala
void ui_clear_screen(void);

// Pauza oczekująca na wciśnięcie klawisza Enter
void ui_pause(void);

// Wyświetlenie menu głównego i pobranie poprawnego wyboru
GameMode ui_show_main_menu(void);

// Formularz konfiguracji rozmiaru planszy
void ui_show_settings(int *board_w, int *board_h);

// Rysowanie pojedynczej planszy (np. podczas ręcznego rozstawiania)
void ui_render_single_board(const Board *board, const char *title, bool reveal_ships);

// Rysowanie planszy własnej i planszy celowania obok siebie (Side-by-side)
void ui_render_boards(const Board *my_board, const Board *tracking_board);

// Pobranie decyzji o sposobie rozstawiania statków (true = ręczne, false = automatyczne)
bool ui_prompt_placement_mode(void);

// Pobranie od gracza współrzędnych i kierunku do postawienia statku
bool ui_get_ship_placement(const Board *board, int size, int *out_x, int *out_y, Direction *out_dir);

// Pobranie od gracza współrzędnych strzału.
// Zwraca: 1 = sukces (poprawne współrzędne), 0 = błąd formatu, -1 = poddanie gry (wpisano 'q'/'Q')
int ui_get_shot_coords(int max_w, int max_h, int *out_x, int *out_y);

// Wyświetlanie sformatowanych komunikatów o statusie gry
void ui_print_message(const char *msg);
void ui_print_banner(const char *title);

#endif // UI_H
