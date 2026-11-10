#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"

void config_set_default(GameSettings *settings) {
    if (!settings) return;
    settings->board_width = DEFAULT_BOARD_WIDTH;
    settings->board_height = DEFAULT_BOARD_HEIGHT;
    settings->net_port = DEFAULT_PORT;
    strncpy(settings->remote_ip, "127.0.0.1", sizeof(settings->remote_ip) - 1);
    
    // Klasyczna flota: 1x4, 2x3, 3x2, 4x1 (10 statków)
    settings->fleet.count = 10;
    int defaults[10] = {4, 3, 3, 2, 2, 2, 1, 1, 1, 1};
    for (int i = 0; i < 10; ++i) {
        settings->fleet.sizes[i] = defaults[i];
    }
}

Board* board_create(int width, int height) {
    if (width <= 0 || height <= 0) return NULL;

    Board *b = (Board*)malloc(sizeof(Board));
    if (!b) return NULL;

    b->width = width;
    b->height = height;
    b->ship_count = 0;
    b->ship_capacity = MAX_SHIPS;
    b->ships_alive = 0;

    b->grid = (CellState**)malloc(height * sizeof(CellState*));
    b->ship_ids = (int**)malloc(height * sizeof(int*));
    for (int y = 0; y < height; ++y) {
        b->grid[y] = (CellState*)calloc(width, sizeof(CellState));
        b->ship_ids[y] = (int*)malloc(width * sizeof(int));
        for (int x = 0; x < width; ++x) {
            b->ship_ids[y][x] = -1;
        }
    }

    b->ships = (Ship*)malloc(b->ship_capacity * sizeof(Ship));
    return b;
}

void board_destroy(Board *board) {
    if (!board) return;
    if (board->grid) {
        for (int y = 0; y < board->height; ++y) {
            free(board->grid[y]);
        }
        free(board->grid);
    }
    if (board->ship_ids) {
        for (int y = 0; y < board->height; ++y) {
            free(board->ship_ids[y]);
        }
        free(board->ship_ids);
    }
    if (board->ships) {
        free(board->ships);
    }
    free(board);
}

void board_clear(Board *board) {
    if (!board) return;
    for (int y = 0; y < board->height; ++y) {
        for (int x = 0; x < board->width; ++x) {
            board->grid[y][x] = CELL_EMPTY;
            board->ship_ids[y][x] = -1;
        }
    }
    board->ship_count = 0;
    board->ships_alive = 0;
}

bool board_is_valid_coord(const Board *board, int x, int y) {
    if (!board) return false;
    return (x >= 0 && x < board->width && y >= 0 && y < board->height);
}

bool board_can_place_ship(const Board *board, int x, int y, int size, Direction dir, bool strict_spacing) {
    if (!board || size <= 0) return false;

    // Sprawdzenie czy statek mieści się w granicach planszy
    if (dir == DIR_HORIZONTAL) {
        if (x < 0 || y < 0 || x + size > board->width || y >= board->height) return false;
    } else {
        if (x < 0 || y < 0 || x >= board->width || y + size > board->height) return false;
    }

    int margin = strict_spacing ? 1 : 0;
    int start_x = (x - margin >= 0) ? x - margin : 0;
    int start_y = (y - margin >= 0) ? y - margin : 0;
    int end_x = (dir == DIR_HORIZONTAL) ? (x + size - 1 + margin) : (x + margin);
    int end_y = (dir == DIR_VERTICAL)   ? (y + size - 1 + margin) : (y + margin);

    if (end_x >= board->width)  end_x = board->width - 1;
    if (end_y >= board->height) end_y = board->height - 1;

    for (int cy = start_y; cy <= end_y; ++cy) {
        for (int cx = start_x; cx <= end_x; ++cx) {
            if (board->grid[cy][cx] != CELL_EMPTY) {
                return false;
            }
        }
    }

    return true;
}

