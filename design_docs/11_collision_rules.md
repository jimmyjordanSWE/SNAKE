# Collision rules (deterministic + reproducible)

## Required collisions
- Snake vs wall
- Snake vs itself
- Snake vs snake

## Definitions
- A snake occupies the set of cells covered by its body segments.
- “Head” is the leading segment.

## Deterministic collision resolution (recommended)

### Wall / self
If a head’s next cell is outside the board, or equals a cell in its own body (after applying classic movement rules), the snake is considered dead and is reset immediately.

### Snake vs snake
Define “kör in” as:
- A snake **loses** if its head moves into a cell that is occupied by the other snake at the time of collision evaluation.

To keep this reproducible with multiple snakes moving simultaneously:
1. Compute all next-head positions.
2. Evaluate collisions against the occupancy state from the start of the tick (pre-move), plus explicit head-to-head rules.

### Head-to-head special case
If two heads move into the same empty cell on the same tick:
- Pick one rule and document it. Options:
  - both reset (simple + symmetric)
  - lower player id wins (deterministic)

## Consistency
- Keep the rule identical in local multiplayer and in network mode.
- Ensure “collision evaluation order” is documented and stable.
