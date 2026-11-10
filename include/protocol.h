#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdbool.h>
#include "game.h"

// Typy komunikatów w protokole sieciowym
typedef enum {
    MSG_UNKNOWN = 0,
    MSG_HELLO,          // Host -> Klient: uzgodnienie rozmiaru planszy i floty
    MSG_HELLO_ACK,      // Klient -> Host: potwierdzenie parametrów
    MSG_READY,          // Gracz zakończył rozstawianie statków i czeka na start
    MSG_SHOT,           // Strzał we współrzędne (x, y)
    MSG_RESULT,         // Odpowiedź na strzał: MISS, HIT, SUNK, WIN (+ dane zatopionego statku)
    MSG_QUIT            // Gracz poddał się lub rozłączył
} MessageType;

typedef struct {
    MessageType type;
    int board_w;
    int board_h;
    int ship_count;
    int x;
    int y;
    ShotResult result;
    Ship sunk_ship;
} NetworkMessage;

// Serializacja wiadomości do czytelnego formatu tekstowego zakończonego \n
bool protocol_serialize(const NetworkMessage *msg, char *out_buf, int max_len);

// Deserializacja linii tekstu do struktury komunikatu
bool protocol_deserialize(const char *in_buf, NetworkMessage *out_msg);

#endif // PROTOCOL_H
