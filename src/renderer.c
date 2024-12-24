#include "renderer.h"

static inline v2
ToScreenSpace(game_renderer *gameRenderer, v2 point)
{
  v2 xAxis = {1.0f, 0.0f};
  v2 yAxis = v2_perp(xAxis);
  v2 origin = gameRenderer->screenCenter;

  v2 pointInScreenCoordinate = {
      v2_dot((v2){point.x, 0.0f}, xAxis),
      v2_dot((v2){0.0f, -point.y}, yAxis),
  };
  return v2_add(origin, v2_scale(pointInScreenCoordinate, PIXELS_PER_METER));
}

static void
RenderFrame(game_renderer *gameRenderer)
{
  SDL_RenderPresent(gameRenderer->renderer);
}

static void
ClearScreen(game_renderer *gameRenderer, v4 color)
{
  SDL_Renderer *renderer = gameRenderer->renderer;
  SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a);
  SDL_RenderClear(renderer);
}

static void
DrawLine(game_renderer *gameRenderer, v2 point1, v2 point2, v4 color, f32 width)
{
  SDL_Renderer *renderer = gameRenderer->renderer;
  SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a);

  v2 point1InScreenSpace = ToScreenSpace(gameRenderer, point1);
  v2 point2InScreenSpace = ToScreenSpace(gameRenderer, point2);
  f32 widthInPixel = width * PIXELS_PER_METER;
  SDL_RenderLine(renderer, point1InScreenSpace.x, point1InScreenSpace.y, point2InScreenSpace.x, point2InScreenSpace.y);
}

static void
DrawCircle(game_renderer *gameRenderer, v2 position, f32 radius, f32 angle, v4 color)
{
  __cleanup_memory_temp__ memory_temp memory = MemoryTempBegin(&gameRenderer->memory);

  u32 pointMax = 4096; // TODO: find maxiumum points needed from radius
  u32 pointCount = 0;
  SDL_FPoint *points = MemoryArenaPush(memory.arena, sizeof(*points) * pointMax, 4);

#if 0
  // TODO: radius <= 0.2f causes artifacts
  f32 radiusInPixels = radius * PIXELS_PER_METER;
  v2 positionInScreenSpace = ToScreenSpace(gameRenderer, position);
  v2 offset = {0.0f, radiusInPixels};
  f32 d = (radius - 1) * PIXELS_PER_METER;

  while (offset.y >= offset.x) {
    SDL_FPoint p[] = {
        {positionInScreenSpace.x - offset.y, positionInScreenSpace.y + offset.x},
        {positionInScreenSpace.x + offset.y, positionInScreenSpace.y + offset.x},
        {positionInScreenSpace.x - offset.x, positionInScreenSpace.y + offset.y},
        {positionInScreenSpace.x + offset.x, positionInScreenSpace.y + offset.y},
        {positionInScreenSpace.x - offset.x, positionInScreenSpace.y - offset.y},
        {positionInScreenSpace.x + offset.x, positionInScreenSpace.y - offset.y},
        {positionInScreenSpace.x - offset.y, positionInScreenSpace.y - offset.x},
        {positionInScreenSpace.x + offset.y, positionInScreenSpace.y - offset.x},
    };
    memcpy(points + pointCount, p, ARRAY_COUNT(p) * sizeof(*p));
    pointCount += ARRAY_COUNT(p);
    debug_assert(pointCount <= pointMax);

    if (d >= 2.0f * offset.x) {
      d -= 2.0f * offset.x + 1.0f;
      offset.x += 1;
    } else if (d < 2.0f * (radiusInPixels - offset.y)) {
      d += 2.0f * offset.y - 1.0f;
      offset.y -= 1;
    } else {
      d += 2.0f * (offset.y - offset.x - 1.0f);
      offset.y -= 1;
      offset.x += 1;
    }
  }
#else
  f32 radiusInPixels = radius * PIXELS_PER_METER;
  v2 positionInScreenSpace = ToScreenSpace(gameRenderer, position);

  // draw line to show the angle
  v2 angleLine[] = {
      // start point
      {positionInScreenSpace.x, positionInScreenSpace.y},
      // end point
      {positionInScreenSpace.x + radiusInPixels * Cos(angle), positionInScreenSpace.y - radiusInPixels * Sin(angle)},
  };

  // see:
  // - http://members.chello.at/~easyfilter/Bresenham.pdf
  // - https://www.youtube.com/watch?v=CceepU1vIKo "NoBS Code - Bresenham's Line Algorithm - Demystified Step by Step"
  // - https://www.youtube.com/watch?v=y_SPO_b-WXk "UofM Introduction to Computer Graphics - COMP 3490 - (Unit 3)
  // Drawing Primitives 2: Bresenham's Line Algorithm"
  // - https://www.youtube.com/watch?v=hpiILbMkF9w "NoBS Code - The Midpoint Circle Algorithm Explained Step by Step"

  f32 x = -radiusInPixels, y = 0.0f, err = 2.0f - 2.0f * radiusInPixels; // bottom left to top right
  do {
    SDL_FPoint p[] = {
        {positionInScreenSpace.x - x, positionInScreenSpace.y + y}, /*   I. Quadrant +x +y */
        {positionInScreenSpace.x - y, positionInScreenSpace.y - x}, /*  II. Quadrant -x +y */
        {positionInScreenSpace.x + x, positionInScreenSpace.y - y}, /* III. Quadrant -x -y */
        {positionInScreenSpace.x + y, positionInScreenSpace.y + x}, /*  IV. Quadrant +x -y */
    };
    memcpy(points + pointCount, p, ARRAY_COUNT(p) * sizeof(*p));
    pointCount += ARRAY_COUNT(p);
    debug_assert(pointCount <= pointMax);

    radiusInPixels = err;
    if (radiusInPixels <= y)
      err += ++y * 2.0f + 1.0f;        /* e_xy+e_y < 0 */
    if (radiusInPixels > x || err > y) /* e_xy+e_x > 0 or no 2nd y-step */
      err += ++x * 2.0f + 1.0f;        /* -> x-step now */
    radiusInPixels = err;
  } while (x < 0.0f);
#endif
  debug_assert(pointCount != 0);

  SDL_Renderer *renderer = gameRenderer->renderer;
  SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a);
  SDL_RenderPoints(renderer, points, (s32)pointCount);
  SDL_RenderLine(renderer,
                 // start point
                 angleLine[0].x, angleLine[0].y,
                 // end point
                 angleLine[1].x, angleLine[1].y);
}

