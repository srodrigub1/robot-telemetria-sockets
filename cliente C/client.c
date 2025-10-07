#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <strings.h>

#define BUF_SZ 1024

static void strip_line(char *s){
    if(!s) return;
    size_t len = strlen(s);
    while(len>0 && (s[len-1]=='\r' || s[len-1]=='\n')){
        s[--len] = '\0';
    }
}

static void trim(char *s){
    if(!s) return;
    char *start = s;
    while(*start && isspace((unsigned char)*start)) start++;
    char *end = start + strlen(start);
    while(end>start && isspace((unsigned char)end[-1])) end--;
    size_t len = (size_t)(end-start);
    if(start!=s) memmove(s,start,len);
    s[len]='\0';
}

static int send_all(int sock, const void *buf, size_t len){
    const char *p = (const char*)buf;
    size_t off=0;
    while(off<len){
        ssize_t n = send(sock,p+off,len-off,0);
        if(n<=0) return -1;
        off += (size_t)n;
    }
    return 0;
}

static int recv_line(int sock, char *dst, size_t cap){
    size_t off=0;
    while(off+1<cap){
        char c;
        ssize_t n = recv(sock,&c,1,0);
        if(n<=0) return -1;
        dst[off++] = c;
        if(c=='\n') break;
    }
    dst[off] = '\0';
    return (int)off;
}

static int recv_n(int sock, char *dst, size_t len){
    size_t off=0;
    while(off<len){
        ssize_t n = recv(sock,dst+off,len-off,0);
        if(n<=0) return -1;
        off += (size_t)n;
    }
    return 0;
}

static bool json_get_string(const char *json, const char *key, char *out, size_t out_len){
    if(!json || !key || !out || out_len==0) return false;
    char pattern[64];
    snprintf(pattern,sizeof pattern,"\"%s\"", key);
    const char *pos = strstr(json, pattern);
    if(!pos) return false;
    pos += strlen(pattern);
    while(*pos && (*pos==':' || isspace((unsigned char)*pos))) pos++;
    if(*pos!='"') return false;
    pos++;
    size_t idx=0;
    while(pos[idx] && pos[idx]!='"' && idx+1<out_len){
        out[idx]=pos[idx];
        idx++;
    }
    if(pos[idx] != '"') return false;
    out[idx]='\0';
    return true;
}

static int connect_to(const char *host, int port){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock<0) return -1;
    struct sockaddr_in addr;
    memset(&addr,0,sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if(inet_pton(AF_INET, host, &addr.sin_addr)<=0){
        perror("inet_pton");
        close(sock);
        return -1;
    }
    if(connect(sock,(struct sockaddr*)&addr,sizeof addr)<0){
        perror("connect");
        close(sock);
        return -1;
    }
    return sock;
}

static int send_request(int sock, const char *method, const char *resource,
                        const char *client_id, const char *token, const char *vars,
                        const char *body){
    char header[BUF_SZ];
    size_t body_len = body? strlen(body):0;
    int written = snprintf(header,sizeof header,"%s %s RTTP/1.0\r\n", method, resource);
    if(written<0 || (size_t)written>=sizeof header) return -1;
    if(client_id && *client_id){
        written += snprintf(header+written,sizeof header - (size_t)written,
                             "Client-Id: %s\r\n", client_id);
    }
    if(token && *token){
        written += snprintf(header+written,sizeof header - (size_t)written,
                             "Token: %s\r\n", token);
    }
    if(vars && *vars){
        written += snprintf(header+written,sizeof header - (size_t)written,
                             "Vars: %s\r\n", vars);
    }
    written += snprintf(header+written,sizeof header - (size_t)written,
                        "Content-Length: %zu\r\n\r\n", body_len);
    if(written<0 || (size_t)written>=sizeof header) return -1;
    if(send_all(sock,header,(size_t)written)<0) return -1;
    if(body_len>0 && send_all(sock,body,body_len)<0) return -1;
    return 0;
}

static int read_response(int sock, int *code, char *reason, size_t reason_len, char *body, size_t body_cap){
    char line[BUF_SZ];
    if(recv_line(sock,line,sizeof line)<=0) return -1;
    strip_line(line);
    if(sscanf(line,"RTTP/1.0 %d %63s", code, reason_len>0?reason:line)<2){
        return -1;
    }
    int content_length = 0;
    while(1){
        if(recv_line(sock,line,sizeof line)<=0) return -1;
        strip_line(line);
        if(line[0]=='\0') break;
        if(strncasecmp(line,"Content-Length:",15)==0){
            char *value = line+15;
            trim(value);
            content_length = atoi(value);
        }
    }
    if(content_length<0) content_length=0;
    if(body_cap>0){
        if((size_t)content_length >= body_cap){
            fprintf(stderr,"response body too large (%d)\n", content_length);
            return -1;
        }
        if(content_length>0){
            if(recv_n(sock,body,(size_t)content_length)<0) return -1;
        }
        body[content_length]='\0';
    }
    return 0;
}

