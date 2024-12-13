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

void
RenderFrame(game_renderer *gameRenderer)
{
  SDL_RenderPresent(gameRenderer->renderer);
}

void
ClearScreen(game_renderer *gameRenderer, v4 color)
{
  SDL_Renderer *renderer = gameRenderer->renderer;
  SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a);
  SDL_RenderClear(renderer);
}

void
DrawLine(game_renderer *gameRenderer, v2 point1, v2 point2, v4 color, f32 width)
{
  SDL_Renderer *renderer = gameRenderer->renderer;
  SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a);

  v2 point1InScreenSpace = ToScreenSpace(gameRenderer, point1);
  v2 point2InScreenSpace = ToScreenSpace(gameRenderer, point2);
  f32 widthInPixel = width * PIXELS_PER_METER;
  SDL_RenderLine(renderer, point1InScreenSpace.x, point1InScreenSpace.y, point2InScreenSpace.x, point2InScreenSpace.y);
}

void
DrawCircle(game_renderer *gameRenderer, v2 position, f32 radius, v4 color)
{
  // TODO: radius <= 0.2f causes artifacts
  // TODO: PERFORMANCE: Lower API calls to one
  SDL_Renderer *renderer = gameRenderer->renderer;
  SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a);

  f32 radiusInPixels = radius * PIXELS_PER_METER;
  v2 positionInScreenSpace = ToScreenSpace(gameRenderer, position);
  v2 offset = {0.0f, radiusInPixels};
  f32 d = (radius - 1) * PIXELS_PER_METER;

  while (offset.y >= offset.x) {
    SDL_FPoint points[] = {
        {positionInScreenSpace.x - offset.y, positionInScreenSpace.y + offset.x},
        {positionInScreenSpace.x + offset.y, positionInScreenSpace.y + offset.x},
        {positionInScreenSpace.x - offset.x, positionInScreenSpace.y + offset.y},
        {positionInScreenSpace.x + offset.x, positionInScreenSpace.y + offset.y},
        {positionInScreenSpace.x - offset.x, positionInScreenSpace.y - offset.y},
        {positionInScreenSpace.x + offset.x, positionInScreenSpace.y - offset.y},
        {positionInScreenSpace.x - offset.y, positionInScreenSpace.y - offset.x},
        {positionInScreenSpace.x + offset.y, positionInScreenSpace.y - offset.x},
    };
    if (!SDL_RenderLines(renderer, points, ARRAY_COUNT(points)))
      break;

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
}

void
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

rect
RendererGetSurfaceRect(game_renderer *renderer)
{
  v2 screenCenterInMeters = v2_scale(renderer->screenCenter, METERS_PER_PIXEL);
  return (rect){
      .min = v2_neg(screenCenterInMeters), // left bottom
      .max = screenCenterInMeters,         // right top
  };
}
