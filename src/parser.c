#include "parser.h"

#include <stdlib.h>

/* Turns a CSV of integers into a flat array, row by row.
   Returns NULL on failure. The caller owns the array and must free it. */
int *parse_csv(const char *text, int *out_width, int *out_height)
{
    /* First pass: figure out how big the grid is, so we know how much to reserve. */

    /* One more column than there are commas on the first line. */
    int width = 1;
    for (const char *cursor = text; *cursor != '\0' && *cursor != '\n'; cursor++) {
        if (*cursor == ',') width++;
    }

    /* One row per line that actually has characters on it.
       This skips the trailing newline at the end of the file. */
    int height = 0;
    const char *cursor = text;
    while (*cursor != '\0') {
        const char *line_start = cursor;
        while (*cursor != '\0' && *cursor != '\n') cursor++;
        if (cursor > line_start) height++;
        if (*cursor == '\n') cursor++;
    }

    if (width <= 0 || height <= 0) return NULL;

    int *tiles = malloc((size_t)width * (size_t)height * sizeof(int));
    if (tiles == NULL) return NULL;

    /* Second pass: read width*height numbers in order.
       Separators do not matter, a comma and a newline are both just "next number". */
    const char *read_head = text;
    for (int i = 0; i < width * height; i++) {
        char *after_number;

        tiles[i] = (int)strtol(read_head, &after_number, 10);

        /* strtol did not move: there was no number where one was expected. */
        if (after_number == read_head) {
            free(tiles);
            return NULL;
        }

        read_head = after_number;

        while (*read_head == ',' || *read_head == '\n' ||
               *read_head == '\r' || *read_head == ' ') {
            read_head++;
        }
    }

    *out_width = width;
    *out_height = height;
    return tiles;
}