#include "game.h"
#include "color.h"
#include "math.h"
#include "renderer.h"
#include "string_builder.h"

#include "physics.c"
#include "random.c"
#include "renderer.c"

#if IS_BUILD_DEBUG
#include <unistd.h> // write()
#endif

void
GameUpdateAndRender(game_memory *memory, game_input *input, game_renderer *renderer)
{
  game_state *state = memory->permanentStorage;
  debug_assert(memory->permanentStorageSize >= sizeof(*state));

  /*****************************************************************
   * PERMANENT STORAGE INITIALIZATION
   *****************************************************************/
  if (!state->isInitialized) {
    // memory
    state->worldArena = (memory_arena){
        .total = memory->permanentStorageSize - sizeof(*state),
        .block = memory->permanentStorage + sizeof(*state),
    };
    memory_arena *worldArena = &state->worldArena;

    state->effectsEntropy = RandomSeed(1);
    random_series *effectsEntropy = &state->effectsEntropy;

    state->particleMax = 4;
    state->particleCount = 4;
    state->particles = MemoryArenaPush(worldArena, sizeof(*state->particles) * state->particleMax, 4);
    for (u32 particleIndex = 0; particleIndex < state->particleCount; particleIndex++) {
      struct particle *particle = state->particles + particleIndex;
      *particle = (struct particle){
          .position =
              {
                  .x = 0,
                  .y = 4.0f - (f32)particleIndex * 1.0f,
              },
          // .mass = RandomBetween(effectsEntropy, 1.0f, 5.0f),
          .mass = 2.0f,
      };
      particle->invMass = 1.0f / particle->mass;
    }
    state->springAnchorPosition = (v2){0.0f, 5.0f};

    rect surfaceRect = RendererGetSurfaceRect(renderer);
    state->liquid = (rect){
        .min = surfaceRect.min,
        .max = {surfaceRect.max.x, 0.0f},
    };

    state->isInitialized = 1;
  }

  /*****************************************************************
   * TRANSIENT STORAGE INITIALIZATION
   *****************************************************************/
  transient_state *transientState = memory->transientStorage;
  debug_assert(memory->transientStorageSize >= sizeof(*transientState));
  if (!transientState->isInitialized) {
    transientState->transientArena = (memory_arena){
        .total = memory->transientStorageSize - sizeof(*transientState),
        .block = memory->transientStorage + sizeof(*transientState),
    };

    transientState->isInitialized = 1;
  }

  string_builder *sb = transientState->sb;

  /*****************************************************************
   * TIME
   *****************************************************************/
  f32 dt = input->dt;
  debug_assert(dt > 0);
  state->time += dt;
#if (0 && IS_BUILD_DEBUG)
  {
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("dt: "));
    StringBuilderAppendF32(sb, dt, 4);
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
    string string = StringBuilderFlush(sb);
    write(STDOUT_FILENO, string.value, string.length);
  }
#endif

  /*****************************************************************
   * INPUT HANDLING
   *****************************************************************/
  global b8 impulse = 0;
  v2 mousePosition = {};
  v2 inputForce = {};
  for (u32 controllerIndex = 0; controllerIndex < ARRAY_COUNT(input->controllers); controllerIndex++) {
    game_controller *controller = input->controllers + controllerIndex;

    v2 input = {controller->lsX, controller->lsY};
    if (v2_length_square(input) > 1.0f) {
      v2_normalize_ref(&input);
      // NOTE: disabled assertion because of floating point error
      // debug_assert(v2_length_square(input) == 1.0f);
    }
    v2_add_ref(&inputForce, input);

    if (controllerIndex == GAME_CONTROLLER_KEYBOARD_AND_MOUSE_INDEX) {
      v2 surfaceHalfDim = RectGetHalfDim(RendererGetSurfaceRect(renderer));
      mousePosition = v2_hadamard((v2){controller->rsX, controller->rsY}, // [-1.0, 1.0]
                                  surfaceHalfDim);

      if (controller->lb) {
        impulse = 1;
      }

      if (impulse && !controller->lb) {
        impulse = 0;

        particle *lastParticle = state->particles + state->particleCount - 1;
        v2 diff = v2_sub(lastParticle->position, mousePosition);
        f32 impulseMagnitude = v2_length(diff) * 5.0f;
        v2 impulseDirection = v2_normalize(diff);
        v2 impulseVector = v2_scale(impulseDirection, impulseMagnitude);
        lastParticle->velocity = impulseVector;

#if (1 && IS_BUILD_DEBUG)
        {
          StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("impulse: "));
          StringBuilderAppendF32(sb, impulseVector.x, 2);
          StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(","));
          StringBuilderAppendF32(sb, impulseVector.y, 2);
          StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
          string string = StringBuilderFlush(sb);
          write(STDOUT_FILENO, string.value, string.length);
        }
#endif
      }
