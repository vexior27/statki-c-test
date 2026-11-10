#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>
#include "protocol.h"

#if defined(_WIN32) || defined(_WIN64)
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef SOCKET socket_t;
    #define INVALID_SOCKET_VALUE INVALID_SOCKET
    #define SOCKET_ERROR_CODE WSAGetLastError()
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    typedef int socket_t;
    #define INVALID_SOCKET_VALUE (-1)
    #define SOCKET_ERROR_CODE errno
#endif

// Inicjalizacja biblioteki sieciowej (np. WSAStartup na Windows)
bool net_init(void);

// Sprzątanie po bibliotece sieciowej (WSACleanup)
void net_cleanup(void);

// Utworzenie gniazda nasłuchującego dla serwera (Hosta)
socket_t net_listen(int port);

// Akceptacja połączenia od klienta
socket_t net_accept(socket_t server_sock);

// Połączenie z serwerem (Hostem) pod podanym adresem IP i portem
socket_t net_connect(const char *ip, int port);

// Wysłanie komunikatu sieciowego
bool net_send_message(socket_t sock, const NetworkMessage *msg);

// Odebranie komunikatu sieciowego (blokujące do odebrania pełnej linii)
bool net_recv_message(socket_t sock, NetworkMessage *out_msg);

// Zamknięcie gniazda
void net_close(socket_t sock);

// Pobranie lokalnego adresu IP (ułatwia Hostowi podanie IP drugiemu graczowi w sieci LAN)
void net_get_local_ip(char *out_ip, int max_len);

#endif // NETWORK_H
