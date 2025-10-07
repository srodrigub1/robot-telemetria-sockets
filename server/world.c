#include "world.h"
#include <pthread.h>
#include <string.h>
#include <strings.h>

static int g_map[WORLD_H][WORLD_W];
static int g_x=1, g_y=1;
static pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;

static inline int inb(int x,int y){ return x>=0 && y>=0 && x<WORLD_W && y<WORLD_H; }

void world_init(void){
    pthread_mutex_lock(&mx);
    memset(g_map,0,sizeof g_map);
    // perímetro como paredes
    for(int x=0;x<WORLD_W;++x){ g_map[0][x]=1; g_map[WORLD_H-1][x]=1; }
    for(int y=0;y<WORLD_H;++y){ g_map[y][0]=1; g_map[y][WORLD_W-1]=1; }
    // obstáculos internos de ejemplo
    g_map[3][3]=g_map[3][4]=g_map[4][3]=1;
    g_map[6][6]=g_map[6][7]=1;
    g_map[2][7]=1;
    // posición inicial
    g_x=1; g_y=1;
    pthread_mutex_unlock(&mx);
}

void world_get_pos(int *x,int *y){
    pthread_mutex_lock(&mx);
    if(x)*x=g_x; if(y)*y=g_y;
    pthread_mutex_unlock(&mx);
}

bool world_parse_dir(const char *s, world_dir_t *d){
    if(!s||!d) return false;
    if(!strcasecmp(s,"LEFT"))  {*d=DIR_LEFT;  return true;}
    if(!strcasecmp(s,"RIGHT")) {*d=DIR_RIGHT; return true;}
    if(!strcasecmp(s,"UP"))    {*d=DIR_UP;    return true;}
    if(!strcasecmp(s,"DOWN"))  {*d=DIR_DOWN;  return true;}
    return false;
}

world_status_t world_move(world_dir_t dir,int steps,int *ox,int *oy){
    if(steps<=0) steps=1;
    pthread_mutex_lock(&mx);
    int x=g_x, y=g_y;
    for(int i=0;i<steps;i++){
        int nx=x, ny=y;
        switch(dir){
            case DIR_LEFT:  nx=x-1; break;
            case DIR_RIGHT: nx=x+1; break;
            case DIR_UP:    ny=y-1; break;
            case DIR_DOWN:  ny=y+1; break;
            default:
                if(ox)*ox=x; if(oy)*oy=y; pthread_mutex_unlock(&mx);
                return WORLD_INVALID_DIR;
        }
        if(!inb(nx,ny)){
            if(ox)*ox=x; if(oy)*oy=y; pthread_mutex_unlock(&mx);
            return WORLD_OOB;
        }
        if(g_map[ny][nx]==1){
            if(ox)*ox=x; if(oy)*oy=y; pthread_mutex_unlock(&mx);
            return WORLD_OBSTACLE;
        }
        x=nx; y=ny;
    }
    g_x=x; g_y=y;
    if(ox)*ox=g_x; if(oy)*oy=g_y;
    pthread_mutex_unlock(&mx);
    return WORLD_OK;
}
