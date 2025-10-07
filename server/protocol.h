#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stddef.h>

typedef struct {
    char method[32];
    char resource[64];
    char client_id[64];
    char vars[128];
    char token[128];
    int  content_length;
} req_t;

void req_init(req_t *r);
void parse_start_line(const char *line, req_t *r);
unsigned parse_vars_mask(const char *v);
#endif
