# Changelog

## 2025-12-16 - Feature updates
- Adjust play field ratio, size & speed: finalized changes to support canonical playfield proportions and tick rate tuning (unit tests added).
- Make the minimap larger: increased minimap target size and minimum pixel-per-cell for better readability; updated unit tests.
- Fix: draw snake head after body segments in minimap to avoid head flicker when body overlaps (unit test added).
- Update: removed blocking welcome/name input at startup (default player name `You` used unless a high score entry is required).

