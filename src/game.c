#include "game.h"
#include "color.h"
#include "math.h"
#include "renderer.h"
#include "string_builder.h"

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

    state->effectsEntropy = RandomSeed(213);
    random_series *effectsEntropy = &state->effectsEntropy;

    state->particleMax = 100;
    state->particles = MemoryArenaPush(worldArena, sizeof(*state->particles) * state->particleMax, 4);
    for (u32 particleIndex = 0; particleIndex < state->particleCount; particleIndex++) {
      struct particle *particle = state->particles + particleIndex;
      *particle = (struct particle){
          .position =
              {
                  .x = RandomBetween(effectsEntropy, -3.0f, 3.0f),
                  .y = RandomBetween(effectsEntropy, 3.0f, 5.0f),
              },
          .mass = RandomBetween(effectsEntropy, 1.0f, 5.0f),
      };
      particle->invMass = 1.0f / particle->mass;
    }

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
  v2 mousePosition = {};
  v2 inputForce = {};
  for (u32 controllerIndex = 0; controllerIndex < ARRAY_COUNT(input->controllers); controllerIndex++) {
    game_controller *controller = input->controllers + controllerIndex;
    v2 input = {controller->lsX, controller->lsY};
    f32 inputLengthSq = v2_length_square(input);
    if (inputLengthSq > 1.0f) {
      // make sure input vector length is 1.0
      input = v2_scale(input, 1.0f / SquareRoot(inputLengthSq));
      debug_assert(v2_length(input) <= 1.0f);
    }

    if (controllerIndex == GAME_CONTROLLER_KEYBOARD_AND_MOUSE_INDEX) {
      v2 surfaceHalfDim = RectGetHalfDim(RendererGetSurfaceRect(renderer));
      mousePosition = v2_hadamard((v2){controller->rsX, controller->rsY}, // [-1.0, 1.0]
                                  surfaceHalfDim);

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
    }

    inputForce = v2_add(inputForce, input);
  }

  /*****************************************************************
   * PHYSICS
   *****************************************************************/
#if (1 && IS_BUILD_DEBUG)
  {
    particle *slowestParticle = state->particles + 0;
    particle *fastestParticle = state->particles + 0;
    for (u32 particleIndex = 0; particleIndex < state->particleCount; particleIndex++) {
      struct particle *particle = state->particles + particleIndex;

      if (v2_length_square(particle->velocity) < v2_length_square(slowestParticle->velocity))
        slowestParticle = particle;

      if (v2_length_square(particle->velocity) > v2_length_square(fastestParticle->velocity))
        fastestParticle = particle;
    }

#define STRING_BUILDER_APPEND_PARTICLE(prefix, particle)                                                               \
  StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(prefix));                                                 \
  StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  mass:         "));                                   \
  StringBuilderAppendF32(sb, particle->mass, 2);                                                                       \
  StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("kg "));                                                  \
  StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  position:     "));                                   \
  StringBuilderAppendF32(sb, particle->position.x, 2);                                                                 \
  StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(", "));                                                   \
  StringBuilderAppendF32(sb, particle->position.y, 2);                                                                 \
  StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  velocity:     "));                                   \
  StringBuilderAppendF32(sb, v2_length(particle->velocity), 2);                                                        \
  StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("m/s "));                                                 \
  StringBuilderAppendF32(sb, particle->velocity.x, 2);                                                                 \
  StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(", "));                                                   \
  StringBuilderAppendF32(sb, particle->velocity.y, 2);                                                                 \
  StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n  acceleration: "));                                   \
  StringBuilderAppendF32(sb, particle->acceleration.x, 2);                                                             \
  StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(", "));                                                   \
  StringBuilderAppendF32(sb, particle->acceleration.y, 2);                                                             \
  StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"))

    STRING_BUILDER_APPEND_PARTICLE("slowest particle:", slowestParticle);
    STRING_BUILDER_APPEND_PARTICLE("fastest particle:", fastestParticle);
#undef STRING_BUILDER_APPEND_PARTICLE

    string string = StringBuilderFlush(sb);
    write(STDOUT_FILENO, string.value, string.length);
  }
