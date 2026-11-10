#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network.h"

static bool s_net_initialized = false;

bool net_init(void) {
    if (s_net_initialized) return true;

#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return false;
    }
#endif

    s_net_initialized = true;
    return true;
}

void net_cleanup(void) {
    if (!s_net_initialized) return;

#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif

    s_net_initialized = false;
}

socket_t net_listen(int port) {
    if (!net_init()) return INVALID_SOCKET_VALUE;

    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET_VALUE) {
        return INVALID_SOCKET_VALUE;
    }

    // Ustawienie opcji SO_REUSEADDR dla szybkiego restartu serwera
    int opt = 1;
#if defined(_WIN32) || defined(_WIN64)
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((unsigned short)port);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        net_close(sock);
        return INVALID_SOCKET_VALUE;
    }

    if (listen(sock, 1) != 0) {
        net_close(sock);
        return INVALID_SOCKET_VALUE;
    }

    return sock;
}

socket_t net_accept(socket_t server_sock) {
    if (server_sock == INVALID_SOCKET_VALUE) return INVALID_SOCKET_VALUE;

    struct sockaddr_in client_addr;
#if defined(_WIN32) || defined(_WIN64)
    int addr_len = sizeof(client_addr);
#else
    socklen_t addr_len = sizeof(client_addr);
#endif

    socket_t client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
    return client_sock;
}

socket_t net_connect(const char *ip, int port) {
    if (!net_init()) return INVALID_SOCKET_VALUE;

    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET_VALUE) {
        return INVALID_SOCKET_VALUE;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);

#if defined(_WIN32) || defined(_WIN64)
    addr.sin_addr.s_addr = inet_addr(ip);
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        struct hostent *he = gethostbyname(ip);
        if (he && he->h_addr_list[0]) {
            memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
        } else {
            net_close(sock);
            return INVALID_SOCKET_VALUE;
        }
    }
#else
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        struct hostent *he = gethostbyname(ip);
        if (he && he->h_addr_list[0]) {
            memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
        } else {
            net_close(sock);
            return INVALID_SOCKET_VALUE;
        }
    }
#endif

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        net_close(sock);
        return INVALID_SOCKET_VALUE;
    }

    return sock;
}

bool net_send_message(socket_t sock, const NetworkMessage *msg) {
    if (sock == INVALID_SOCKET_VALUE || !msg) return false;

    char buffer[256];
    if (!protocol_serialize(msg, buffer, sizeof(buffer))) {
        return false;
    }

    int len = (int)strlen(buffer);
    int total_sent = 0;

    while (total_sent < len) {
        int sent = send(sock, buffer + total_sent, len - total_sent, 0);
        if (sent <= 0) {
            return false;
        }
        total_sent += sent;
    }

    return true;
}

bool net_recv_message(socket_t sock, NetworkMessage *out_msg) {
    if (sock == INVALID_SOCKET_VALUE || !out_msg) return false;

    char buffer[256];
    int buf_idx = 0;

    // Odczyt po jednym bajcie aż do znaku nowej linii \n
    while (buf_idx < (int)sizeof(buffer) - 1) {
        char ch;
        int bytes = recv(sock, &ch, 1, 0);
        if (bytes <= 0) {
            return false; // Rozłączenie lub błąd
        }

        if (ch == '\n') {
            buffer[buf_idx] = '\0';
            break;
        }

        if (ch != '\r') {
            buffer[buf_idx++] = ch;
        }
    }
    buffer[buf_idx] = '\0';

    return protocol_deserialize(buffer, out_msg);
}

void net_close(socket_t sock) {
    if (sock == INVALID_SOCKET_VALUE) return;

#if defined(_WIN32) || defined(_WIN64)
    closesocket(sock);
#else
    close(sock);
#endif
}

void net_get_local_ip(char *out_ip, int max_len) {
    if (!out_ip || max_len <= 0) return;
    snprintf(out_ip, max_len, "127.0.0.1"); // domyślny fallback

    if (!net_init()) return;

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        struct hostent *he = gethostbyname(hostname);
        if (he != NULL) {
            for (int i = 0; he->h_addr_list[i] != NULL; i++) {
                char *ip = inet_ntoa(*(struct in_addr*)he->h_addr_list[i]);
                if (ip && strncmp(ip, "127.", 4) != 0) {
                    snprintf(out_ip, max_len, "%s", ip);
                    return;
                }
            }
        }
    }
}
