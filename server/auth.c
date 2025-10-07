#include "auth.h"
#include <stdlib.h>
#include <string.h>

void auth_init(void){ /* aquí podrías cargar archivo users.db */ }

bool auth_check(const char *u,const char *p){
    return (u && p && strcmp(u,"admin")==0 && strcmp(p,"admin")==0);
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
