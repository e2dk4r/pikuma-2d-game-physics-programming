#include "renderer.h"
#include "SDL3/SDL.h"

void
GameRendererInit(game_renderer *gameRenderer, s32 windowWidth, s32 windowHeight)
{
  gameRenderer->screenCenter = (v2){(f32)windowWidth * 0.5f, (f32)windowHeight * 0.5f};
}

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
DrawCircle(game_renderer *gameRenderer, v2 position, f32 radius, v4 color, f32 width)
{
  SDL_Renderer *renderer = gameRenderer->renderer;
  SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a);
}
