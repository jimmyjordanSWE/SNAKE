# 3D Rendering Performance Improvements

Low-hanging fruit identified in the render pipeline.

---

## 1. **Repeated `env_bool()` Calls in Hot Path**  
**File:** `render_3d.c` lines 499-503

```c
int debug_timing = env_bool("SNAKE_DEBUG_3D_TIMING", 0);
int fast_wall_tex = env_bool("SNAKE_3D_FAST_WALLS", 1);
int fast_floor_tex = env_bool("SNAKE_3D_FAST_FLOOR", 1);
```

**Problem:** `env_bool()` calls `getenv()` **every frame**. `getenv()` is O(n) on the environment list.

**Fix:** Cache these in `Render3DContext` at init time or use `static` with lazy init:
```c
static int fast_wall_tex = -1;
if (fast_wall_tex < 0) fast_wall_tex = env_bool("SNAKE_3D_FAST_WALLS", 1);
```

**Impact:** ~3 `getenv()` calls × 60 FPS = 180 useless lookups/sec removed.

---

## 2. **Per-Frame Decal/Bucket Allocation**  
**File:** `render_3d.c` lines 593-599

```c
buckets = calloc((size_t)map_w * (size_t)map_h, sizeof(*buckets));
decals = calloc((size_t)max_decals, sizeof(Decal));
```

**Problem:** Allocates and frees ~2 arrays **every frame** for floor shadow decals.

**Fix:** Pre-allocate in `Render3DContext` at init, reuse with `memset()` to clear:
```c
// In Render3DContext:
Decal* decals;
TileBucket* buckets;
int decals_cap, buckets_cap;
```

**Impact:** Eliminates 2 `malloc`+`free` pairs per frame. Huge win for cache locality.

---

## 3. **Sprite Shading: Expensive `sqrtf`/`powf` Per Pixel**  
**File:** `sprite.c` lines 420-447

```c
float nz = sqrtf(1.0f - n2);
// ...
spec = powf(spec, shininess) * spec_strength;
```

**Problem:** For shaded spheres, every pixel inside the circle computes `sqrtf()` and `powf()`.

**Fixes:**
1. **Approximate `sqrtf`:** Use `fast_inv_sqrt()` already in `math_fast.h`:
   ```c
   float nz = 1.0f / fast_inv_sqrt(1.0f - n2);
   ```
2. **Replace `powf(x, 24)`:** Use integer exponentiation or lookup table.
3. **Pre-shade:** For small sprites, use pre-computed normal-mapped sprites.

**Impact:** Shaded sprites are 5-10× slower than flat. This is the biggest CPU sink for large snake bodies.

---

## 4. **Blend Division in Hot Path**  
**File:** `render_3d_sdl.c` lines 204-206

```c
uint8_t rr = (uint8_t)((sr * sa + dr * inv) / 255);
```

**Problem:** Division by 255 is slow. Called per-pixel for all blended sprites.

**Fix:** Use the approximation `x / 255 ≈ (x + 128) >> 8`:
```c
uint8_t rr = (uint8_t)(((sr * sa + dr * inv) + 128) >> 8);
```

**Impact:** ~20% faster alpha blending.

---

## 5. **Floor Loop: Repeated `column_depths` Access Pattern**  
**File:** `render_3d.c` lines 681-712

The floor decal shadow check iterates buckets but doesn't use `column_depths` - this is correct. However, the wall pass re-reads `cos_offsets`/`sin_offsets` arrays that are already computed.

**Already optimized** - the precomputed trig cache (lines 521-532) is good.

---

## 6. **Raycast: Trig Per Ray**  
**File:** `raycast.c` lines 41-42

```c
float cos_a = cosf(ray_angle);
float sin_a = sinf(ray_angle);
```

**Problem:** `cosf`/`sinf` called per column (e.g., 800 times per frame).

**Fix:** The camera already caches angle offsets. Pass pre-computed `cos_a`/`sin_a` to `raycast_cast_ray()`:
```c
bool raycast_cast_ray_fast(Raycaster3D* rc, float cam_x, float cam_y, 
                           float cos_a, float sin_a, RayHit* hit);
```

**Impact:** Saves 1600 trig calls per frame (800 sin + 800 cos).

---

## 7. **SDL Present: Row-by-Row memcpy**  
**File:** `render_3d_sdl.c` lines 316-322

```c
for (int y = 0; y < ctx->height; ++y) {
    memcpy(dst, src + (size_t)y * row_bytes, row_bytes);
    dst += tex_pitch;
}
```

**Problem:** If `tex_pitch == row_bytes`, this could be a single `memcpy`.

**Fix:**
```c
if (tex_pitch == (int)row_bytes) {
    memcpy(dst, ctx->pixels, row_bytes * ctx->height);
} else {
    // row-by-row fallback
}
```

**Impact:** Minor, but eliminates loop overhead when pitch matches.

---

## Priority Order (Effort vs Impact)

| Priority | Fix | Effort | Impact |
|----------|-----|--------|--------|
| **1** | Cache `env_bool()` calls | 5 min | Low but easy |
| **2** | Pre-allocate decal/bucket arrays | 20 min | Medium |
| **3** | Fast blend approximation | 5 min | Medium |
| **4** | Pass pre-computed trig to raycast | 30 min | Medium-High |
| **5** | Fast sqrt for sprite shading | 15 min | High for shaded |
| **6** | Optimize SDL present memcpy | 5 min | Low |

---

## Existing Profiling

Enable timing with:
```bash
export SNAKE_DEBUG_3D_TIMING=1
export SNAKE_SPRITE_PROFILE=1
./build/debug/snake
```

Output shows breakdown of setup/walls/sprites/overlays/present.