#if 0
      static f32 lbPressedAt = 0.0f;
      if (controller->lb) {
        if (lbPressedAt == 0.0f && state->particleCount != state->particleMax) {
          u32 particleIndex = state->particleCount;
          struct particle *particle = state->particles + particleIndex;
          particle->position = mousePosition;
          particle->mass = 1.0f;
          particle->invMass = 1.0f / particle->mass;

          state->particleCount++;

          lbPressedAt = state->time;
        }

        // Register click at 100ms intervals.
        // 1s = 10³ms
        if ((state->time - lbPressedAt) >= 0.1f) {
          lbPressedAt = 0.0f;
        }
      } else {
        lbPressedAt = 0.0f;
      }
#endif
    }
  }

  /*****************************************************************
   * PHYSICS
   *****************************************************************/
  /*
   * - Apply forces
   */
  for (u32 particleIndex = 0; particleIndex < state->particleCount; particleIndex++) {
    struct particle *particle = state->particles + particleIndex;
    b8 isFirstParticle = particleIndex == 0;
    b8 isLastParticle = particleIndex == state->particleCount - 1;
    struct particle *prevParticle = !isFirstParticle ? state->particles + particleIndex - 1 : 0;

    // apply input force
    if (isLastParticle)
      v2_add_ref(&particle->netForce, v2_scale(inputForce, 50.0f));

    // apply weight force
    v2 weightForce = GenerateWeightForce(particle);
    v2_add_ref(&particle->netForce, weightForce);

    // apply drag force
    v2 dragForce = GenerateDragForce(particle, 0.001f);
    v2_add_ref(&particle->netForce, dragForce);

    // apply spring force
    // explanation:
    // - "Multiple spring-mass system" @00:00:28
    //   - https://www.youtube.com/watch?v=3wfMPxORS-4
    //   - https://cdn.kastatic.org/ka-youtube-converted/3wfMPxORS-4.mp4/3wfMPxORS-4.mp4
    f32 springConstant = 100.0f;
    f32 restLength = 1.0f;
    v2 anchorPosition;
    if (prevParticle) {
      // if particle is not first one
      anchorPosition = prevParticle->position;
    } else {
      // if particle is the first one
      anchorPosition = state->springAnchorPosition;
    }
    v2 springForce = GenerateSpringForce(particle, anchorPosition, restLength, springConstant);
    v2_add_ref(&particle->netForce, springForce);
    v2 dampingForce = GenerateDampingForce(particle, 2.5f);
    v2_add_ref(&particle->netForce, dampingForce);
    if (prevParticle) {
      v2_add_ref(&prevParticle->netForce, v2_neg(springForce));
      v2_add_ref(&prevParticle->netForce, v2_neg(dampingForce));
    }
  }

  f32 ground = -5.8f;

  /*
   * - Integrate applied forces
   */
  for (u32 particleIndex = 0; particleIndex < state->particleCount; particleIndex++) {
    struct particle *particle = state->particles + particleIndex;
    b8 isLastParticle = particleIndex == state->particleCount - 1;

#if 0
    // constant acceleration
    // acceleration = f''(t) = a
    f32 speed = 30.0f; // unit: m/s
    particle->acceleration = v2_scale((v2){1.0f, 0.0f}, speed);
#endif

    // F = ma
    // a = F/m
    particle->acceleration = v2_scale(particle->netForce, particle->invMass);

    // velocity     = ∫f''(t)
    //              = f'(t) = at + v₀
    v2_add_ref(&particle->velocity, v2_scale(particle->acceleration, dt));
    // position     = ∫f'(t)
    //              = f(t) = ½at² + vt + p₀
    v2_add_ref(&particle->position, v2_add(
                                        // ½at²
                                        v2_scale(particle->acceleration, 0.5f * Square(dt)),
                                        // + vt
                                        v2_scale(particle->velocity, dt)));

#if (1 && IS_BUILD_DEBUG)
    {
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("particle #"));
      StringBuilderAppendU64(sb, particleIndex + 1);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));

      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("  pos: "));
      StringBuilderAppendF32(sb, particle->position.x, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(", "));
      StringBuilderAppendF32(sb, particle->position.y, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));

      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("  vel: "));
      StringBuilderAppendF32(sb, particle->velocity.x, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(", "));
      StringBuilderAppendF32(sb, particle->velocity.y, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));

      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("  acc: "));
      StringBuilderAppendF32(sb, particle->acceleration.x, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(", "));
      StringBuilderAppendF32(sb, particle->acceleration.y, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));

      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("  Fnet:"));
      StringBuilderAppendF32(sb, particle->netForce.x, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(", "));
      StringBuilderAppendF32(sb, particle->netForce.y, 2);
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));

      if (isLastParticle) {
        StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
        StringBuilderAppendString(
            sb, &STRING_FROM_ZERO_TERMINATED("****************************************************************"));
      }
      StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));

      string string = StringBuilderFlush(sb);
      write(STDOUT_FILENO, string.value, string.length);
    }
