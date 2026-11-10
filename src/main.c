#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "game.h"
#include "ui.h"
#include "ai.h"
#include "network.h"
#include "protocol.h"
#include "config.h"

#define MAX_FLEET_SHIPS 32

static void setup_player_ships(Board *player_board, const int *fleet, int ship_count) {
    bool manual = ui_prompt_placement_mode();

    if (!manual) {
        if (ai_auto_place_ships(player_board, fleet, ship_count)) {
            ui_clear_screen();
            ui_print_message("Statki zostaly rozstawione losowo!");
            ui_render_single_board(player_board, "TWOJA PLANSZA PO ROZSTAWIENIU", true);
            ui_pause();
            return;
        } else {
            ui_print_message("Blad losowego rozstawiania! Przelaczam na tryb reczny.");
        }
    }

    // Tryb ręczny
    for (int i = 0; i < ship_count; i++) {
        int size = fleet[i];
        bool placed = false;

        while (!placed) {
            ui_clear_screen();
            ui_print_banner("RECZNE ROZSTAWIANIE FLOTY");
            printf("Rozstawiasz statek %d z %d (dlugosc: %d masztow)\n", i + 1, ship_count, size);
            ui_render_single_board(player_board, "TWOJA PLANSZA", true);

            int x = 0, y = 0;
            Direction dir = DIR_HORIZONTAL;
            if (!ui_get_ship_placement(player_board, size, &x, &y, &dir)) {
                ui_print_message("Przerwano reczne rozstawianie. Reszta statkow zostanie wylosowana.");
                ai_auto_place_ships(player_board, &fleet[i], ship_count - i);
                ui_pause();
                return;
            }

            if (board_place_ship(player_board, x, y, size, dir, true)) {
                placed = true;
            } else {
                printf("\n%s[BLAD] Nie mozna postawic statku w tym miejscu!%s\n", "\033[31m", "\033[0m");
                printf("Upewnij sie, ze statek miesci sie na planszy i nie dotyka innych statkow.\n");
                ui_pause();
            }
        }
    }

    ui_clear_screen();
    ui_print_message("Wszystkie statki zostaly poprawnie rozstawione!");
    ui_render_single_board(player_board, "TWOJA FINALNA PLANSZA", true);
    ui_pause();
}

static AIDifficulty prompt_ai_difficulty(void) {
    ui_clear_screen();
    ui_print_banner("WYBOR POZIOMU TRUDNOSCI BOTA");
    printf("\n");
    printf("  [1] Latwy (Czysto losowe strzaly)\n");
    printf("  [2] Sredni (Hunt & Target - dobijanie wykrytych statkow)\n");
    printf("  [3] Trudny (Hunt & Target + optymalizacja szachownicy)\n\n");
    printf("Wybierz poziom (1-3, domyslnie 2): ");
    fflush(stdout);

    char line[64];
    if (fgets(line, sizeof(line), stdin)) {
        if (line[0] == '1') return AI_EASY;
        if (line[0] == '3') return AI_HARD;
    }
    return AI_MEDIUM;
}