int main(int argc, char **argv){
    if(argc<3){
        fprintf(stderr,"usage: %s <server_ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *host = argv[1];
    int port = atoi(argv[2]);
    if(port<=0){
        fprintf(stderr,"invalid port\n");
        return EXIT_FAILURE;
    }

    int sock = connect_to(host,port);
    if(sock<0){
        return EXIT_FAILURE;
    }
    printf("Conectado a %s:%d\n", host, port);

    char user[64];
    char pass[64];
    printf("Usuario: ");
    if(!fgets(user,sizeof user,stdin)){ close(sock); return EXIT_FAILURE; }
    trim(user);
    printf("Contraseña: ");
    if(!fgets(pass,sizeof pass,stdin)){ close(sock); return EXIT_FAILURE; }
    trim(pass);

    char body[256];
    snprintf(body,sizeof body,"{\"user\":\"%s\",\"pass\":\"%s\"}", user, pass);
    if(send_request(sock,"AUTH","/login",user,NULL,NULL,body)<0){
        fprintf(stderr,"no se pudo enviar AUTH\n");
        close(sock);
        return EXIT_FAILURE;
    }

    int code=0; char reason[64]={0}; char resp_body[BUF_SZ];
    if(read_response(sock,&code,reason,sizeof reason,resp_body,sizeof resp_body)<0){
        fprintf(stderr,"error leyendo respuesta AUTH\n");
        close(sock);
        return EXIT_FAILURE;
    }
    printf("[%d %s] %s\n", code, reason, resp_body);
    if(code!=200){
        close(sock);
        return EXIT_FAILURE;
    }
    char token[64]={0};
    if(!json_get_string(resp_body,"token",token,sizeof token)){
        fprintf(stderr,"no se recibió token\n");
        close(sock);
        return EXIT_FAILURE;
    }

    char command[128];
    while(1){
        printf("comando (move/pos/sense/logout/exit)> ");
        if(!fgets(command,sizeof command,stdin)) break;
        trim(command);
        if(command[0]=='\0') continue;
        if(strncasecmp(command,"exit",4)==0 || strncasecmp(command,"quit",4)==0){
            break;
        }else if(strncasecmp(command,"move",4)==0){
            char dir[32];
            int steps=1;
            if(sscanf(command+4,"%31s %d",dir,&steps)<1){
                printf("uso: move <LEFT|RIGHT|UP|DOWN> [pasos]\n");
                continue;
            }
            for(size_t i=0;i<strlen(dir);++i) dir[i] = (char)toupper((unsigned char)dir[i]);
            if(steps<=0) steps=1;
            snprintf(body,sizeof body,"{\"dir\":\"%s\",\"steps\":%d}", dir, steps);
            if(send_request(sock,"MOVE","/",user,token,NULL,body)<0){
                printf("error enviando MOVE\n");
                break;
            }
        }else if(strncasecmp(command,"pos",3)==0){
            if(send_request(sock,"GET","/pos",user,token,NULL,"")<0){
                printf("error enviando POS\n");
                break;
            }
        }else if(strncasecmp(command,"sense",5)==0){
            char *vars = command+5;
            trim(vars);
            const char *vars_header = NULL;
            char vars_buf[128];
            if(vars[0]){
                snprintf(vars_buf,sizeof vars_buf,"%s", vars);
                vars_header = vars_buf;
            }
            if(send_request(sock,"GET","/telemetry",user,token,vars_header,"")<0){
                printf("error enviando SENSE\n");
                break;
            }
        }else if(strncasecmp(command,"logout",6)==0){
            if(send_request(sock,"LOGOUT","/",user,token,NULL,"")<0){
                printf("error enviando LOGOUT\n");
                break;
            }
        }else{
            printf("comando desconocido\n");
            continue;
        }
        if(read_response(sock,&code,reason,sizeof reason,resp_body,sizeof resp_body)<0){
            printf("error leyendo respuesta\n");
            break;
        }
        printf("[%d %s] %s\n", code, reason, resp_body);
        if(code==200 && strcasecmp(command,"logout")==0){
            printf("sesión finalizada\n");
            break;
        }
    }

    close(sock);
    return EXIT_SUCCESS;
}
