#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

// Domyślne wymiary planszy
#define DEFAULT_BOARD_WIDTH   10
#define DEFAULT_BOARD_HEIGHT  10
#define MIN_BOARD_SIZE        6
#define MAX_BOARD_SIZE        20

// Domyślny port do komunikacji sieciowej
#define DEFAULT_PORT          5555
#define MAX_IP_STR_LEN        64

// Konfiguracja statków
#define MAX_SHIPS             20
#define MAX_SHIP_LENGTH       10

// Struktura konfiguracji floty (liczba i rozmiary statków)
typedef struct {
    int count;                  // Łączna liczba statków
    int sizes[MAX_SHIPS];       // Rozmiary kolejnych statków (np. {4, 3, 3, 2, 2, 2, 1, 1, 1, 1})
} FleetConfig;

// Globalna konfiguracja rozgrywki
typedef struct {
    int board_width;
    int board_height;
    FleetConfig fleet;
    int net_port;
    char remote_ip[MAX_IP_STR_LEN];
} GameSettings;

// Pomocnicze funkcje do inicjalizacji domyślnych ustawień
void config_set_default(GameSettings *settings);

#endif // CONFIG_H
