#ifndef NET_H
#define NET_H

#ifdef _WIN32
  // --- Windows / MinGW ---
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef int socklen_t;  // en Windows no siempre está definido
#else
  // --- Linux / Unix ---
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

int create_server_socket(int port);
int send_all(int sock, const void *buf, size_t len);
int recv_line(int sock, char *buf, size_t maxlen);
int recv_n(int sock, void *buf, size_t n);
void close_socket(int sock);

#endif
