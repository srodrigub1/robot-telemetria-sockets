#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stddef.h>

void telemetry_init(void);
void telemetry_generate(unsigned mask, char *out, size_t cap);

#endif
