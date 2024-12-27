#pragma once

/*
 * 0,0 is the center of surface
 * positive x is right, negative is left
 * positive y is up, negative is down
 * angle is in radians until otherwise said
 */

// TODO: Scale and rotate by using specified coordinate system.
//       origin, xAxis, yAxis

#include "math.h"
#include "memory.h"
#include <SDL3/SDL.h>

typedef struct {
  SDL_Renderer *renderer;
  memory_arena memory;
  v2 screenCenter;
} game_renderer;

#define PIXELS_PER_METER 60
#define METERS_PER_PIXEL (1.0f / PIXELS_PER_METER)

static void
RenderFrame(game_renderer *renderer);

static void
ClearScreen(game_renderer *renderer, v4 color);

static void
DrawLine(game_renderer *renderer, v2 from, v2 to, v4 color, f32 width);

static void
DrawCircle(game_renderer *renderer, v2 position, f32 radius, f32 angle, v4 color);

static void
DrawRect(game_renderer *renderer, rect rect, v4 color);

static void
DrawRectRotated(game_renderer *renderer, rect rect, f32 rotation, v4 color);

static void
DrawCrosshair(game_renderer *renderer, v2 position, f32 dim, v4 color);

static rect
RendererGetSurfaceRect(game_renderer *renderer);
