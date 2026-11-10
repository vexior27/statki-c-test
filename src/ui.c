#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ui.h"
#include "config.h"

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#endif

// Definicje kodów kolorów ANSI
#define ANSI_RESET      "\033[0m"
#define ANSI_BOLD       "\033[1m"
#define ANSI_DIM        "\033[2m"
#define ANSI_BLUE       "\033[34m"
#define ANSI_CYAN       "\033[36m"
#define ANSI_GREEN      "\033[32m"
#define ANSI_YELLOW     "\033[33m"
#define ANSI_RED        "\033[31m"
#define ANSI_MAGENTA    "\033[35m"
#define ANSI_WHITE      "\033[37m"
#define ANSI_BG_RED     "\033[41m"
#define ANSI_BG_BLUE    "\033[44m"

void ui_init(void) {
#if defined(_WIN32) || defined(_WIN64)
    // Włączenie obsługi sekwencji ANSI w konsoli Windows 10/11
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif
}

void ui_clear_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

void ui_pause(void) {
    printf("\n%sNacisnij ENTER, aby kontynuowac...%s", ANSI_DIM, ANSI_RESET);
    fflush(stdout);
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

static bool read_input_line(char *buffer, int max_len) {
    if (!fgets(buffer, max_len, stdin)) {
        return false;
    }
    buffer[strcspn(buffer, "\r\n")] = '\0';
    return true;
}

void ui_print_banner(const char *title) {
    printf("%s%s============================================================%s\n", ANSI_BOLD, ANSI_CYAN, ANSI_RESET);
    printf("%s%s                  %s%s\n", ANSI_BOLD, ANSI_YELLOW, title, ANSI_RESET);
    printf("%s%s============================================================%s\n", ANSI_BOLD, ANSI_CYAN, ANSI_RESET);
}

void ui_print_message(const char *msg) {
    if (msg) {
        printf("%s%s%s\n", ANSI_BOLD, msg, ANSI_RESET);
    }
}

GameMode ui_show_main_menu(void) {
    while (1) {
        ui_clear_screen();
        ui_print_banner("B A T T L E S H I P   ( C )");
        printf("\n");
        printf("  %s[1]%s Gra z komputerem (AI)\n", ANSI_GREEN, ANSI_RESET);
        printf("  %s[2]%s Rozgrywka dla 2 graczy - HOSTUJ (Serwer LAN / Localhost)\n", ANSI_GREEN, ANSI_RESET);
        printf("  %s[3]%s Rozgrywka dla 2 graczy - DOLACZ (Klient LAN / Localhost)\n", ANSI_GREEN, ANSI_RESET);
        printf("  %s[4]%s Ustawienia rozmiaru planszy\n", ANSI_YELLOW, ANSI_RESET);
        printf("  %s[5]%s Wyjscie z gry\n", ANSI_RED, ANSI_RESET);
        printf("\n%sWybierz opcje (1-5): %s", ANSI_BOLD, ANSI_RESET);
        fflush(stdout);

        char line[64];
        if (!read_input_line(line, sizeof(line))) {
            return MODE_EXIT;
        }

        if (strcmp(line, "1") == 0) return MODE_VS_AI;
        if (strcmp(line, "2") == 0) return MODE_LAN_HOST;
        if (strcmp(line, "3") == 0) return MODE_LAN_JOIN;
        if (strcmp(line, "4") == 0) return MODE_SETTINGS;
        if (strcmp(line, "5") == 0) return MODE_EXIT;

        printf("%sNiepoprawny wybor! Sprobuj ponownie.%s\n", ANSI_RED, ANSI_RESET);
        ui_pause();
    }
}

void ui_show_settings(int *board_w, int *board_h) {
    ui_clear_screen();
    ui_print_banner("USTAWIENIA ROZMIARU PLANSZY");
    printf("\nAktualny rozmiar planszy: %s%dx%d%s\n", ANSI_BOLD, *board_w, *board_h, ANSI_RESET);
    printf("Minimalny rozmiar: %dx%d, Maksymalny rozmiar: %dx%d\n\n", 
           MIN_BOARD_SIZE, MIN_BOARD_SIZE, MAX_BOARD_SIZE, MAX_BOARD_SIZE);

    printf("Podaj nowa szerokosc (lub nacisnij Enter, aby zostawic %d): ", *board_w);
    fflush(stdout);
    char line[64];
    if (read_input_line(line, sizeof(line)) && strlen(line) > 0) {
        int w = atoi(line);
        if (w >= MIN_BOARD_SIZE && w <= MAX_BOARD_SIZE) {
            *board_w = w;
        } else {
            printf("%sNieprawidlowa wartosc! Pozostawiono: %d%s\n", ANSI_RED, *board_w, ANSI_RESET);
        }
    }

    printf("Podaj nowa wysokosc (lub nacisnij Enter, aby zostawic %d): ", *board_h);
    fflush(stdout);
    if (read_input_line(line, sizeof(line)) && strlen(line) > 0) {
        int h = atoi(line);
        if (h >= MIN_BOARD_SIZE && h <= MAX_BOARD_SIZE) {
            *board_h = h;
        } else {
            printf("%sNieprawidlowa wartosc! Pozostawiono: %d%s\n", ANSI_RED, *board_h, ANSI_RESET);
        }
    }

    printf("\n%sUstawiono rozmiar planszy na: %dx%d%s\n", ANSI_GREEN, *board_w, *board_h, ANSI_RESET);
    ui_pause();
}

static void print_cell(CellState state, bool reveal_ships) {
    switch (state) {
        case CELL_EMPTY:
            printf("%s~ %s", ANSI_CYAN, ANSI_RESET);
            break;
        case CELL_SHIP:
            if (reveal_ships) {
                printf("%s# %s", ANSI_GREEN, ANSI_RESET);
            } else {
                printf("%s~ %s", ANSI_CYAN, ANSI_RESET);
            }
            break;
        case CELL_MISS:
            printf("%s* %s", ANSI_WHITE, ANSI_RESET);
            break;
        case CELL_HIT:
            printf("%sX %s", ANSI_YELLOW, ANSI_RESET);
            break;
        case CELL_SUNK:
            printf("%s%sX %s", ANSI_BOLD, ANSI_RED, ANSI_RESET);
            break;
        default:
            printf("? ");
            break;
    }
}

void ui_render_single_board(const Board *board, const char *title, bool reveal_ships) {
    if (!board) return;

    printf("\n%s=== %s ===%s\n", ANSI_BOLD, title, ANSI_RESET);

    printf("    ");
    for (int x = 0; x < board->width; x++) {
        printf("%c ", 'A' + x);
    }
    printf("\n");

    for (int y = 0; y < board->height; y++) {
        printf("%2d  ", y + 1);
        for (int x = 0; x < board->width; x++) {
            print_cell(board->grid[y][x], reveal_ships);
        }
        printf("\n");
    }
    printf("\n");
}

void ui_render_boards(const Board *my_board, const Board *tracking_board) {
    if (!my_board) return;

    ui_clear_screen();
    ui_print_banner("STAN BITWY MORSKIEJ");

    int w = my_board->width;
    int h = my_board->height;

    // Nagłówki plansz
    printf("\n      %s[ TWOJA FLOTA ]%s", ANSI_BOLD, ANSI_RESET);
    for (int i = 0; i < (w * 2 - 8); i++) printf(" ");
    printf("         %s[ STRZALY WE WROGA ]%s\n\n", ANSI_BOLD, ANSI_RESET);

    // Oznaczenia kolumn dla obu plansz (A, B, C...)
    printf("    ");
    for (int x = 0; x < w; x++) printf("%c ", 'A' + x);
    printf("      |      ");
    printf("    ");
    if (tracking_board) {
        for (int x = 0; x < tracking_board->width; x++) printf("%c ", 'A' + x);
    }
    printf("\n");

    // Wiersze planszy
    for (int y = 0; y < h; y++) {
        // Lewa plansza
        printf("%2d  ", y + 1);
        for (int x = 0; x < w; x++) {
            print_cell(my_board->grid[y][x], true);
        }

        // Separator
        printf("      |      ");

        // Prawa plansza
        if (tracking_board && y < tracking_board->height) {
            printf("%2d  ", y + 1);
            for (int x = 0; x < tracking_board->width; x++) {
                print_cell(tracking_board->grid[y][x], false);
            }
        }
        printf("\n");
    }

    // Podsumowanie stanu
    printf("\n%sPozostalo Twoich statkow:%s %d", ANSI_BOLD, ANSI_RESET, my_board->ships_alive);
    if (tracking_board) {
        printf("         %sZatopione statki wroga:%s %d\n", ANSI_BOLD, ANSI_RESET, 
               tracking_board->ship_count - tracking_board->ships_alive);
    } else {
        printf("\n");
    }

    printf("%sLegenda:%s %s~%s Woda  %s#%s Twoj statek  %s*%s Pudlo  %sX%s Trafienie  %s%sX%s Zatopiony\n\n",
           ANSI_DIM, ANSI_RESET,
           ANSI_CYAN, ANSI_RESET,
           ANSI_GREEN, ANSI_RESET,
           ANSI_WHITE, ANSI_RESET,
           ANSI_YELLOW, ANSI_RESET,
           ANSI_BOLD, ANSI_RED, ANSI_RESET);
}

bool ui_prompt_placement_mode(void) {
    while (1) {
        printf("\n%sWybierz sposob rozstawienia statkow:%s\n", ANSI_BOLD, ANSI_RESET);
        printf("  %s[1]%s Reczne rozstawianie\n", ANSI_GREEN, ANSI_RESET);
        printf("  %s[2]%s Automatyczne / Losowe rozstawienie (Szybki start)\n", ANSI_CYAN, ANSI_RESET);
        printf("Wybór (1-2): ");
        fflush(stdout);

        char line[64];
        if (read_input_line(line, sizeof(line))) {
            if (strcmp(line, "1") == 0) return true;
            if (strcmp(line, "2") == 0) return false;
        }
        printf("%sNiepoprawny wybor! Wpisz 1 lub 2.%s\n", ANSI_RED, ANSI_RESET);
    }
}

bool ui_get_ship_placement(const Board *board, int size, int *out_x, int *out_y, Direction *out_dir) {
    if (!board || !out_x || !out_y || !out_dir) return false;

    char line[64];
    while (1) {
        printf("Podaj wspolrzedne dla statku o dlugosci %s%d%s (np. B3) lub 'q' aby przerwac: ", 
               ANSI_YELLOW, size, ANSI_RESET);
        fflush(stdout);

        if (!read_input_line(line, sizeof(line))) {
            return false;
        }

        char *p = line;
        while (*p && isspace((unsigned char)*p)) p++;
        if (strcmp(p, "q") == 0 || strcmp(p, "Q") == 0 || strcmp(p, "'q'") == 0 || strcmp(p, "'Q'") == 0) {
            return false;
        }

        int shot_status = ui_get_shot_coords(board->width, board->height, out_x, out_y);
        if (shot_status == -1) {
            return false;
        }
        if (shot_status != 1) {
            printf("%sNiepoprawny format wspolrzednych! Podaj litere kolumny i numer wiersza (np. A1, C5).%s\n", 
                   ANSI_RED, ANSI_RESET);
            continue;
        }

        if (size == 1) {
            *out_dir = DIR_HORIZONTAL;
            return true;
        }

        printf("Podaj orientacje (%sH%s - pozioma, %sV%s - pionowa): ", 
               ANSI_BOLD, ANSI_RESET, ANSI_BOLD, ANSI_RESET);
        fflush(stdout);

        char dir_line[64];
        if (!read_input_line(dir_line, sizeof(dir_line))) {
            return false;
        }

        if (dir_line[0] == 'h' || dir_line[0] == 'H') {
            *out_dir = DIR_HORIZONTAL;
            return true;
        } else if (dir_line[0] == 'v' || dir_line[0] == 'V') {
            *out_dir = DIR_VERTICAL;
            return true;
        } else {
            printf("%sNieznana orientacja! Wpisz H (poziomo) lub V (pionowo).%s\n", ANSI_RED, ANSI_RESET);
        }
    }
}

int ui_get_shot_coords(int max_w, int max_h, int *out_x, int *out_y) {
    if (!out_x || !out_y || max_w <= 0 || max_h <= 0) return 0;

    char line[64];
    printf("%sWprowadz cel strzalu (np. B4 lub 'q' aby wyjsc): %s", ANSI_BOLD, ANSI_RESET);
    fflush(stdout);

    if (!read_input_line(line, sizeof(line))) {
        return -1; // EOF traktujemy jako wyjście
    }

    // Pomijanie początkowych białych znaków
    char *p = line;
    while (*p && isspace((unsigned char)*p)) p++;

    if (*p == '\0') return 0;

    // Sprawdzenie czy gracz chce się poddać / wyjść ('q', 'Q', "'q'", "'Q'", "quit", "exit")
    if (strcmp(p, "q") == 0 || strcmp(p, "Q") == 0 ||
        strcmp(p, "'q'") == 0 || strcmp(p, "'Q'") == 0 ||
        strcmp(p, "\"q\"") == 0 || strcmp(p, "\"Q\"") == 0 ||
        strcmp(p, "quit") == 0 || strcmp(p, "exit") == 0) {
        return -1; // Sygnał poddania / wyjścia
    }

    int col = -1;
    int row = -1;

    // Format 1: Litera kolumny na początku (np. "B4", "b 10")
    if (isalpha((unsigned char)*p)) {
        col = toupper((unsigned char)*p) - 'A';
        p++;
        while (*p && isspace((unsigned char)*p)) p++;
        if (isdigit((unsigned char)*p)) {
            row = atoi(p) - 1;
        }
    } 
    // Format 2: Dwie liczby (np. "4 2", "4,2")
    else if (isdigit((unsigned char)*p)) {
        col = atoi(p) - 1;
        while (*p && (isdigit((unsigned char)*p) || isspace((unsigned char)*p) || *p == ',')) {
            if (*p == ' ' || *p == ',') {
                p++;
                while (*p && isspace((unsigned char)*p)) p++;
                if (isdigit((unsigned char)*p)) {
                    row = atoi(p) - 1;
                }
                break;
            }
            p++;
        }
    }

    if (col >= 0 && col < max_w && row >= 0 && row < max_h) {
        *out_x = col;
        *out_y = row;
        return 1; // Poprawne współrzędne
    }

    return 0; // Błąd formatu
}
