#include "texture.h"

typedef struct {
    const char *path;
    int frame_w;
    int frame_h;
} TextureDef;


//This is basically the list of all textures avaiable. 
//TEX NONE MUST ALWAYS REMAIN THE FIRST
static const TextureDef texture_defs[TEX_COUNT] = {
    [TEX_NONE]   = { "assets/none.png", 16, 16 },
    [TEX_PLAYER] = { "assets/Actor/CharacterAnimated/NinjaGreen/SpriteSheet.png", 32, 32 },
};

static TextureAsset textures[TEX_COUNT];

//this just loads the previous shi)
void texture_load_all(void){
    for (int i = 0; i < TEX_COUNT; i++) {

        textures[i].texture = LoadTexture(texture_defs[i].path);
        //Texture integrity logic

        if(textures[i].texture.id == 0){
            if(i == TEX_NONE){
                TraceLog(LOG_FATAL, "ERROR! CHECK GAME INTEGRITY. %s NOT FOUND. THIS IS CORE TO THE GAME'S FUNCTIONALITY, EXITING NOW.", texture_defs[i].path);
            } else {
                TraceLog(LOG_WARNING, "ERROR! Asset %s could not load properly. Please check game integrity.", texture_defs[i].path);
                textures[i] = textures[TEX_NONE];
            }
        } else {

            textures[i].frame_w = texture_defs[i].frame_w;
            textures[i].frame_h = texture_defs[i].frame_h;
            textures[i].columns = textures[i].texture.width  / texture_defs[i].frame_w;
            textures[i].rows    = textures[i].texture.height / texture_defs[i].frame_h;
        }
    }
}

void texture_unload_all(void){
    for (int i = 0; i < TEX_COUNT; i++) {
        UnloadTexture(textures[i].texture);
    }
}

TextureAsset texture_get(TextureId id){
    return textures[id];
}