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
    int line_number = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        char key[64];
        int value;

        line_number++;

        //Blank lines and comments (starting with #) are skipped
        int first = 0;

        while (line[first] == ' ' || line[first] == '\t') {
            first++;
        }

        if (line[first] == '\n' || line[first] == '\r' || line[first] == '\0' || line[first] == '#') {
            continue;
        }

        //Anything else has to be "key: number"
        if (sscanf(line, " %63[^:]: %d", key, &value) != 2) {
            TraceLog(LOG_WARNING, "ERROR! Line %d of %s could not be read. Skipping it.", line_number, path);
            continue;
        }

        //"key_up : 87" leaves spaces at the end of the key, take them out
        int length = strlen(key);

        while (length > 0 && (key[length - 1] == ' ' || key[length - 1] == '\t')) {
            length--;
            key[length] = '\0';
        }

        for (int i = 0; i < count; i++) {
            if (strcmp(key, keys[i]) == 0) {
                values[i] = value;
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
