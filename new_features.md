IMPORTANT: LLMs never commit to git. 

# New Features

- Adjust play field ratio, size & speed
  - Change the default playfield aspect ratio and size to canonical Snake proportions. Adjust the default game speed / tick rate.

- Remove welcome and name input screen
  - Start the game immediately with default player name `You`. Only prompt for name if the player makes the highscore list (max name length 8 + NUL).

- Add floor textures
  - Use `assets/floor.png`. Ensure the floor texture connects to walls and does not render below the floor; fix texture mapping/scrolling so it remains visually stable.

- Add console screens to 3D view
  - Render the "you died" and "congratulations / input name" screens in the 3D view.

- Make the minimap larger
  - Increase minimap size and adjust scale for readability at the canonical playfield size.

- Implement multiplayer (optional)
  - Prototype support via https://github.com/robin-onvo/mpapi

---

## Progress (2025-12-16)

- **Adjust play field ratio, size & speed** ✅ *In progress (started 2025-12-16)*
  - **Status:** Partially implemented — default board size and tick rate updated; minimap scaling adjusted to better support larger canonical fields.
  - **Planned changes:** Update remaining UI scaling and tweak tick tuning if playtesting suggests further adjustments.
  - **Tests:** Added unit tests for minimap scaling and validated configuration defaults via existing tests. Integration tests for gameplay at new sizes/speeds remain to be added.
  - **Next steps:** Add integration tests under `tests/integration` and any gameplay tests under `tests/game`, run full test suite and fix regressions, then update `CHANGELOG.md` when finished.

- **Remove welcome and name input screen** — Not started
 - **Remove welcome and name input screen** ✅ *In progress (implemented startup default + highscore prompt)*
  - **Status:** Implemented non-blocking startup (default player name `You`) and added a highscore name prompt (max 8 chars) which is shown only when the session score qualifies for the high score list.
  - **Tests:** Added unit test to verify the default player name; integration tests for interactive prompt and 3D console rendering remain.
  - **Next steps:** Add integration tests to simulate highscore entry, and render the highscore prompt in the 3D view.

- **Add floor textures** — Not started
  - Will integrate `assets/floor.png` and add rendering tests for texture mapping.

- **Add console screens to 3D view** — Not started
  - Plan to reuse existing console text assets and render them in 3D overlay; add related tests.

- **Make the minimap larger** — Not started
  - Adjust minimap rendering and add tests to ensure correct scaling.

- **Implement multiplayer** — Not started (optional/prototype)


If you'd like, I can also create a brief entry in `CHANGELOG.md` when each feature is completed.
