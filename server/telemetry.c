#include "telemetry.h"
#include "common.h"
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <stdio.h>

static double rand_range(double min,double max){
    double r = (double)rand() / (double)RAND_MAX;
    return min + (max-min)*r;
}

void telemetry_init(void){
    static bool seeded = false;
    if(!seeded){
        srand((unsigned)time(NULL));
        seeded = true;
    }
}

void telemetry_generate(unsigned mask, char *out, size_t cap){
    if(!out || cap==0) return;
    telemetry_init();
    if(mask==0) mask = VAR_TEMP | VAR_HUM | VAR_PRESS;

    double temp  = rand_range(18.0, 28.0);
    double hum   = rand_range(35.0, 65.0);
    double press = rand_range(1008.0, 1022.0);
    double co2   = rand_range(350.0, 550.0);

    size_t off = 0;
    bool first = true;
    off += snprintf(out+off, cap-off, "{");
    if((mask & VAR_TEMP) && off<cap){
        off += snprintf(out+off, cap-off, "%s\"temp\":%.1f", first?"":", ", temp);
        first = false;
    }
    if((mask & VAR_HUM) && off<cap){
        off += snprintf(out+off, cap-off, "%s\"hum\":%.1f", first?"":", ", hum);
        first = false;
    }
    if((mask & VAR_PRESS) && off<cap){
        off += snprintf(out+off, cap-off, "%s\"press\":%.1f", first?"":", ", press);
        first = false;
    }
    if((mask & VAR_CO2) && off<cap){
        off += snprintf(out+off, cap-off, "%s\"co2\":%.1f", first?"":", ", co2);
        first = false;
    }
    if(off<cap){
        snprintf(out+off, cap-off, "}");
    }else if(cap>0){
        out[cap-1]='\0';
    }
}
