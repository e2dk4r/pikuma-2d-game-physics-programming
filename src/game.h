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

typedef struct collision_map {
  u32 a;
  u32 b;
  // TODO: collision_map *next;
} collision_map;

typedef struct {
  b8 isInitialized : 1;

  memory_arena worldArena;

  random_series effectsEntropy;
  entity *entities;
  u32 entityCount;
  u32 entityMax;

  collision_map collisionMap;
  // TODO: collision_map collisionMapFree;

  f32 time; // unit: sec
} game_state;

typedef struct {
  b8 isInitialized : 1;
  memory_arena transientArena;
  string_builder *sb;
} transient_state;

typedef void (*pfnGameUpdateAndRender)(game_memory *memory, game_input *input, game_renderer *renderer);
void
GameUpdateAndRender(game_memory *memory, game_input *input, game_renderer *renderer);
