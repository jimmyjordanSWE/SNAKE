#include "snake/collision.h"
#include <stddef.h>
bool collision_is_wall(SnakePoint p, int board_w, int board_h) { return p.x < 0 || p.x >= board_w || p.y < 0 || p.y >= board_h; }
bool collision_is_self(SnakePoint p, const PlayerState* player) {
if(player == NULL || player->length < 2) return false;
for(int i= 1; i < player->length; i++) {
SnakePoint segment= player->body[i];
if(segment.x == p.x && segment.y == p.y) return true;
}
return false;
}
bool collision_is_snake(SnakePoint p, const PlayerState* player) {
if(player == NULL || player->length <= 0) return false;
for(int i= 0; i < player->length; i++) {
SnakePoint segment= player->body[i];
if(segment.x == p.x && segment.y == p.y) return true;
}
return false;
}
SnakePoint collision_next_head(SnakePoint current, SnakeDir dir) {
SnakePoint next= current;
switch(dir) {
case SNAKE_DIR_UP: next.y--; break;
case SNAKE_DIR_DOWN: next.y++; break;
case SNAKE_DIR_LEFT: next.x--; break;
case SNAKE_DIR_RIGHT: next.x++; break;
}
return next;
}
void collision_detect_and_resolve(GameState* game) {
if(game == NULL) return;
SnakePoint next_heads[SNAKE_MAX_PLAYERS];
bool should_reset[SNAKE_MAX_PLAYERS];
for(int i= 0; i < SNAKE_MAX_PLAYERS; i++) {
should_reset[i]= false;
next_heads[i]= (SnakePoint){.x= -1, .y= -1};
}
int num_players= game->num_players;
if(num_players < 0) num_players= 0;
if(num_players > SNAKE_MAX_PLAYERS) num_players= SNAKE_MAX_PLAYERS;
for(int i= 0; i < num_players; i++) {
PlayerState* player= &game->players[i];
if(!player->active || player->length <= 0) continue;
SnakePoint current_head= player->body[0];
next_heads[i]= collision_next_head(current_head, player->current_dir);
}
for(int i= 0; i < num_players; i++) {
PlayerState* player= &game->players[i];
if(!player->active || player->length <= 0) continue;
SnakePoint next_head= next_heads[i];
if(collision_is_wall(next_head, game->width, game->height)) {
should_reset[i]= true;
continue;
}
if(collision_is_self(next_head, player)) {
should_reset[i]= true;
continue;
}
for(int j= 0; j < num_players; j++) {
if(i == j) continue;
PlayerState* other= &game->players[j];
if(!other->active || other->length <= 0) continue;
if(collision_is_snake(next_head, other)) {
should_reset[i]= true;
break;
}
}
}
for(int i= 0; i < num_players; i++) {
PlayerState* a= &game->players[i];
if(!a->active || a->length <= 0) continue;
for(int j= i + 1; j < num_players; j++) {
PlayerState* b= &game->players[j];
if(!b->active || b->length <= 0) continue;
if(next_heads[i].x == next_heads[j].x && next_heads[i].y == next_heads[j].y) {
should_reset[i]= true;
should_reset[j]= true;
}
}
}
for(int i= 0; i < num_players; i++)
if(should_reset[i]) game->players[i].needs_reset= true;
}