static void play_singleplayer(int board_w, int board_h) {
    AIDifficulty diff = prompt_ai_difficulty();

    Board *player_board = board_create(board_w, board_h);
    Board *ai_board = board_create(board_w, board_h);
    Board *player_tracking = board_create(board_w, board_h);

    if (!player_board || !ai_board || !player_tracking) {
        ui_print_message("[BLAD] Nie udalo sie zaalokowac planszy!");
        if (player_board) board_destroy(player_board);
        if (ai_board) board_destroy(ai_board);
        if (player_tracking) board_destroy(player_tracking);
        ui_pause();
        return;
    }

    int fleet[MAX_FLEET_SHIPS];
    int ship_count = ai_get_default_fleet(board_w, board_h, fleet, MAX_FLEET_SHIPS);

    player_tracking->ship_count = ship_count;
    player_tracking->ships_alive = ship_count;

    setup_player_ships(player_board, fleet, ship_count);
    ai_auto_place_ships(ai_board, fleet, ship_count);

    AIState ai;
    ai_init(&ai, diff);

    char last_player_msg[128] = "Gra rozpoczeta! Oddaj pierwszy strzal.";
    char last_ai_msg[128] = "Czekam na Twoj ruch...";

    while (player_board->ships_alive > 0 && ai_board->ships_alive > 0) {
        ui_render_boards(player_board, player_tracking);
        printf("\n  %sOstatni Twoj ruch:%s %s\n", "\033[32m", "\033[0m", last_player_msg);
        printf("  %sOstatni ruch bota:%s %s\n\n", "\033[33m", "\033[0m", last_ai_msg);

        // --- TURA GRACZA ---
        int shot_x = -1, shot_y = -1;
        bool valid_shot = false;

        while (!valid_shot) {
            int shot_status = ui_get_shot_coords(board_w, board_h, &shot_x, &shot_y);
            if (shot_status == -1) {
                printf("\n%sPoddano gre. Powrot do menu glownego.%s\n", "\033[33m", "\033[0m");
                goto cleanup;
            }
            if (shot_status == 0) {
                printf("%s[BLAD] Wprowadz poprawne wspolrzedne (np. B4 lub 2 4) lub 'q' aby wyjsc!%s\n", 
                       "\033[31m", "\033[0m");
                continue;
            }

            if (player_tracking->grid[shot_y][shot_x] != CELL_EMPTY) {
                printf("%s[BLAD] To pole zostalo juz ostrzelane! Wybierz inne.%s\n", "\033[31m", "\033[0m");
                continue;
            }

            valid_shot = true;
        }

        Ship sunk_ship;
        ShotResult p_res = board_receive_shot(ai_board, shot_x, shot_y, &sunk_ship);

        if (p_res == SHOT_MISS) {
            player_tracking->grid[shot_y][shot_x] = CELL_MISS;
            snprintf(last_player_msg, sizeof(last_player_msg), "Strzal w %c%d - Pudlo!", 'A' + shot_x, shot_y + 1);
        } else if (p_res == SHOT_HIT) {
            player_tracking->grid[shot_y][shot_x] = CELL_HIT;
            snprintf(last_player_msg, sizeof(last_player_msg), "Strzal w %c%d - TRAFIONY!", 'A' + shot_x, shot_y + 1);
        } else if (p_res == SHOT_SUNK) {
            player_tracking->grid[shot_y][shot_x] = CELL_SUNK;
            for (int i = 0; i < sunk_ship.size; i++) {
                int sx = sunk_ship.x + (sunk_ship.dir == DIR_HORIZONTAL ? i : 0);
                int sy = sunk_ship.y + (sunk_ship.dir == DIR_VERTICAL ? i : 0);
                player_tracking->grid[sy][sx] = CELL_SUNK;
            }
            board_mark_sunk_perimeter(player_tracking, &sunk_ship);
            player_tracking->ships_alive--;
            snprintf(last_player_msg, sizeof(last_player_msg), "Strzal w %c%d - TRAFIONY I ZATOPIONY (%d maszty)!", 
                     'A' + shot_x, shot_y + 1, sunk_ship.size);
        } else if (p_res == SHOT_WIN) {
            player_tracking->grid[shot_y][shot_x] = CELL_SUNK;
            player_tracking->ships_alive = 0;
            snprintf(last_player_msg, sizeof(last_player_msg), "Strzal w %c%d - OSTATNI STATEK ZATOPIONY! WYGRANA!", 
                     'A' + shot_x, shot_y + 1);
            break;
        }

        // --- TURA BOTA ---
        int ai_x = -1, ai_y = -1;
        ai_make_move(&ai, player_board, &ai_x, &ai_y);

        Ship ai_sunk_ship;
        ShotResult ai_res = board_receive_shot(player_board, ai_x, ai_y, &ai_sunk_ship);
        ai_notify_result(&ai, ai_x, ai_y, ai_res, player_board);

        if (ai_res == SHOT_SUNK) {
            board_mark_sunk_perimeter(player_board, &ai_sunk_ship);
            snprintf(last_ai_msg, sizeof(last_ai_msg), "Bot strzelil w %c%d - ZATOPIL Twoj statek!", 
                     'A' + ai_x, ai_y + 1);
        } else if (ai_res == SHOT_HIT) {
            snprintf(last_ai_msg, sizeof(last_ai_msg), "Bot strzelil w %c%d - TRAFIL Twoj statek!", 
                     'A' + ai_x, ai_y + 1);
        } else if (ai_res == SHOT_WIN) {
            snprintf(last_ai_msg, sizeof(last_ai_msg), "Bot strzelil w %c%d - ZATOPIL Twoj ostatni statek!", 
                     'A' + ai_x, ai_y + 1);
            break;
        } else {
            snprintf(last_ai_msg, sizeof(last_ai_msg), "Bot strzelil w %c%d - Pudlo.", 
                     'A' + ai_x, ai_y + 1);
        }
    }

    ui_render_boards(player_board, player_tracking);
    printf("\n");
    if (ai_board->ships_alive == 0) {
        printf("%s%s============================================================%s\n", "\033[1m", "\033[32m", "\033[0m");
        printf("%s%s           ZWYCIESTWO! WROGIE OKRETY ZOSTALY ZNISZCZONE!     %s\n", "\033[1m", "\033[32m", "\033[0m");
        printf("%s%s============================================================%s\n", "\033[1m", "\033[32m", "\033[0m");
    } else {
        printf("%s%s============================================================%s\n", "\033[1m", "\033[31m", "\033[0m");
        printf("%s%s           PORAZKA! TWOJA FLOTA POSZLA NA DNO!              %s\n", "\033[1m", "\033[31m", "\033[0m");
        printf("%s%s============================================================%s\n", "\033[1m", "\033[31m", "\033[0m");
    }

cleanup:
    board_destroy(player_board);
    board_destroy(ai_board);
    board_destroy(player_tracking);
    if (!feof(stdin)) {
        ui_pause();
    }
}

