# Rendering design (TTY) (`src/render/`)

## Purpose
Render the game state clearly in a terminal.

## Requirements
- Clearly show:
  - snakes
  - food
  - game status (running/paused/game over)
  - score per player
- Visual separation between background, playfield, snake, and food.
- Consistent colors and readable text.

## Integration
- Use the TTY library design in [design_docs/TTY_graphics.md](TTY_graphics.md).
- Renderer takes a snapshot of the game state and draws it.

## Layout
- Playfield area: fixed rectangle.
- HUD/status area: one or more lines above/below.
- Border: box drawing characters or ASCII.

## Draw order
1. Clear back buffer / paint background.
2. Border + playfield.
3. Food.
4. Snakes (distinct color per player).
5. HUD (status + scores + restart hint).

## No game logic
- Renderer must not mutate `GameState`.
