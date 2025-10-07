#define _GNU_SOURCE
#include "log.h"
#include <pthread.h>
#include <stdarg.h>
#include <time.h>

static FILE *g_logf = NULL;
static pthread_mutex_t log_mx = PTHREAD_MUTEX_INITIALIZER;

const char* now_iso(char *buf, size_t n){
    struct timespec ts; clock_gettime(CLOCK_REALTIME,&ts);
    struct tm tm; gmtime_r(&ts.tv_sec,&tm);
    snprintf(buf,n,"%04d-%02d-%02dT%02d:%02d:%02dZ",
             tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec);
    return buf;
}

void log_init(const char *path){
    if (path) g_logf = fopen(path,"a");
}
void log_close(void){ if (g_logf){ fclose(g_logf); g_logf=NULL; } }

void logf(const char *fmt, ...){
    pthread_mutex_lock(&log_mx);
    char ts[32]; now_iso(ts,sizeof ts);
    fprintf(stdout,"[%s] ", ts);
    if (g_logf) fprintf(g_logf,"[%s] ", ts);
    va_list ap; va_start(ap,fmt);
    vfprintf(stdout,fmt,ap);
    if (g_logf){ va_list ap2; va_start(ap2,fmt); vfprintf(g_logf,fmt,ap2); va_end(ap2); }
    va_end(ap);
    fprintf(stdout,"\n"); fflush(stdout);
    if (g_logf){ fprintf(g_logf,"\n"); fflush(g_logf); }
    pthread_mutex_unlock(&log_mx);
}
