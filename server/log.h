#ifndef LOG_H
#define LOG_H
#include <stdio.h>
void log_init(const char *path);
void log_close(void);
void logf(const char *fmt, ...);
const char* now_iso(char *buf, size_t n);
#endif
