IMPORTANT: LLMs never commit to git. IGNORE LEAKS FROM SDL.

# New Features

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

- **Remove welcome and name input screen** — Not started
 - **Remove welcome and name input screen** ✅ *In progress (implemented startup default + highscore prompt)*
  - **Status:** Implemented non-blocking startup (default player name `You`) and added a highscore name prompt (max 8 chars) which is shown only when the session score qualifies for the high score list.
  - **Tests:** Added unit test to verify the default player name; integration tests for interactive prompt and 3D console rendering remain.
  - **Next steps:** Add integration tests to simulate highscore entry, and render the highscore prompt in the 3D view.

- **Add floor textures** — Not started
  - Will integrate `assets/floor.png` and add rendering tests for texture mapping.

- **Add console screens to 3D view** — Not started
  - Plan to reuse existing console text assets and render them in 3D overlay; add related tests.

- **Make the minimap larger** ✅ *Completed (2025-12-16)*
  - **Status:** Implemented — increased minimap size and tightened unit tests to ensure better readability at canonical playfield sizes.
  - **Bugfix:** Ensure the snake head is drawn after body segments to prevent a gray body segment overlay that caused blinking in the minimap.

- **Smooth snake rendering & head visibility** ✅ *Completed (2025-12-16)*
  - **Status:** Implemented — per-segment previous positions are recorded on tick and body segments are interpolated during render (applies to the minimap and 3D sprite rendering), eliminating hopping and making movement smooth.
  - **Bugfix:** Fixed situations where a body segment could draw over the head by adding ordering heuristics and a small depth nudge in sprite sorting; unit tests were added/adjusted and the test suite was run successfully.
  - **Next steps:** Add visual/integration tests that render a few frames and validate pixel-level expectations; optionally tighten the head-priority rule if needed.

- **Implement multiplayer** — Not started (optional/prototype)


If you'd like, I can also create a brief entry in `CHANGELOG.md` when each feature is completed.