#endif

  f32 ground = -5.8f;

  v2 windForce = {2.0f, 0.0f};
  // see: https://en.wikipedia.org/wiki/Gravity_of_Earth
  v2 gravitationForce = {0.0f, -9.80665f};

  for (u32 particleIndex = 0; particleIndex < state->particleCount; particleIndex++) {
    struct particle *particle = state->particles + particleIndex;

    v2 sumOfForces = {0.0f, 0.0f};
    // apply input force
    sumOfForces = v2_add(sumOfForces, v2_scale(inputForce, 5.0f));
    // apply weight force Fw = mg
    v2 weightForce = v2_scale(gravitationForce, particle->mass);
    sumOfForces = v2_add(sumOfForces, weightForce);

    // apply drag force when we are inside the liquid
    if (IsPointInsideRect(particle->position, state->liquid)) {
      v2 dragForce = {0.0f, 0.0f};
      if (v2_length_square(particle->velocity) > 0.0f) {
        // Drag force law is:
        //   F = ½ ρ K A ||v||² (-normalized(v))
        //   where ρ is fluid density
        //         K is coefficient of drag
        //         A is cross-sectional area
        //         v is velocity
        //         ||v||² is velocity's magnitude squared
        // We can simplify this formul by replacing all constant to:
        //   F = k ||v||² (-normalized(v))
        v2 dragDirection = v2_neg(v2_normalize(particle->velocity));
        f32 k = 0.05f;
        f32 dragMagnitude = k * v2_length_square(particle->velocity);
        dragForce = v2_scale(dragDirection, dragMagnitude);
      }
      sumOfForces = v2_add(sumOfForces, dragForce);
    } else { // if not in liquid
      // apply wind force
      sumOfForces = v2_add(sumOfForces, windForce);
    }

    // F = ma
    // a = F/m
    particle->acceleration = v2_scale(sumOfForces, particle->invMass);

    // acceleration = f''(t) = a
    // particle->acceleration = v2_scale((v2){1.0f, 0.0f}, speed);

    // velocity     = ∫f''(t)
    //              = f'(t) = at + v₀
    // position     = ∫f'(t)
    //              = f(t) = ½at² + vt + p₀
    particle->velocity = v2_add(particle->velocity, v2_scale(particle->acceleration, dt));
    particle->position =
        // ½at² + vt + p₀
        v2_add(particle->position, v2_add(
                                       // ½at²
                                       v2_scale(particle->acceleration, 0.5f * Square(dt)),
                                       // + vt
                                       v2_scale(particle->velocity, dt)));

    // TODO: Ground collision is broken
    if (particle->position.y <= ground) {
      v2 groundNormal = {0.0f, 1.0f};

      // reflect
      // v' = v - 2(v∙n)n
      particle->velocity =
          v2_sub(particle->velocity, v2_scale(groundNormal, 2.0f * v2_dot(particle->velocity, groundNormal)));
    }

    // Is particle over 10m away from origin?
    if (v2_length_square(particle->position) > Square(10.0f)) {
      random_series *effectsEntropy = &state->effectsEntropy;
      particle->position = (v2){
          .x = RandomBetween(effectsEntropy, -5.0f, 5.0f),
          .y = RandomBetween(effectsEntropy, 3.0f, 5.0f),
      };
      particle->velocity = (v2){0, 0};
    }
  }

  /*****************************************************************
   * RENDER
   *****************************************************************/
  ClearScreen(renderer, COLOR_ZINC_900);

#if (0)
  // Cartesian coordinate system
  DrawLine(renderer, (v2){-100, 0}, (v2){100, 0}, COLOR_BLUE_500, 0);
  DrawLine(renderer, (v2){0, -100}, (v2){0, 100}, COLOR_BLUE_500, 0);
#endif

  // ground
  DrawLine(renderer, (v2){-15, ground}, (v2){15, ground}, COLOR_GRAY_500, 1);

  // liquid
  DrawRect(renderer, state->liquid, COLOR_BLUE_950);

  // mouse
  DrawCrosshair(renderer, mousePosition, 0.5f, COLOR_RED_500);

  // particles
  for (u32 particleIndex = 0; particleIndex < state->particleCount; particleIndex++) {
    struct particle *particle = state->particles + particleIndex;

    f32 massNormalized = particle->mass / 10.0f /* maximum particle mass */;
    u32 colorIndex = (u32)(Lerp(0.0f, ARRAY_COUNT(COLORS) / 11, massNormalized));
    const v4 *color = COLORS + colorIndex * 11 + 6;

    DrawCircle(renderer, particle->position, 0.01f + particle->mass / 10.0f, *color);
  }

  RenderFrame(renderer);
}
