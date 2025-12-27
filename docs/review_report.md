# Project Review Report: SNAKE

## Style Guide Compliance

### 1. Opaque Pointers
- **Status**: Mostly compliant, but with notable leaks.
- **Findings**:
    - `GameState` and `PlayerState` are forward-declared in `game.h` but their internal structure is leaked via `game_get_state` and subsequent casts in `snakegame.c`.
    - `DisplayContext` in `display.h` is opaque, which is good.
    - `HighScore` and `GameConfig` are opaque.
- **Recommendation**: Replace direct access to `GameState` fields with accessor functions in `game.h`.

### 2. Error Handling (Error-Out Pattern)
- **Status**: Partially compliant.
- **Findings**:
    - The "goto out" pattern is used effectively in `game_init` and `persist.c`.
    - **CRITICAL**: Many return values from system calls (`tcsetattr`, `fcntl`, `clock_gettime`) and project functions (`persist_append_score`, `sprite_add_color`) are ignored.
- **Recommendation**: Audit and fix ignored return values. Use `(void)` only where explicitly safe and documented.

### 3. Naming and Formatting
- **Status**: Compliant.
- **Findings**:
    - Avoids `_t` suffix.
    - Follows `struct typedef name must match tag`.
    - Minimal LLM token usage is prioritized (concise code).

### 4. Dependency Injection (Capability Structs)
- **Status**: Low compliance.
- **Findings**:
    - RNG and IO are globally accessed or tightly coupled.
- **Recommendation**: Introduce capability structs if unit testing of core logic becomes a priority.

## Architectural Review

### 1. Module Boundaries
- The project is well-modularized: `core`, `input`, `persist`, `platform`, `render`.
- `snakegame.c` acts as a coordinator, which is a good pattern.

### 2. Resource Management
- Reverse-order cleanup is generally followed.
- Use of `calloc` and `sizeof(*ptr)` is consistent.

### 3. Coupling
- `snakegame.c` is slightly too aware of `core` internals (e.g., casting `GameState`).
- `render_3d.c` has some direct dependencies that could be abstracted.

## Proposed Improvements
1.  **Seal Opaque Types**: Add missing accessors for `GameState` and `PlayerState`.
2.  **Fix Error Handling**: Address ignored return values identified by `errors.py`.
3.  **Refactor Main Loop**: Reduce logic in `snakegame.c` by delegating more to the `game` module.