static void run_multiplayer_game(socket_t peer_sock, Board *player_board, Board *player_tracking, bool is_host) {
    bool my_turn = is_host; // Host zaczyna pierwszy
    char last_my_msg[128] = "Oczekiwanie na rozpoczecie wymiany ognia...";
    char last_enemy_msg[128] = "Polaczenie nawiazane.";

    while (player_board->ships_alive > 0 && player_tracking->ships_alive > 0) {
        ui_render_boards(player_board, player_tracking);
        printf("\n  %sOstatni Twoj ruch:%s %s\n", "\033[32m", "\033[0m", last_my_msg);
        printf("  %sRuch przeciwnika:%s %s\n\n", "\033[33m", "\033[0m", last_enemy_msg);

        if (my_turn) {
            printf("%s>>> TWOJA TURA! <<<%s\n", "\033[1;32m", "\033[0m");
            int shot_x = -1, shot_y = -1;
            bool valid_shot = false;

            while (!valid_shot) {
                int shot_status = ui_get_shot_coords(player_board->width, player_board->height, &shot_x, &shot_y);
                if (shot_status == -1) {
                    // Gracz poddał partię
                    NetworkMessage quit_msg = { .type = MSG_QUIT };
                    net_send_message(peer_sock, &quit_msg);
                    printf("\n%sPoddales rozgrywke! Powrot do menu.%s\n", "\033[33m", "\033[0m");
                    return;
                }
                if (shot_status == 0) {
                    printf("%s[BLAD] Wprowadz poprawne wspolrzedne (np. B4 lub 2 4) lub 'q' aby sie poddac!%s\n", 
                           "\033[31m", "\033[0m");
                    continue;
                }

                if (player_tracking->grid[shot_y][shot_x] != CELL_EMPTY) {
                    printf("%s[BLAD] To pole zostalo juz ostrzelane! Wybierz inne.%s\n", "\033[31m", "\033[0m");
                    continue;
                }

                valid_shot = true;
            }

            // Wysłanie strzału do przeciwnika
            NetworkMessage shot_msg = { .type = MSG_SHOT, .x = shot_x, .y = shot_y };
            if (!net_send_message(peer_sock, &shot_msg)) {
                ui_print_message("[BLAD SIECI] Utracono polaczenie z drugim graczem!");
                return;
            }

            // Oczekiwanie na wynik strzału
            NetworkMessage res_msg;
            if (!net_recv_message(peer_sock, &res_msg)) {
                ui_print_message("[BLAD SIECI] Przeciwnik rozlaczyl sie!");
                return;
            }

            if (res_msg.type == MSG_QUIT) {
                printf("\n%s%sPrzeciwnik poddal gre lub sie rozlaczyl! WYGRALES VOUCHEREM!%s\n", 
                       "\033[1m", "\033[32m", "\033[0m");
                return;
            }

            if (res_msg.type == MSG_RESULT) {
                if (res_msg.result == SHOT_MISS) {
                    player_tracking->grid[shot_y][shot_x] = CELL_MISS;
                    snprintf(last_my_msg, sizeof(last_my_msg), "Strzal w %c%d - Pudlo!", 'A' + shot_x, shot_y + 1);
                } else if (res_msg.result == SHOT_HIT) {
                    player_tracking->grid[shot_y][shot_x] = CELL_HIT;
                    snprintf(last_my_msg, sizeof(last_my_msg), "Strzal w %c%d - TRAFIONY!", 'A' + shot_x, shot_y + 1);
                } else if (res_msg.result == SHOT_SUNK) {
                    player_tracking->grid[shot_y][shot_x] = CELL_SUNK;
                    for (int i = 0; i < res_msg.sunk_ship.size; i++) {
                        int sx = res_msg.sunk_ship.x + (res_msg.sunk_ship.dir == DIR_HORIZONTAL ? i : 0);
                        int sy = res_msg.sunk_ship.y + (res_msg.sunk_ship.dir == DIR_VERTICAL ? i : 0);
                        if (board_is_valid_coord(player_tracking, sx, sy)) {
                            player_tracking->grid[sy][sx] = CELL_SUNK;
                        }
                    }
                    board_mark_sunk_perimeter(player_tracking, &res_msg.sunk_ship);
                    player_tracking->ships_alive--;
                    snprintf(last_my_msg, sizeof(last_my_msg), "Strzal w %c%d - ZATOPIONY (%d masztow)! Brawo!", 
                             'A' + shot_x, shot_y + 1, res_msg.sunk_ship.size);
                } else if (res_msg.result == SHOT_WIN) {
                    player_tracking->grid[shot_y][shot_x] = CELL_SUNK;
                    player_tracking->ships_alive = 0;
                    snprintf(last_my_msg, sizeof(last_my_msg), "Strzal w %c%d - OSTATNI STATEK WROGA ZATOPIONY!", 
                             'A' + shot_x, shot_y + 1);
                    break; // Zwycięstwo!
                }
            }

            my_turn = false;
        } else {
            printf("%s>>> TURA PRZECIWNIKA (Oczekiwanie na strzal)... <<<%s\n", "\033[1;33m", "\033[0m");
            printf("(Wpisz 'q' w dowolnym momencie w swojej turze, aby przerwac)\n");

            NetworkMessage in_msg;
            if (!net_recv_message(peer_sock, &in_msg)) {
                ui_print_message("[BLAD SIECI] Przeciwnik rozlaczyl sie!");
                return;
            }

            if (in_msg.type == MSG_QUIT) {
                printf("\n%s%sPrzeciwnik poddal gre! ZWYCIESTWO!%s\n", "\033[1m", "\033[32m", "\033[0m");
                return;
            }

            if (in_msg.type == MSG_SHOT) {
                Ship sunk_ship;
                ShotResult res = board_receive_shot(player_board, in_msg.x, in_msg.y, &sunk_ship);

                NetworkMessage out_res = {
                    .type = MSG_RESULT,
                    .result = res,
                    .sunk_ship = sunk_ship
                };

                if (res == SHOT_SUNK) {
                    board_mark_sunk_perimeter(player_board, &sunk_ship);
                    snprintf(last_enemy_msg, sizeof(last_enemy_msg), "Przeciwnik strzelil w %c%d i ZATOPIL Twoj statek!", 
                             'A' + in_msg.x, in_msg.y + 1);
                } else if (res == SHOT_HIT) {
                    snprintf(last_enemy_msg, sizeof(last_enemy_msg), "Przeciwnik strzelil w %c%d i TRAFIL!", 
                             'A' + in_msg.x, in_msg.y + 1);
                } else if (res == SHOT_WIN) {
                    snprintf(last_enemy_msg, sizeof(last_enemy_msg), "Przeciwnik strzelil w %c%d - Twoja flota zostala zniszczona!", 
                             'A' + in_msg.x, in_msg.y + 1);
                    net_send_message(peer_sock, &out_res);
                    break; // Porażka
                } else {
                    snprintf(last_enemy_msg, sizeof(last_enemy_msg), "Przeciwnik strzelil w %c%d - Pudlo.", 
                             'A' + in_msg.x, in_msg.y + 1);
                }

                net_send_message(peer_sock, &out_res);
            }

            my_turn = true;
        }
    }

    ui_render_boards(player_board, player_tracking);
    printf("\n");
    if (player_tracking->ships_alive == 0) {
        printf("%s%s============================================================%s\n", "\033[1m", "\033[32m", "\033[0m");
        printf("%s%s     GRATULACJE! POKONALES DRUGIEGO GRACZA W SIECI!         %s\n", "\033[1m", "\033[32m", "\033[0m");
        printf("%s%s============================================================%s\n", "\033[1m", "\033[32m", "\033[0m");
    } else {
        printf("%s%s============================================================%s\n", "\033[1m", "\033[31m", "\033[0m");
        printf("%s%s     PORAZKA! DRUGI GRACZ ZNISZCZYL TWOJA FLOTE!            %s\n", "\033[1m", "\033[31m", "\033[0m");
        printf("%s%s============================================================%s\n", "\033[1m", "\033[31m", "\033[0m");
    }
}