#endif

    // clear forces
    particle->netForce = (v2){0.0f, 0.0f};

    /*
     * COLLISION
     */
    // TODO: Ground collision is broken
    if (particle->position.y <= ground) {
      v2 groundNormal = {0.0f, 1.0f};

      // reflect
      // v' = v - 2(v∙n)n
      particle->velocity =
          v2_sub(particle->velocity, v2_scale(groundNormal, 2.0f * v2_dot(particle->velocity, groundNormal)));
    }
  }

  /*****************************************************************
   * RENDER
   *****************************************************************/
  ClearScreen(renderer, COLOR_ZINC_900);

#if (0 && IS_BUILD_DEBUG)
  // Cartesian coordinate system
  DrawLine(renderer, (v2){-100, 0}, (v2){100, 0}, COLOR_BLUE_500, 0);
  DrawLine(renderer, (v2){0, -100}, (v2){0, 100}, COLOR_BLUE_500, 0);
#endif

  // ground
  DrawLine(renderer, (v2){-15, ground}, (v2){15, ground}, COLOR_GRAY_500, 1);

#if 0
  // liquid
  DrawRect(renderer, state->liquid, COLOR_BLUE_950);
#endif

  // mouse
  DrawCrosshair(renderer, mousePosition, 0.5f, COLOR_RED_500);

  if (impulse) {
    particle *lastParticle = state->particles + state->particleCount - 1;
    DrawLine(renderer, lastParticle->position, mousePosition, COLOR_BLUE_300, 1);
  }

  // spring
  v2 springAnchorPosition = state->springAnchorPosition;
  DrawLine(renderer, v2_add(springAnchorPosition, (v2){-1.0f, 0.0}), v2_add(springAnchorPosition, (v2){1.0f, 0.0f}),
           COLOR_RED_500, 1);

  // particles
  for (u32 particleIndex = 0; particleIndex < state->particleCount; particleIndex++) {
    struct particle *particle = state->particles + particleIndex;

    f32 massNormalized = particle->mass / 10.0f /* maximum particle mass */;
    u32 colorIndex = (u32)(Lerp(0.0f, ARRAY_COUNT(COLORS) / 11, massNormalized));
    const v4 *color = COLORS + colorIndex * 11 + 6;

    DrawCircle(renderer, particle->position, 0.01f + particle->mass / 10.0f, *color);

    // spring connection
    if (particleIndex == 0)
      DrawLine(renderer, springAnchorPosition, particle->position, COLOR_RED_500, 1);

    struct particle *nextParticle =
        (particleIndex != state->particleCount - 1) ? state->particles + particleIndex + 1 : 0;
    if (nextParticle)
      DrawLine(renderer, particle->position, nextParticle->position, COLOR_RED_500, 1);
  }

  RenderFrame(renderer);
}
