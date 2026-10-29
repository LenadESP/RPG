#include "map.h"
#include "parser.h"
#include "texture.h"
#include "raylib.h"
#include <stdlib.h>

void map_load(Map *map){
    //Hardcoded as a placeholder until I wire level.c
    char *text = LoadFileText("assets/Maps/map.csv");

    int *parsed_text = NULL;
    int width = 0;
    int height = 0;

    if(text != NULL){
        parsed_text = parse_csv(text, &width, &height);
        UnloadFileText(text);

        if(parsed_text != NULL){
            //Single layer, also a placeholder (that 1)
            map->layers = malloc(1 * sizeof(Layer));

            if(map->layers != NULL){
                map->layers[0].tiles = parsed_text;
                map->layers[0].tex = TEX_TILESET_FLOOR;

                map->w = width;
                map->h = height;

                //same as before
                map->layer_amount = 1;

            } else {
                free(parsed_text);
                TraceLog(LOG_WARNING, "Error allocating memory for the layer. wtf, how u got here dude");
            }
        }

        else {
            TraceLog(LOG_WARNING, "Error loading the map, map parsing is NULL");
        }

    } else {TraceLog(LOG_WARNING, "Error loading the map, the file is empty or has not been found");}
}

void map_unload(Map *map){
    //If there's layers:
    for(int i = 0; i < map->layer_amount; i++){
        free(map->layers[i].tiles);
    }
    
    //Then we free the layers, set the pointer to NULL, and reset all values
    free(map->layers);
    map->layers = NULL;

    map->w = 0;
    map->h = 0;

    map->layer_amount = 0;
}

void map_draw(const Map *map){
    for (int l = 0; l < map->layer_amount; l++) {

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

void map_is_solid(){

}