static void play_multiplayer_host(int board_w, int board_h) {
    ui_clear_screen();
    ui_print_banner("ROZGRYWKA WIELOOSOBOWA - HOST (SERWER)");

    char local_ip[64];
    net_get_local_ip(local_ip, sizeof(local_ip));

    printf("\n  Twoj adres IP w sieci LAN: %s%s%s\n", "\033[1;32m", local_ip, "\033[0m");
    printf("  Port nasluchu: %s%d%s\n", "\033[1;33m", DEFAULT_PORT, "\033[0m");
    printf("  * Jesli grasz w 2 terminalach na tym samym PC, w drugim terminalu wpisz: %s127.0.0.1%s\n\n", 
           "\033[1;36m", "\033[0m");
    printf("Oczekiwanie na polaczenie od drugiego gracza...\n");

    socket_t server_sock = net_listen(DEFAULT_PORT);
    if (server_sock == INVALID_SOCKET_VALUE) {
        ui_print_message("[BLAD] Nie udalo sie otworzyc portu. Moze jest juz zajety?");
        ui_pause();
        return;
    }

    socket_t client_sock = net_accept(server_sock);
    net_close(server_sock); // Akceptujemy jednego gracza i zamykamy nasłuch

    if (client_sock == INVALID_SOCKET_VALUE) {
        ui_print_message("[BLAD] Nie udalo sie nawiazac polaczenia.");
        ui_pause();
        return;
    }

    ui_print_message("\nGracz 2 dolaczyl! Uzgadnianie parametrow rozgrywki...");

    // 1. Wysyłanie uzgodnienia planszy do klienta
    int fleet[MAX_FLEET_SHIPS];
    int ship_count = ai_get_default_fleet(board_w, board_h, fleet, MAX_FLEET_SHIPS);

    NetworkMessage hello = {
        .type = MSG_HELLO,
        .board_w = board_w,
        .board_h = board_h,
        .ship_count = ship_count
    };
    net_send_message(client_sock, &hello);

    NetworkMessage hello_ack;
    if (!net_recv_message(client_sock, &hello_ack) || hello_ack.type != MSG_HELLO_ACK) {
        ui_print_message("[BLAD SIECI] Klient nie odpowiedzial na handshake!");
        net_close(client_sock);
        ui_pause();
        return;
    }

    // 2. Alokacja plansz
    Board *player_board = board_create(board_w, board_h);
    Board *player_tracking = board_create(board_w, board_h);
    player_tracking->ship_count = ship_count;
    player_tracking->ships_alive = ship_count;

    // 3. Rozstawianie statków
    setup_player_ships(player_board, fleet, ship_count);

    ui_clear_screen();
    ui_print_message("Wysylam status gotowosci. Oczekiwanie az drugi gracz rozstawi statki...");
    NetworkMessage ready_msg = { .type = MSG_READY };
    net_send_message(client_sock, &ready_msg);

    NetworkMessage client_ready;
    if (!net_recv_message(client_sock, &client_ready) || client_ready.type != MSG_READY) {
        ui_print_message("[BLAD SIECI] Utracono kontakt z drugim graczem podczas rozstawiania.");
        board_destroy(player_board);
        board_destroy(player_tracking);
        net_close(client_sock);
        ui_pause();
        return;
    }

    // 4. Rozgrywka
    run_multiplayer_game(client_sock, player_board, player_tracking, true);

    board_destroy(player_board);
    board_destroy(player_tracking);
    net_close(client_sock);
    ui_pause();
}

