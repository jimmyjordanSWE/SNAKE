# impl_003: Collision Detection

**Header:** `include/snake/collision.h`  
**Source:** `src/core/collision.c`  
**Status:** ✅ Complete

---

## Overview

Collision detection primitives used by game.c. Decoupled for testability and unit test support.

See `collision.h` for function signatures (no implementation details in docs).

## Collision Types

**Wall collision:** Head outside board bounds → death

**Self collision:** Head touches own body (after growth) → death

**Food collision:** Head on food → grow, score++, spawn new food

**Snake-vs-snake:** Head touches opponent → both die (or rule-based interaction)

---

## Design Notes

**Ring buffer:** PlayerState.body[] wraps around; collision functions account for wraparound without reordering.

**Edge case:** When snake grows, new head is added but tail not yet removed; collision check must exclude just-added head.

**Symmetry:** Multi-player collisions checked independently; A hitting B is determined separately from B hitting A.

**Pure functions:** No side effects; safe for unit testing.
