#include "entity.h"
#include "raylib.h"
#include "collision.h"
#include "texture.h"
#include "map.h"

#include <math.h>

/* ── tile collision helpers ─────────────────────────────────────────── */

typedef struct TileRange {
    int first_x;
    int last_x;
    int first_y;
    int last_y;
} TileRange;

/* The range of map cells a hitbox overlaps.
   It can include one extra row or column when an edge of the box lands
   exactly on a cell border. That is harmless: CheckCollisionRecs uses
   strict inequalities, so a cell that is merely touched gets discarded
   by the overlap test later on. */
static TileRange tiles_under(Rectangle box) {
    TileRange range;

    /* floorf before the cast: casting a negative float to int truncates
       toward zero, which would pick the wrong cell at negative
       coordinates. */
    range.first_x = (int)floorf(box.x / TILE_SIZE);
    range.last_x  = (int)floorf((box.x + box.width) / TILE_SIZE);
    range.first_y = (int)floorf(box.y / TILE_SIZE);
    range.last_y  = (int)floorf((box.y + box.height) / TILE_SIZE);

    return range;
}

static Rectangle tile_rect(int tile_x, int tile_y) {
    return (Rectangle){ (float)(tile_x * TILE_SIZE),
                        (float)(tile_y * TILE_SIZE),
                        (float)TILE_SIZE,
                        (float)TILE_SIZE };
}

/* Pushes the entity out of any solid cell it landed in after moving along
   X. Call only when dx is not zero.

   The cell range is computed once, before any snapping. That holds because
   an entity moves far less than one tile per tick (130 px/s at 60 ticks is
   about 2.2 px against a 16 px tile), so it can never skip a cell. */
static void resolve_tiles_x(Entity *e, const Map *map, float dx) {
    Rectangle box = collision_rect(e);
    TileRange range = tiles_under(box);

    for (int tile_y = range.first_y; tile_y <= range.last_y; tile_y++) {
        for (int tile_x = range.first_x; tile_x <= range.last_x; tile_x++) {
            if (!map_is_solid(map, tile_x, tile_y)) continue;

            Rectangle tile = tile_rect(tile_x, tile_y);
            if (!CheckCollisionRecs(box, tile)) continue;

            // x is the horizontal CENTER of the hitbox
            if (dx > 0.0f) e->x = tile.x - e->hb_w / 2.0f;
            else           e->x = tile.x + TILE_SIZE + e->hb_w / 2.0f;

            /* The hitbox just moved, so the remaining cells have to be
               tested against where it is now, not where it was. */
            box = collision_rect(e);
        }
    }
}

/* Same as above, along Y. */
static void resolve_tiles_y(Entity *e, const Map *map, float dy) {
    Rectangle box = collision_rect(e);
    TileRange range = tiles_under(box);

    for (int tile_y = range.first_y; tile_y <= range.last_y; tile_y++) {
        for (int tile_x = range.first_x; tile_x <= range.last_x; tile_x++) {
            if (!map_is_solid(map, tile_x, tile_y)) continue;

            Rectangle tile = tile_rect(tile_x, tile_y);
            if (!CheckCollisionRecs(box, tile)) continue;

            // y is the BOTTOM edge of the hitbox
            if (dy > 0.0f) e->y = tile.y;
            else           e->y = tile.y + TILE_SIZE + e->hb_h;

            box = collision_rect(e);
        }
    }
}

/* ── entity ─────────────────────────────────────────────────────────── */

void entity_update(Entity *e, float dx, float dy, float dt,
                   Entity *entities, int count, const Map *map) {
    Rectangle other;

    // --- X AXIS ---
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

        /* Tiles go last on purpose: the map is immovable, so when an
           entity is squeezed between a pushable entity and a wall, the
           wall is the one that wins. */
        resolve_tiles_x(e, map, dx);
    }

    // --- Y AXIS ---
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

        resolve_tiles_y(e, map, dy);
    }
}

void entity_draw(const Entity *e){
    TextureAsset asset = texture_get(e->texture_id);
    DrawTextureRec(asset.texture,
                   (Rectangle){ 0, 0, asset.frame_w, asset.frame_h },
                   (Vector2){e->x - asset.off_x, e->y - asset.off_y},
                   WHITE);
}