static void play_multiplayer_join(void) {
    ui_clear_screen();
    ui_print_banner("ROZGRYWKA WIELOOSOBOWA - DOLACZ (KLIENT)");

    printf("\nPodaj adres IP Hosta (Enter dla '127.0.0.1' - 2 terminale na tym samym PC): ");
    fflush(stdout);

    char ip[64];
    if (!fgets(ip, sizeof(ip), stdin)) {
        return;
    }
    ip[strcspn(ip, "\r\n")] = '\0';
    if (strlen(ip) == 0) {
        snprintf(ip, sizeof(ip), "127.0.0.1");
    }

    printf("Laczenie z %s:%d...\n", ip, DEFAULT_PORT);
    socket_t sock = net_connect(ip, DEFAULT_PORT);
    if (sock == INVALID_SOCKET_VALUE) {
        ui_print_message("\n[BLAD] Nie udalo sie polaczyc z Hostem! Upewnij sie, ze serwer wystartowal.");
        ui_pause();
        return;
    }

    ui_print_message("Polaczono pomyslnie! Oczekiwanie na parametry planszy od Hosta...");

    // 1. Odbiór parametrów planszy od Hosta
    NetworkMessage hello;
    if (!net_recv_message(sock, &hello) || hello.type != MSG_HELLO) {
        ui_print_message("[BLAD SIECI] Nie otrzymano konfiguracji planszy od Hosta.");
        net_close(sock);
        ui_pause();
        return;
    }

    NetworkMessage ack = { .type = MSG_HELLO_ACK };
    net_send_message(sock, &ack);

    int board_w = hello.board_w;
    int board_h = hello.board_h;
    int ship_count = hello.ship_count;

    int fleet[MAX_FLEET_SHIPS];
    ai_get_default_fleet(board_w, board_h, fleet, MAX_FLEET_SHIPS);

    // 2. Alokacja plansz
    Board *player_board = board_create(board_w, board_h);
    Board *player_tracking = board_create(board_w, board_h);
    player_tracking->ship_count = ship_count;
    player_tracking->ships_alive = ship_count;

    // 3. Rozstawianie statków
    setup_player_ships(player_board, fleet, ship_count);

    ui_clear_screen();
    ui_print_message("Wysylam status gotowosci. Oczekiwanie na Hosta...");
    NetworkMessage ready_msg = { .type = MSG_READY };
    net_send_message(sock, &ready_msg);

    NetworkMessage host_ready;
    if (!net_recv_message(sock, &host_ready) || host_ready.type != MSG_READY) {
        ui_print_message("[BLAD SIECI] Utracono kontakt z Hostem podczas rozstawiania.");
        board_destroy(player_board);
        board_destroy(player_tracking);
        net_close(sock);
        ui_pause();
        return;
    }

    // 4. Rozgrywka
    run_multiplayer_game(sock, player_board, player_tracking, false);

    board_destroy(player_board);
    board_destroy(player_tracking);
    net_close(sock);
    ui_pause();
}

int main(void) {
    ui_init();
    net_init();

    int board_w = DEFAULT_BOARD_WIDTH;
    int board_h = DEFAULT_BOARD_HEIGHT;

    bool running = true;
    while (running) {
        GameMode mode = ui_show_main_menu();

        switch (mode) {
            case MODE_VS_AI:
                play_singleplayer(board_w, board_h);
                break;
            case MODE_LAN_HOST:
                play_multiplayer_host(board_w, board_h);
                break;
            case MODE_LAN_JOIN:
                play_multiplayer_join();
                break;
            case MODE_SETTINGS:
                ui_show_settings(&board_w, &board_h);
                break;
            case MODE_EXIT:
                running = false;
                break;
        }
    }

    net_cleanup();
    ui_clear_screen();
    printf("Dziekujemy za gre w Statki! Do zobaczenia.\n");
    return 0;
}
