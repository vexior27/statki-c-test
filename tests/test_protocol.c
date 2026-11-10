#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "protocol.h"

void test_protocol_serialization(void) {
    printf("[TEST] Serializacja i deserializacja protokolu sieciowego...\n");
    char buf[256];
    NetworkMessage msg, out;

    // 1. HELLO
    msg = (NetworkMessage){ .type = MSG_HELLO, .board_w = 12, .board_h = 12, .ship_count = 8 };
    assert(protocol_serialize(&msg, buf, sizeof(buf)));
    assert(protocol_deserialize(buf, &out));
    assert(out.type == MSG_HELLO);
    assert(out.board_w == 12);
    assert(out.board_h == 12);
    assert(out.ship_count == 8);

    // 2. SHOT
    msg = (NetworkMessage){ .type = MSG_SHOT, .x = 5, .y = 3 };
    assert(protocol_serialize(&msg, buf, sizeof(buf)));
    assert(protocol_deserialize(buf, &out));
    assert(out.type == MSG_SHOT);
    assert(out.x == 5);
    assert(out.y == 3);

    // 3. RESULT (z zatopionym statkiem)
    msg = (NetworkMessage){ 
        .type = MSG_RESULT, 
        .result = SHOT_SUNK, 
        .sunk_ship = (Ship){ .size = 3, .x = 2, .y = 4, .dir = DIR_VERTICAL } 
    };
    assert(protocol_serialize(&msg, buf, sizeof(buf)));
    assert(protocol_deserialize(buf, &out));
    assert(out.type == MSG_RESULT);
    assert(out.result == SHOT_SUNK);
    assert(out.sunk_ship.size == 3);
    assert(out.sunk_ship.x == 2);
    assert(out.sunk_ship.y == 4);
    assert(out.sunk_ship.dir == DIR_VERTICAL);

    // 4. QUIT
    msg = (NetworkMessage){ .type = MSG_QUIT };
    assert(protocol_serialize(&msg, buf, sizeof(buf)));
    assert(protocol_deserialize(buf, &out));
    assert(out.type == MSG_QUIT);

    printf("  -> OK\n");
}

int main(void) {
    printf("=== URUCHOMIENIE TESTOW JEDNOSTKOWYCH: MODUL PROTOKOLU ===\n\n");
    test_protocol_serialization();
    printf("\n>>> TESTY PROTOKOLU ZAKONCZONE SUKCESEM! <<<\n");
    return 0;
}
