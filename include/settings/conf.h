#ifndef CONF_H
#define CONF_H

#include <stdbool.h>

//A .conf file is one "key: value" per line. keys[i] goes with values[i]
bool conf_read(const char *path, const char *const *keys, int *values, int count);
void conf_write(const char *path, const char *const *keys, const int *values, int count);

#endif
