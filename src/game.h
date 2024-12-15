#pragma once

#include "math.h"
#include "memory.h"
#include "type.h"

#if IS_BUILD_DEBUG
#include "string_builder.h"
#endif

#include "platform.h"
#include "random.h"
#include "renderer.h"

typedef struct particle {
  v2 position;     // unit: m
  v2 velocity;     // unit: m/s
  v2 acceleration; // unit: m/s²
  f32 mass;        // unit: kg
  f32 invMass;     // computed from 1/mass
} particle;

typedef struct {
  b8 isInitialized : 1;

  memory_arena worldArena;

  random_series effectsEntropy;
  particle *particles;
  u32 particleCount;

  rect liquid;
} game_state;

typedef struct {
  b8 isInitialized : 1;
  memory_arena transientArena;
  string_builder *sb;
} transient_state;

typedef void (*pfnGameUpdateAndRender)(game_memory *memory, game_input *input, game_renderer *renderer);
void
GameUpdateAndRender(game_memory *memory, game_input *input, game_renderer *renderer);