static void
DrawRect(game_renderer *gameRenderer, rect rect, v4 color)
{
  debug_assert(rect.min.x != rect.max.x && rect.min.y != rect.max.y && "invalid rect");

  v2 leftBottom = ToScreenSpace(gameRenderer, rect.min);
  v2 dim = v2_scale(RectGetDim(rect), PIXELS_PER_METER);
  v2 leftTop = v2_add(leftBottom, (v2){0.0f, -dim.y});

  SDL_FRect sdlRect = {
      .x = leftTop.x,
      .y = leftTop.y,
      .w = dim.x,
      .h = dim.y,
  };

  SDL_Renderer *renderer = gameRenderer->renderer;
  SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a);
  SDL_RenderFillRect(renderer, &sdlRect);
}

static void
DrawCrosshair(game_renderer *gameRenderer, v2 position, f32 dim, v4 color)
{
  f32 dimInPixels = dim * 0.5f * PIXELS_PER_METER;
  f32 radiusInPixels = dimInPixels * 0.5f;
  v2 center = ToScreenSpace(gameRenderer, position);

  SDL_FRect rects[] = {
      {.x = center.x - radiusInPixels, .w = dimInPixels, .y = center.y - 0.5f, .h = 1.0f},
      {.x = center.x - 0.5f, .w = 1.0f, .y = center.y - radiusInPixels, .h = dimInPixels},
  };
  SDL_Renderer *renderer = gameRenderer->renderer;
  SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a);
  SDL_RenderFillRects(renderer, rects, ARRAY_COUNT(rects));
}

static rect
RendererGetSurfaceRect(game_renderer *renderer)
{
  v2 screenCenterInMeters = v2_scale(renderer->screenCenter, METERS_PER_PIXEL);
  return (rect){
      .min = v2_neg(screenCenterInMeters), // left bottom
      .max = screenCenterInMeters,         // right top
  };
}
