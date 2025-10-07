#include "auth.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    const char *user;
    const char *pass;
    bool is_admin;
} credential_t;

static const credential_t g_creds[] = {
    {"victor",    "1234",  false},
    {"sebastian", "1234",  false},
    {"admin",     "12345", true }
};

void auth_init(void){
    srand((unsigned)time(NULL));
}

bool auth_check(const char *u,const char *p,bool *is_admin){
    if(!u || !p) return false;
    for(size_t i=0;i<sizeof(g_creds)/sizeof(g_creds[0]);++i){
        if(strcmp(g_creds[i].user,u)==0 && strcmp(g_creds[i].pass,p)==0){
            if(is_admin) *is_admin = g_creds[i].is_admin;
            return true;
        }
    }
    return false;
}

void auth_make_token(char *out,size_t n){
    static const char *hex="0123456789abcdef";
    if(!out||n<37){ if(out&&n>0) out[0]=0; return; }
    for(int i=0;i<36;i++){
        int r=rand()%16;
        switch(i){
            case 8: case 13: case 18: case 23: out[i]='-'; break;
            case 14: out[i]='4'; break;
            case 19: out[i]=hex[(rand()%4)+8]; break;
            default: out[i]=hex[r]; break;
        }
    }
    out[36]='\0';
}
