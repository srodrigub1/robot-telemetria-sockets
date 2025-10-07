#include "protocol.h"
#include <string.h>
#include <strings.h>
#include <stdio.h>

void req_init(req_t* r){ memset(r,0,sizeof *r); r->content_length=0; }

void parse_start_line(const char* line, req_t* r){
    char proto[16]={0};
    sscanf(line,"%31s %63s %15s", r->method, r->resource, proto);
}

unsigned parse_vars_mask(const char* v){
    unsigned m=0; if(!v) return 0;
    char tmp[160]; snprintf(tmp,sizeof tmp,"%s",v);
    for(char* p=strtok(tmp,","); p; p=strtok(NULL,",")){
        while(*p==' '||*p=='\t') p++;
        if (!strcasecmp(p,"temp"))  m|=1u<<0;
        else if(!strcasecmp(p,"hum"))   m|=1u<<1;
        else if(!strcasecmp(p,"press")) m|=1u<<2;
        else if(!strcasecmp(p,"co2"))   m|=1u<<3;
    }
    return m;
}
