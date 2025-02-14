#pragma once

#include "math.h"
#include "memory.h"
#include "type.h"

#if IS_BUILD_DEBUG
#include "string_builder.h"
#endif

#include "physics.h"
#include "platform.h"
#include "random.h"
#include "renderer.h"

typedef struct {
  b8 isInitialized : 1;

  memory_arena worldArena;

  random_series effectsEntropy;
  entity *entities;
  u32 entityCount;
  u32 entityMax;

  volume *smallCircleVolume;

  f32 time; // unit: sec
} game_state;

typedef struct {
  b8 isInitialized : 1;
  memory_arena transientArena;
  string_builder *sb;
} transient_state;

typedef void (*pfnGameUpdateAndRender)(game_memory *memory, game_input *input, game_renderer *renderer);
#if IS_PLATFORM_WINDOWS
__declspec(dllexport)
#endif
void
GameUpdateAndRender(game_memory *memory, game_input *input, game_renderer *renderer);
