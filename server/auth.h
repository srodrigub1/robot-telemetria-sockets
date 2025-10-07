#ifndef AUTH_H
#define AUTH_H
#include <stddef.h>
#include <stdbool.h>
void auth_init(void);
bool auth_check(const char *user,const char *pass);
void auth_make_token(char *out,size_t n);
#endif
