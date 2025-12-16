# FPS Counter Implementation

## Overview
Added a real-time FPS counter to the 3D rendering view, displayed in the top-left corner with green text.

## Implementation Details

### Changes to Render3DContext Structure
Added three new fields to track frame timing:
```c
float             frame_times[60];      /* Circular buffer of last 60 frame times */
int               frame_time_idx;       /* Current index in circular buffer */
float             current_fps;          /* Calculated FPS (updated each frame) */
```

### New Functions

#### `render_3d_update_fps(float delta_seconds)`
- Called once per frame with the current frame delta time
- Maintains a circular buffer of the last 60 frame times
- Calculates average frame time from the buffer
- Computes FPS as 1.0 / average_frame_time
- Smooths out frame-to-frame variance with 60-frame averaging

#### `render_3d_draw_fps_counter(void)`
- Draws "FPS: XX.X" in the top-left corner (4 pixels from edges)
- Uses green text (0, 255, 0) for visibility
- Renders one character at a time using the existing `render_3d_draw_char()` function
- Properly formats the number with decimal place
- Handles 1-3 digit FPS values (e.g., "5.2", "60.0", "120.5")

### Rendering Pipeline Integration
The FPS counter is drawn in the proper order:
```
Phase 1: Raycast columns
Phase 2: Collect sprites
Phase 3: Project and render sprites
Phase 4: Draw minimap and FPS counter
Phase 5: Present frame to screen
```

This ensures the FPS counter appears on top of all game elements.

## Features

- **60-frame moving average**: Smooths out individual frame spikes for readable FPS
- **Real-time updates**: Calculated and displayed every frame
- **Accurate formatting**: Shows one decimal place (e.g., "59.8", "60.0")
- **Green text**: High-contrast color for easy visibility
- **Minimal overhead**: Only draws 8 characters per frame

## Display Location
- **Position**: Top-left corner (x=4, y=4 pixels from edge)
- **Size**: ~50 pixels wide × 7 pixels tall (scaled with font size)
- **Color**: Green (RGB: 0, 255, 0)
- **Format**: "FPS: XX.X"

## Usage
The FPS counter is automatically displayed whenever render_3d_draw() is called. No additional configuration needed.

## Files Modified
- `/home/jimmy/SNAKE/src/render/render_3d.c`
  - Added FPS tracking fields to Render3DContext
  - Added forward declaration for render_3d_draw_char()
  - Added render_3d_update_fps() function
  - Added render_3d_draw_fps_counter() function
  - Called render_3d_update_fps() in render_3d_draw()
  - Called render_3d_draw_fps_counter() before screen presentation

## Compilation Status
✅ All changes compile successfully with no errors or warnings
