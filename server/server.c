#include "common.h"
#include "net.h"
#include "protocol.h"
#include "world.h"
#include "auth.h"
#include "log.h"
#include "telemetry.h"

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

#define BODY_BUF_SZ 1024

static client_t clients[MAX_CLIENTS];
static pthread_mutex_t clients_mx = PTHREAD_MUTEX_INITIALIZER;
static int server_sock = -1;
static volatile sig_atomic_t shutting_down = 0;

static void handle_signal(int sig){
    (void)sig;
    shutting_down = 1;
    if(server_sock >= 0){
        close_socket(server_sock);
    }
}

static void trim_inplace(char *s){
    if(!s) return;
    char *start = s;
    while(*start && isspace((unsigned char)*start)) start++;
    char *end = start + strlen(start);
    while(end>start && isspace((unsigned char)end[-1])) end--;
    size_t len = (size_t)(end-start);
    if(start!=s) memmove(s,start,len);
    s[len] = '\0';
}

static void strip_line(char *s){
    if(!s) return;
    size_t len = strlen(s);
    while(len>0 && (s[len-1]=='\n' || s[len-1]=='\r')){
        s[--len] = '\0';
    }
}

static client_t* client_alloc(int sock, const struct sockaddr_in *addr){
    pthread_mutex_lock(&clients_mx);
    client_t *slot = NULL;
    for(size_t i=0;i<MAX_CLIENTS;i++){
        if(!clients[i].in_use){
            slot = &clients[i];
            memset(slot,0,sizeof *slot);
            slot->sock = sock;
            if(addr) slot->addr = *addr;
            slot->in_use = true;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mx);
    return slot;
}

static void client_release(client_t *c){
    if(!c) return;
    if(c->sock>=0){
        close_socket(c->sock);
    }
    pthread_mutex_lock(&clients_mx);
    memset(c,0,sizeof *c);
    pthread_mutex_unlock(&clients_mx);
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
        out[idx] = pos[idx];
        idx++;
    }
    if(pos[idx] != '"') return false;
    out[idx] = '\0';
    return true;
}

static bool json_get_int(const char *json, const char *key, int *value){
    if(!json || !key || !value) return false;
    char pattern[64];
    snprintf(pattern,sizeof pattern,"\"%s\"", key);
    const char *pos = strstr(json, pattern);
    if(!pos) return false;
    pos += strlen(pattern);
    while(*pos && (*pos==':' || isspace((unsigned char)*pos))) pos++;
    if(!*pos) return false;
    char *end=NULL;
    long v = strtol(pos,&end,10);
    if(end==pos) return false;
    *value = (int)v;
    return true;
}

static int send_json(int sock, int code, const char *reason, const char *fmt, ...){
    char body[512];
    va_list ap;
    va_start(ap,fmt);
    vsnprintf(body,sizeof body,fmt,ap);
    va_end(ap);
    return send_response(sock,code,reason,body);
}

static void send_error(client_t *client, int code, const char *reason, const char *message){
    log_msgf("client %d error %d %s: %s", client->sock, code, reason?reason:"", message?message:"");
    send_json(client->sock, code, reason, "{\"error\":\"%s\"}", message?message:"internal");
}

static bool require_auth(client_t *client, const req_t *req){
    if(client->token[0]=='\0'){
        send_error(client,401,"UNAUTHORIZED","login required");
        return false;
    }
    if(req->token[0]=='\0'){
        send_error(client,401,"UNAUTHORIZED","token missing");
        return false;
    }
    if(strcmp(req->token,client->token)!=0){
        send_error(client,403,"FORBIDDEN","invalid token");
        return false;
    }
    return true;
}

