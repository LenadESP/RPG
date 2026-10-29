#include "entity.h"
#include "raylib.h"
#include "collision.h"
#include "texture.h"

void entity_update(Entity *e, float dx, float dy, float dt, Entity *entities, int count) {
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

            // x is the horizontal CENTER of the hitbox
            if (dx > 0.0f) e->x = other.x - e->hb_w / 2.0f;
            else           e->x = other.x + other.width + e->hb_w / 2.0f;
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

            // y is the BOTTOM edge of the hitbox
            if (dy > 0.0f) e->y = other.y;
            else           e->y = other.y + other.height + e->hb_h;
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