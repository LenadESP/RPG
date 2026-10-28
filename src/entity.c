#include "entity.h"
#include "raylib.h"
#include "collision.h"
#include "texture.h"

void entity_update(Entity *e, float dx, float dy, float dt, Entity *entities, int count) {
    Rectangle self  = collision_rect(e);
    Rectangle other;

    // --- EJE X ---
    e->x += dx * e->vel * dt;

    if (dx != 0.0f) {
        for (int i = 0; i < count; i++) {
            if (!entities[i].active) continue;
            if (!entities[i].solid) continue;
            if (&entities[i] == e) continue;
            if (!collision_check(e, &entities[i])) continue;

            other = collision_rect(&entities[i]);

            if (dx > 0.0f) e->x = other.x - self.width;
            else           e->x = other.x + other.width;
        }
    }

    // --- EJE Y ---
    e->y += dy * e->vel * dt;

    if (dy != 0.0f) {
        for (int i = 0; i < count; i++) {
            if (!entities[i].active) continue;
            if (!entities[i].solid) continue;
            if (&entities[i] == e) continue;
            if (!collision_check(e, &entities[i])) continue;

            other = collision_rect(&entities[i]);

            if (dy > 0.0f) e->y = other.y - self.height;
            else           e->y = other.y + other.height;
        }
    }
}

void entity_draw(const Entity *e){
    TextureAsset asset = texture_get(e->texture_id);
    DrawTextureRec(asset.texture,
                   (Rectangle){ 0, 0, asset.frame_w, asset.frame_h },
                   (Vector2){e->x - asset.off_x, e->y - asset.off_y},
                   WHITE);
}