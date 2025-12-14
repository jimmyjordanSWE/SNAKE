#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct {
float world_x, world_y;
float screen_x, screen_y;
float screen_z;
float screen_width, screen_height;
uint8_t entity_type, entity_id;
bool is_visible;
} SpriteProjection;
typedef struct {
SpriteProjection* sprites;
int max_sprites, sprite_count;
const void* camera;
} SpriteRenderer3D;
void sprite_init(SpriteRenderer3D* sr, int max_sprites, const void* camera);
void sprite_clear(SpriteRenderer3D* sr);
bool sprite_add(SpriteRenderer3D* sr, float world_x, float world_y, uint8_t entity_type, uint8_t entity_id);
void sprite_sort_by_depth(SpriteRenderer3D* sr);
const SpriteProjection* sprite_get(const SpriteRenderer3D* sr, int idx);
int sprite_get_count(const SpriteRenderer3D* sr);
void sprite_shutdown(SpriteRenderer3D* sr);