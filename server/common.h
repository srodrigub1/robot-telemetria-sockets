#ifndef COMMON_H
#define COMMON_H

//  Includes para sockets 
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef int socklen_t;  
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// ==== Definiciones del proyecto ====
#define MAX_CLIENTS 10
#define BUF_SZ 512
#define TOKEN_SZ 64

// Variables de telemetría (máscaras)
#define VAR_TEMP  (1u<<0)
#define VAR_HUM   (1u<<1)
#define VAR_PRESS (1u<<2)
#define VAR_CO2   (1u<<3)

typedef struct {
    int sock;
    struct sockaddr_in addr;   
    bool in_use;
    bool is_admin;
    unsigned subs_mask;
  char user[64];
    char token[TOKEN_SZ];
} client_t;

#endif