static void handle_auth(client_t *client, const req_t *req, const char *body){
    (void)req;
    char user[64]={0};
    char pass[64]={0};
    bool is_admin=false;
    if(!json_get_string(body,"user",user,sizeof user) || !json_get_string(body,"pass",pass,sizeof pass)){
        send_error(client,400,"BAD_REQUEST","missing credentials");
        return;
    }
    if(!auth_check(user,pass,&is_admin)){
        send_error(client,401,"UNAUTHORIZED","invalid credentials");
        return;
    }
    auth_make_token(client->token,sizeof client->token);
    snprintf(client->user,sizeof client->user,"%s",user);
    client->is_admin = is_admin;
    log_msgf("client %d authenticated as %s (admin=%s)", client->sock, client->user, is_admin?"true":"false");
    send_json(client->sock,200,"OK","{\"status\":\"OK\",\"token\":\"%s\",\"admin\":%s}",
              client->token, is_admin?"true":"false");
}

static void handle_move(client_t *client, const req_t *req, const char *body){
    if(!require_auth(client,req)) return;
    char dirbuf[32]={0};
    if(!json_get_string(body,"dir",dirbuf,sizeof dirbuf)){
        send_error(client,400,"BAD_REQUEST","dir required");
        return;
    }
    world_dir_t dir;
    if(!world_parse_dir(dirbuf,&dir)){
        send_error(client,400,"BAD_REQUEST","invalid dir");
        return;
    }
    int steps=1;
    if(json_get_int(body,"steps",&steps)){
        if(steps<=0) steps=1;
        if(steps>WORLD_W+WORLD_H) steps = WORLD_W+WORLD_H;
    }
    int x=0,y=0;
    world_status_t st = world_move(dir,steps,&x,&y);
    switch(st){
        case WORLD_OK:
            send_json(client->sock,200,"OK","{\"status\":\"OK\",\"x\":%d,\"y\":%d}",x,y);
            break;
        case WORLD_OBSTACLE:
            send_json(client->sock,409,"OBSTACLE","{\"status\":\"OBSTACLE\",\"x\":%d,\"y\":%d}",x,y);
            break;
        case WORLD_OOB:
            send_json(client->sock,409,"OOB","{\"status\":\"OOB\",\"x\":%d,\"y\":%d}",x,y);
            break;
        case WORLD_INVALID_DIR:
        default:
            send_error(client,400,"BAD_REQUEST","invalid dir");
            break;
    }
}

static void handle_pos(client_t *client, const req_t *req){
    if(!require_auth(client,req)) return;
    int x=0,y=0;
    world_get_pos(&x,&y);
    send_json(client->sock,200,"OK","{\"x\":%d,\"y\":%d}",x,y);
}

static void handle_telemetry(client_t *client, req_t *req){
    if(!require_auth(client,req)) return;
    unsigned mask = parse_vars_mask(req->vars);
    if(mask==0) mask = VAR_TEMP | VAR_HUM | VAR_PRESS;
    if(!client->is_admin){
        mask &= ~VAR_CO2;
    }
    char payload[256];
    telemetry_generate(mask,payload,sizeof payload);
    send_json(client->sock,200,"OK","%s",payload);
}

static void handle_logout(client_t *client){
    if(client->token[0]=='\0'){
        send_error(client,400,"BAD_REQUEST","not logged in");
        return;
    }
    log_msgf("client %d logout", client->sock);
    client->token[0]='\0';
    client->is_admin=false;
    client->user[0]='\0';
    send_json(client->sock,200,"OK","{\"status\":\"BYE\"}");
}