bool board_place_ship(Board *board, int x, int y, int size, Direction dir, bool strict_spacing) {
    if (!board_can_place_ship(board, x, y, size, dir, strict_spacing)) {
        return false;
    }

    if (board->ship_count >= board->ship_capacity) {
        return false;
    }

    int ship_idx = board->ship_count;
    Ship *s = &board->ships[ship_idx];
    s->id = ship_idx + 1;
    s->x = x;
    s->y = y;
    s->size = size;
    s->dir = dir;
    s->hits_left = size;
    s->is_sunk = false;

    for (int i = 0; i < size; ++i) {
        int cx = (dir == DIR_HORIZONTAL) ? (x + i) : x;
        int cy = (dir == DIR_VERTICAL) ? (y + i) : y;
        board->grid[cy][cx] = CELL_SHIP;
        board->ship_ids[cy][cx] = ship_idx;
    }

    board->ship_count++;
    board->ships_alive++;
    return true;
}

ShotResult board_receive_shot(Board *board, int x, int y, Ship *out_sunk_ship) {
    if (out_sunk_ship) {
        memset(out_sunk_ship, 0, sizeof(Ship));
    }
    if (!board_is_valid_coord(board, x, y)) {
        return SHOT_INVALID;
    }

    CellState current = board->grid[y][x];
    if (current == CELL_MISS || current == CELL_HIT || current == CELL_SUNK) {
        return SHOT_INVALID;
    }

    if (current == CELL_EMPTY) {
        board->grid[y][x] = CELL_MISS;
        return SHOT_MISS;
    }

    if (current == CELL_SHIP) {
        board->grid[y][x] = CELL_HIT;
        int ship_idx = board->ship_ids[y][x];
        if (ship_idx >= 0 && ship_idx < board->ship_count) {
            Ship *s = &board->ships[ship_idx];
            s->hits_left--;
            if (s->hits_left == 0) {
                s->is_sunk = true;
                board->ships_alive--;
                if (out_sunk_ship) {
                    *out_sunk_ship = *s;
                }

                // Oznaczenie segmentów na planszy jako zatopione
                for (int i = 0; i < s->size; ++i) {
                    int sx = (s->dir == DIR_HORIZONTAL) ? (s->x + i) : s->x;
                    int sy = (s->dir == DIR_VERTICAL) ? (s->y + i) : s->y;
                    board->grid[sy][sx] = CELL_SUNK;
                }

                if (board->ships_alive == 0) {
                    return SHOT_WIN;
                }
                return SHOT_SUNK;
            }
            return SHOT_HIT;
        }
    }

    return SHOT_INVALID;
}

void board_mark_sunk_perimeter(Board *board, const Ship *ship) {
    if (!board || !ship) return;
    int start_x = (ship->x > 0) ? ship->x - 1 : 0;
    int start_y = (ship->y > 0) ? ship->y - 1 : 0;
    int end_x = (ship->dir == DIR_HORIZONTAL) ? (ship->x + ship->size) : (ship->x + 1);
    int end_y = (ship->dir == DIR_VERTICAL)   ? (ship->y + ship->size) : (ship->y + 1);

    if (end_x >= board->width)  end_x = board->width - 1;
    if (end_y >= board->height) end_y = board->height - 1;

    for (int y = start_y; y <= end_y; ++y) {
        for (int x = start_x; x <= end_x; ++x) {
            if (board->grid[y][x] == CELL_EMPTY) {
                board->grid[y][x] = CELL_MISS;
            }
        }
    }
}

bool board_all_sunk(const Board *board) {
    return (board && board->ships_alive == 0 && board->ship_count > 0);
}

const char* shot_result_to_string(ShotResult res) {
    switch (res) {
        case SHOT_MISS:    return "Pudlo";
        case SHOT_HIT:     return "Trafiony!";
        case SHOT_SUNK:    return "Trafiony i zatopiony!";
        case SHOT_WIN:     return "Zwyciestwo! Zatopiono cala flote wroga!";
        case SHOT_INVALID:
        default:           return "Niepoprawny strzal";
    }
}
