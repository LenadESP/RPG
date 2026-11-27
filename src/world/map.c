#include "map.h"
#include "parser.h"
#include "texture.h"
#include "raylib.h"
#include <stdlib.h>

void map_load(Map *map, const LayerDef *defs, int count, int lay_cut, const char *collision_path){
    
    map->layers = malloc(count * sizeof(Layer));

    map->layer_cut = lay_cut;

    int *parsed_text = NULL;
    int width = 0;
    int height = 0;

    for (int i = 0; i < count; i++){
        char *text = LoadFileText(defs[i].path);

        if(text != NULL){
            parsed_text = parse_csv(text, &width, &height);
            UnloadFileText(text);

            if(parsed_text != NULL){

                if(map->layers != NULL){
                    map->layers[i].tiles = parsed_text;
                    map->layers[i].tex = defs[i].tex;

                    map->w = width;
                    map->h = height;

                    //same as before
                    map->layer_amount = i+1;

                } else {
                    free(parsed_text);
                    TraceLog(LOG_FATAL, "Error allocating memory for the layer. wtf, how u got here dude");
                    break;
                }
            } else {
                TraceLog(LOG_FATAL, "Error loading the map, map parsing is NULL");
                break;
            }
        } else {
            TraceLog(LOG_FATAL, "Error loading the map, the file is empty or has not been found");
            break;
        }
    }

    char *collision_text = LoadFileText(collision_path);

    if (collision_text == NULL) {
        TraceLog(LOG_FATAL, "Collision layer not found: %s", collision_path);
    }

    int collision_w = 0;
    int collision_h = 0;

    map->collision = parse_csv(collision_text, &collision_w, &collision_h);
    UnloadFileText(collision_text);

    if (map->collision == NULL) {
        TraceLog(LOG_FATAL, "Collision layer failed to parse: %s", collision_path);
    }
}

void map_unload(Map *map){
    //If there's layers:
    for(int i = 0; i < map->layer_amount; i++){
        free(map->layers[i].tiles);
    }
    
    //Then we free the layers, set the pointer to NULL, and reset all values
    free(map->layers);
    free(map->collision);
    map->layers = NULL;
    map->collision = NULL;

    map->w = 0;
    map->h = 0;

    map->layer_amount = 0;
}

void map_draw(const Map *map, int first, int last){
    for (int l = first; l < last; l++) {

        const Layer *layer = &map->layers[l];
        TextureAsset asset = texture_get(layer->tex);

        for (int y = 0; y < map->h; y++) {
            for (int x = 0; x < map->w; x++) {

                int n = layer->tiles[y * map->w + x];

                // Tiled writes -1 for cells that were never painted.
                if (n < 0) continue;

                // Where this tile lives inside the tileset image.
                Rectangle source = {
                    (float)((n % asset.columns) * TILE_SIZE),
                    (float)((n / asset.columns) * TILE_SIZE),
                    (float)TILE_SIZE,
                    (float)TILE_SIZE
                };

                // Where it goes in the world.
                Vector2 position = {
                    (float)(x * TILE_SIZE),
                    (float)(y * TILE_SIZE)
                };

                DrawTextureRec(asset.texture, source, position, WHITE);
            }
        }
    }
}

/* Anything outside the map counts as solid, so the player cannot walk
   off the edge and we do not need border barriers. */
bool map_is_solid(const Map *map, int tile_x, int tile_y){
    if (tile_x < 0 || tile_x >= map->w) return true;
    if (tile_y < 0 || tile_y >= map->h) return true;

    return map->collision[tile_y * map->w + tile_x] >= 0;
}