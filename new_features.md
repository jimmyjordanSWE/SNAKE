IMPORTANT: LLMs never commit to git. IGNORE LEAKS FROM SDL.

# New Features

- Add floor textures
  - Use `assets/floor.png`. Ensure the floor texture connects to walls and does not render below the floor; fix texture mapping/scrolling so it remains visually stable.

- Implement multiplayer (optional)
  - Prototype support via https://github.com/robin-onvo/mpapi

- Default to building with all available CPU cores by setting `NPROC := $(shell nproc)` and adding `MAKEFLAGS += -j$(NPROC)` in the top-level `Makefile` (speeds up builds on multi-core machines).