static int read_request(client_t *client, req_t *req, char *body, size_t body_cap){
    char line[BUF_SZ];
    while(1){
        int n = recv_line(client->sock,line,sizeof line);
        if(n<=0) return -1;
        strip_line(line);
        if(line[0]=='\0') continue;
        req_init(req);
        parse_start_line(line,req);
        break;
    }
    while(1){
        int n = recv_line(client->sock,line,sizeof line);
        if(n<=0) return -1;
        strip_line(line);
        if(line[0]=='\0') break;
        char *value=NULL;
        if(strncasecmp(line,"Client-Id:",10)==0){
            value = line+10;
            trim_inplace(value);
            snprintf(req->client_id,sizeof req->client_id,"%s",value);
        }else if(strncasecmp(line,"Vars:",5)==0){
            value = line+5;
            trim_inplace(value);
            snprintf(req->vars,sizeof req->vars,"%s",value);
        }else if(strncasecmp(line,"Token:",6)==0){
            value = line+6;
            trim_inplace(value);
            snprintf(req->token,sizeof req->token,"%s",value);
        }else if(strncasecmp(line,"Content-Length:",15)==0){
            value = line+15;
            trim_inplace(value);
            req->content_length = atoi(value);
            if(req->content_length < 0) req->content_length = 0;
            if(req->content_length >= (int)body_cap){
                send_error(client,413,"PAYLOAD_TOO_LARGE","body too big");
                return -2;
            }
        }
    }
    if(req->content_length>0){
        if(recv_n(client->sock,body,(size_t)req->content_length)<0) return -1;
        body[req->content_length]='\0';
    }else if(body_cap>0){
        body[0]='\0';
    }
    return 0;
}

static void process_request(client_t *client, req_t *req, char *body){
    if(strcasecmp(req->method,"AUTH")==0){
        handle_auth(client,req,body);
    }else if(strcasecmp(req->method,"MOVE")==0){
        handle_move(client,req,body);
    }else if(strcasecmp(req->method,"GET")==0){
        if(strcasecmp(req->resource,"/pos")==0){
            handle_pos(client,req);
        }else if(strcasecmp(req->resource,"/telemetry")==0){
            handle_telemetry(client,req);
        }else{
            send_error(client,404,"NOT_FOUND","unknown resource");
        }
    }else if(strcasecmp(req->method,"LOGOUT")==0){
        handle_logout(client);
    }else{
        send_error(client,400,"BAD_REQUEST","unknown method");
    }
}

static void *client_thread(void *arg){
    client_t *client = (client_t*)arg;
    pthread_detach(pthread_self());
    log_msgf("client %d connected", client->sock);
    char body[BODY_BUF_SZ];
    while(!shutting_down){
        req_t req; req_init(&req);
        int rc = read_request(client,&req,body,sizeof body);
        if(rc==-2) break;
        if(rc<0){
            log_msgf("client %d disconnected", client->sock);
            break;
        }
        process_request(client,&req,body);
    }
    client_release(client);
    return NULL;
}

int main(int argc, char **argv){
    int port = 15000;
    if(argc>1){
        port = atoi(argv[1]);
        if(port<=0) port = 15000;
    }

    log_init(NULL);
    auth_init();
    world_init();
    telemetry_init();

    struct sigaction sa;
    memset(&sa,0,sizeof sa);
    sa.sa_handler = handle_signal;
    sigaction(SIGINT,&sa,NULL);
    sigaction(SIGTERM,&sa,NULL);

    server_sock = create_server_socket(port);
    if(server_sock<0){
        perror("create_server_socket");
        return EXIT_FAILURE;
    }
    log_msgf("server listening on port %d", port);

    while(!shutting_down){
        struct sockaddr_in addr;
        socklen_t alen = sizeof addr;
        int csock = accept(server_sock,(struct sockaddr*)&addr,&alen);
        if(csock<0){
            if(shutting_down) break;
            if(errno==EINTR) continue;
            perror("accept");
            continue;
        }
        client_t *client = client_alloc(csock,&addr);
        if(!client){
            log_msgf("rejecting connection: max clients reached");
            send_json(csock,503,"BUSY","{\"error\":\"busy\"}");
            close_socket(csock);
            continue;
        }
        pthread_t th;
        if(pthread_create(&th,NULL,client_thread,client)!=0){
            log_msgf("failed to spawn thread for client %d", csock);
            send_json(csock,500,"ERROR","{\"error\":\"server error\"}");
            client_release(client);
        }
    }

    if(server_sock>=0) close_socket(server_sock);
    log_msgf("server shutting down");
    log_close();
    return EXIT_SUCCESS;
}
