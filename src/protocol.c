#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "protocol.h"

bool protocol_serialize(const NetworkMessage *msg, char *out_buf, int max_len) {
    if (!msg || !out_buf || max_len <= 0) return false;

    switch (msg->type) {
        case MSG_HELLO:
            snprintf(out_buf, max_len, "HELLO %d %d %d\n", msg->board_w, msg->board_h, msg->ship_count);
            return true;

        case MSG_HELLO_ACK:
            snprintf(out_buf, max_len, "HELLO_ACK\n");
            return true;

        case MSG_READY:
            snprintf(out_buf, max_len, "READY\n");
            return true;

        case MSG_SHOT:
            snprintf(out_buf, max_len, "SHOT %d %d\n", msg->x, msg->y);
            return true;

        case MSG_RESULT:
            snprintf(out_buf, max_len, "RESULT %d %d %d %d %d\n",
                     (int)msg->result,
                     msg->sunk_ship.size,
                     msg->sunk_ship.x,
                     msg->sunk_ship.y,
                     (int)msg->sunk_ship.dir);
            return true;

        case MSG_QUIT:
            snprintf(out_buf, max_len, "QUIT\n");
            return true;

        default:
            return false;
    }
}

bool protocol_deserialize(const char *in_buf, NetworkMessage *out_msg) {
    if (!in_buf || !out_msg) return false;

    memset(out_msg, 0, sizeof(NetworkMessage));
    out_msg->type = MSG_UNKNOWN;

    char cmd[32];
    if (sscanf(in_buf, "%31s", cmd) != 1) {
        return false;
    }

    if (strcmp(cmd, "HELLO") == 0) {
        if (sscanf(in_buf, "HELLO %d %d %d", &out_msg->board_w, &out_msg->board_h, &out_msg->ship_count) == 3) {
            out_msg->type = MSG_HELLO;
            return true;
        }
    } else if (strcmp(cmd, "HELLO_ACK") == 0) {
        out_msg->type = MSG_HELLO_ACK;
        return true;
    } else if (strcmp(cmd, "READY") == 0) {
        out_msg->type = MSG_READY;
        return true;
    } else if (strcmp(cmd, "SHOT") == 0) {
        if (sscanf(in_buf, "SHOT %d %d", &out_msg->x, &out_msg->y) == 2) {
            out_msg->type = MSG_SHOT;
            return true;
        }
    } else if (strcmp(cmd, "RESULT") == 0) {
        int res_int = 0, size = 0, x = 0, y = 0, dir_int = 0;
        if (sscanf(in_buf, "RESULT %d %d %d %d %d", &res_int, &size, &x, &y, &dir_int) >= 1) {
            out_msg->type = MSG_RESULT;
            out_msg->result = (ShotResult)res_int;
            out_msg->sunk_ship.size = size;
            out_msg->sunk_ship.x = x;
            out_msg->sunk_ship.y = y;
            out_msg->sunk_ship.dir = (Direction)dir_int;
            out_msg->sunk_ship.is_sunk = true;
            return true;
        }
    } else if (strcmp(cmd, "QUIT") == 0) {
        out_msg->type = MSG_QUIT;
        return true;
    }

    return false;
}
