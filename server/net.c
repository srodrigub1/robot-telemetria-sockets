#include "net.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>
#ifdef _WIN32
#include <io.h>
static bool g_wsa_started = false;
#else
#include <unistd.h>
#endif

int send_all(int sock,const void *buf,size_t len){
    const char *p = (const char*)buf;
    size_t off=0;
    while(off<len){
        ssize_t n=send(sock,p+off,len-off,0);
        if(n<=0) return -1;
        off+=n;
    }
    return 0;
}

static int ensure_wsa(void){
#ifdef _WIN32
    if(!g_wsa_started){
        WSADATA wsa;
        if(WSAStartup(MAKEWORD(2,2), &wsa)!=0){
            return -1;
        }
        g_wsa_started = true;
    }
#endif
    return 0;
}

int create_server_socket(int port){
    if(ensure_wsa()<0) return -1;
    int sock = (int)socket(AF_INET, SOCK_STREAM, 0);
    if(sock<0) return -1;

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr,0,sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);

    if(bind(sock,(struct sockaddr*)&addr,sizeof addr)<0){
        perror("bind");
        close_socket(sock);
        return -1;
    }
    if(listen(sock,16)<0){
        perror("listen");
        close_socket(sock);
        return -1;
    }
    return sock;
}

void close_socket(int sock){
    if(sock<0) return;
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

int send_response(int sock,int code,const char *reason,const char *body){
    char hdr[512];
    size_t blen = body? strlen(body):0;
    int n = snprintf(hdr,sizeof hdr,
        "RTTP/1.0 %d %s\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n",
        code, reason?reason:"OK", blen);
    if(send_all(sock,hdr,(size_t)n)<0) return -1;
    if(blen>0 && send_all(sock,body,blen)<0) return -1;
    return 0;
}

int recv_line(int sock,char *dst,size_t cap){
    size_t off=0; while(off+1<cap){
        char c; ssize_t n=recv(sock,&c,1,0); if(n<=0) return -1;
        dst[off++]=c; if(c=='\n') break;
    } dst[off]=0; return (int)off;
}
int recv_n(int sock,void *dst,size_t n){
    size_t off=0; char *p = (char*)dst;
    while(off<n){ ssize_t r=recv(sock,p+off,n-off,0); if(r<=0) return -1; off+=r; }
    return 0;
}
