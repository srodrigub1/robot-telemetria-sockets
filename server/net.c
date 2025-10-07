#include "net.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>

int send_all(int sock,const char *buf,size_t len){
    size_t off=0; while(off<len){ ssize_t n=send(sock,buf+off,len-off,0); if(n<=0) return -1; off+=n; }
    return 0;
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
int recv_n(int sock,char *dst,size_t n){
    size_t off=0; while(off<n){ ssize_t r=recv(sock,dst+off,n-off,0); if(r<=0) return -1; off+=r; } return 0;
}
