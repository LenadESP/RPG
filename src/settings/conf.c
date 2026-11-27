#include "conf.h"

#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

//Keys that aren't in the file keep the value they already had
bool conf_read(const char *path, const char *const *keys, int *values, int count){
    FILE *file = fopen(path, "r");

    if (file == NULL) {
        TraceLog(LOG_WARNING, "ERROR! %s could not be read. Using the defaults.", path);
        return false;
    }

    char line[128];

    while (fgets(line, sizeof(line), file) != NULL) {
        char key[64];
        int value;

        //Anything that isn't "key: number" (comments, blank lines) is skipped
        if (sscanf(line, " %63[^:]: %d", key, &value) == 2) {
            for (int i = 0; i < count; i++) {
                if (strcmp(key, keys[i]) == 0) {
                    values[i] = value;
                }
            }
        }
    }

    fclose(file);
    return true;
}

void conf_write(const char *path, const char *const *keys, const int *values, int count){
    FILE *file = fopen(path, "w");

    if (file == NULL) {
        TraceLog(LOG_WARNING, "ERROR! %s could not be saved.", path);
        return;
    }

    for (int i = 0; i < count; i++) {
        fprintf(file, "%s: %d\n", keys[i], values[i]);
    }

    fclose(file);
}
