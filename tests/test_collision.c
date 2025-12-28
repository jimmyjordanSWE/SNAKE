/* Legacy test moved to Unity: tests/unity/test_collision.c */
#include <stdio.h>
int main(void) { fprintf(stderr, "Deprecated: use tests/unity/test_collision.c\n"); return 77; }

    /* collision_is_self and detect and resolve scenario */
    struct GameState g;
    memset(&g, 0, sizeof(g));
    g.width = 5; g.height = 5; g.num_players = 2; g.max_players = 2; g.max_length = 4;
    /* allocate minimal player arrays */
    g.players = calloc((size_t)g.max_players, sizeof(struct PlayerState));
    for (int i = 0; i < g.max_players; i++) {
        g.players[i].max_length = g.max_length;
        g.players[i].body = calloc((size_t)g.max_length, sizeof(SnakePoint));
    }
    /* place two players head-on to collide */
    g.players[0].active = true; g.players[0].length = 1;
    g.players[0].body[0] = (SnakePoint){2,2}; g.players[0].current_dir = SNAKE_DIR_RIGHT;
    g.players[1].active = true; g.players[1].length = 1;
    g.players[1].body[0] = (SnakePoint){3,2}; g.players[1].current_dir = SNAKE_DIR_LEFT;
    collision_detect_and_resolve(&g);
    assert(g.players[0].needs_reset && g.players[1].needs_reset);

    /* cleanup */
    for (int i = 0; i < g.max_players; i++) {
        free(g.players[i].body);
    }
    free(g.players);

    printf("test_collision: OK\n");
    return 0;
}