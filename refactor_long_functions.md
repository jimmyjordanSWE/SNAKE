# Refactoring Plan: Long Functions

Based on the analysis of `scripts/out/long_functions_out.txt`, several functions exceed the target complexity and length for maintainable C99 code. This plan details the decomposition strategies for these functions.

## 1. `src/snakegame.c`: `snake_game_run` (429 LOC)

The current implementation mixes initialization, network setup, headless mode, input handling, and the main game loop.

### Decomposition Strategy:
- **`snake_game_init_multiplayer`**: Extract lines 438-459. Handles `mpclient` creation and session joining.
- **`snake_game_run_headless`**: Extract lines 462-521. Clean separation of the non-graphical loop.
- **`snake_game_handle_resize`**: Extract lines 536-562. Handles terminal size validation and blocking wait for resize.
- **`snake_game_process_inputs`**: Extract lines 564-579. Handles polling and enqueuing for all players.
- **`snake_game_handle_highscores`**: Extract lines 584-626 and 627-725. Large block of logic for single and multiplayer death/winner overlays and score persistence.
- **`snake_game_run_frame`**: Extract lines 726-800+. The inner frame loop that handles Delta-T, frequent input polling, and rendering calls.

## 2. `src/render/render_3d.c`: `render_3d_draw` (420 LOC)

This function is a "God function" for the 3D rendering pipeline.

### Decomposition Strategy:
- **`render_3d_draw_debug_textures`**: Extract lines 561-589.
- **`render_3d_setup_floor_decals`**: Extract lines 596-674. Handles decal pooling and bucket distribution.
- **`render_3d_draw_floor_ceiling`**: Extract lines 675-784. The row-major rendering pass for the floor and ceiling.
- **`render_3d_draw_walls`**: Extract lines 789+. The column-major raycasting and wall texture mapping pass.

## 3. `src/persist/persist.c`: `persist_load_config` (317 LOC)

A monolithic parser for the configuration file.

### Decomposition Strategy:
- Use a table-driven approach or helper functions for key categories:
    - **`parse_graphics_config`**: Render glyphs, 3D flags, FOV, scale factors, textures.
    - **`parse_player_config`**: Names, colors, keys.
    - **`parse_multiplayer_config`**: Host, port, identifier, session.
    - **`parse_legacy_keys`**: Support for old single-player key mappings.

## 4. Supporting Refactors

- **`persist_write_config`**: Mirror the `load_config` decomposition for symmetrical writing logic.
- **`render_draw` (2D)**: Split into `render_draw_board`, `render_draw_hud`, and `render_draw_status_messages`.
- **`collision_detect_and_resolve`**: Break into sub-checks for walls, self-collision, and head-to-head.

## Success Criteria
- No function exceeds 100 lines (excluding large table initializations if necessary).
- No change in game behavior or performance.
- Improved unit testability of individual components.
