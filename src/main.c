#include <unistd.h> // write()

#include "compiler.h"
#include "game.h"
#include "type.h"

#if !IS_BUILD_DEBUG
#include "game.c"
#endif

#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_loadso.h>
#include <SDL3/SDL_main.h>

typedef struct {
  u64 loadedAt;
  SDL_SharedObject *handle;

  pfnGameUpdateAndRender GameUpdateAndRender;
} game_library;

typedef struct {
  SDL_Window *window;
  f32 invWindowWidth;
  f32 invWindowHeight;
  game_memory memory;
  game_input inputs[2];
  u32 inputIndex : 1;
  game_renderer renderer;
  u64 lastTime;
  string_builder sb;
#if IS_BUILD_DEBUG
  string executablePath;
  game_library lib;
#endif
} sdl_state;

#if IS_BUILD_DEBUG

static void
GameLibraryReload(game_library *lib, sdl_state *state)
{
  // create absolute path, instead of relative
  string_builder *sb = &state->sb;
  string pwd = PathGetDirectory(&state->executablePath);
  debug_assert(pwd.length > 0);
  StringBuilderAppendString(sb, &pwd);
  StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("/")); // seperator
  StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("game.so"));
  string libPath = StringBuilderFlushZeroTerminated(sb);

  // get create time
  SDL_PathInfo libPathInfo;
  b8 isFileExists = SDL_GetPathInfo((const char *)libPath.value, &libPathInfo);
  if (!isFileExists)
    return;

  u64 libCreatedAt = (u64)libPathInfo.create_time;
  if (lib->loadedAt == libCreatedAt)
    return;

  // close already open library
  if (lib->handle) {
    SDL_UnloadObject(lib->handle);
    lib->handle = 0;
  }

  lib->handle = SDL_LoadObject((const char *)libPath.value);
  debug_assert(lib->handle && "cannot load library");

  lib->GameUpdateAndRender = (pfnGameUpdateAndRender)SDL_LoadFunction(lib->handle, "GameUpdateAndRender");
  debug_assert(lib->GameUpdateAndRender && "library malformed");

  {
    string *message = &STRING_FROM_ZERO_TERMINATED("Reloaded library!\n");
    write(STDOUT_FILENO, message->value, message->length);
  }

  lib->loadedAt = libCreatedAt;
}

#endif

SDL_AppResult
SDL_AppIterate(void *appstate)
{
  sdl_state *state = appstate;

  u64 nowInNanoseconds = SDL_GetTicksNS();
  debug_assert(nowInNanoseconds > 0);
  u64 elapsedInNanoseconds = nowInNanoseconds - state->lastTime;
#if IS_BUILD_DEBUG
  // Prevent ∆t to be invalid when debugging
  if (elapsedInNanoseconds > 17000000 /* 17ms */)
    elapsedInNanoseconds = 16666666;
#endif

  game_input *input = state->inputs + state->inputIndex;
  input->dt = (f32)elapsedInNanoseconds * 1e-9f;

  game_memory *memory = &state->memory;
  game_renderer *renderer = &state->renderer;
#if IS_BUILD_DEBUG
  GameLibraryReload(&state->lib, state);
  pfnGameUpdateAndRender GameUpdateAndRender = state->lib.GameUpdateAndRender;
#endif
  GameUpdateAndRender(memory, input, renderer);

  state->lastTime = nowInNanoseconds;

  {
    // Cycle inputs
    state->inputIndex = (state->inputIndex + 1) % ARRAY_COUNT(state->inputs);
    game_input *prevInput = input;
    game_input *nextInput = state->inputs + state->inputIndex;
    *nextInput = *prevInput;
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult
SDL_AppEvent(void *appstate, SDL_Event *event)
{
  sdl_state *state = appstate;
  game_input *input = state->inputs + state->inputIndex;

  switch (event->type) {
  case SDL_EVENT_QUIT: {
    return SDL_APP_SUCCESS;
  } break;

  case SDL_EVENT_KEY_DOWN:
  case SDL_EVENT_KEY_UP: {
    game_controller *keyboardAndMouse =
        GameControllerGetKeyboardAndMouse(input->controllers, ARRAY_COUNT(input->controllers));

#if (0 && IS_BUILD_DEBUG)
    SDL_KeyboardEvent keyboardEvent = event->key;
    string_builder *sb = &state->sb;
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("key scancode: "));
    StringBuilderAppendU64(sb, keyboardEvent.scancode);
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(" down: "));
    StringBuilderAppendU64(sb, keyboardEvent.down);
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(" repeat: "));
    StringBuilderAppendU64(sb, keyboardEvent.repeat);
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
    string s = StringBuilderFlush(sb);
    write(STDOUT_FILENO, s.value, s.length);
#endif
    b8 *keyboardState = (b8 *)SDL_GetKeyboardState(0);
    if (unlikely(!keyboardState))
      break;

    if (keyboardState[SDL_SCANCODE_LEFT] || keyboardState[SDL_SCANCODE_A])
      keyboardAndMouse->lsX = -1.0f;
    else if (keyboardState[SDL_SCANCODE_RIGHT] || keyboardState[SDL_SCANCODE_D])
      keyboardAndMouse->lsX = 1.0f;
    else
      keyboardAndMouse->lsX = 0.0f;

    if (keyboardState[SDL_SCANCODE_DOWN] || keyboardState[SDL_SCANCODE_S])
      keyboardAndMouse->lsY = -1.0f;
    else if (keyboardState[SDL_SCANCODE_UP] || keyboardState[SDL_SCANCODE_W])
      keyboardAndMouse->lsY = 1.0f;
    else
      keyboardAndMouse->lsY = 0.0f;
  } break;

  case SDL_EVENT_MOUSE_BUTTON_DOWN:
  case SDL_EVENT_MOUSE_BUTTON_UP: {
    game_controller *keyboardAndMouse =
        GameControllerGetKeyboardAndMouse(input->controllers, ARRAY_COUNT(input->controllers));
    SDL_MouseButtonEvent buttonEvent = event->button;
    u64 timestamp = buttonEvent.timestamp;
    b8 isPressed = buttonEvent.down;

    if (buttonEvent.button == SDL_BUTTON_LEFT)
      keyboardAndMouse->lb = (u8)(isPressed & 0x1);
    else if (buttonEvent.button == SDL_BUTTON_RIGHT)
      keyboardAndMouse->rb = (u8)(isPressed & 0x1);
  } break;

  case SDL_EVENT_MOUSE_MOTION: {
    game_controller *keyboardAndMouse =
        GameControllerGetKeyboardAndMouse(input->controllers, ARRAY_COUNT(input->controllers));
    SDL_MouseMotionEvent motionEvent = event->motion;

    keyboardAndMouse->rsX = motionEvent.x * state->invWindowWidth * 2.0f - 1.0f;
    keyboardAndMouse->rsY = motionEvent.y * state->invWindowHeight * 2.0f - 1.0f;
    keyboardAndMouse->rsY *= -1;

#if (0 && IS_BUILD_DEBUG)
    SDL_KeyboardEvent keyboardEvent = event->key;
    string_builder *sb = &state->sb;
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("mouse x "));
    StringBuilderAppendF32(sb, mouseEvent.x, 2);
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED(" y "));
    StringBuilderAppendF32(sb, mouseEvent.y, 2);
    StringBuilderAppendString(sb, &STRING_FROM_ZERO_TERMINATED("\n"));
    string s = StringBuilderFlush(sb);
    write(STDOUT_FILENO, s.value, s.length);
#endif
  } break;

  // when gamepad is added, open gamepad
  case SDL_EVENT_GAMEPAD_ADDED: {
    SDL_GamepadDeviceEvent deviceEvent = event->gdevice;
    s32 gamepadIndex = SDL_GetGamepadPlayerIndexForID(deviceEvent.which);
    if (gamepadIndex < 0 || gamepadIndex >= ARRAY_COUNT(input->controllers) - 1 /* index 0 is reserved for keyboard */)
      break;

    SDL_OpenGamepad(deviceEvent.which);
  } break;

    // when gamepad is removed, clear gamepad
  case SDL_EVENT_GAMEPAD_REMOVED: {
    SDL_GamepadDeviceEvent deviceEvent = event->gdevice;
    s32 gamepadIndex = SDL_GetGamepadPlayerIndexForID(deviceEvent.which);
    if (gamepadIndex < 0)
      break;
    for (u32 inputIndex = 0; inputIndex < ARRAY_COUNT(state->inputs); inputIndex++) {
      game_input *input = state->inputs + inputIndex;
      game_controller *controller =
          GameControllerGetGamepad(input->controllers, ARRAY_COUNT(input->controllers), (u32)gamepadIndex);
      memset(controller, 0, sizeof(*controller));
    }
  } break;

  case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
  case SDL_EVENT_GAMEPAD_BUTTON_UP: {
    SDL_GamepadButtonEvent buttonEvent = event->gbutton;
    s32 gamepadIndex = SDL_GetGamepadPlayerIndexForID(buttonEvent.which);
    debug_assert(gamepadIndex >= 0);
    if (gamepadIndex < 0)
      break;
    game_controller *controller =
        GameControllerGetGamepad(input->controllers, ARRAY_COUNT(input->controllers), (u32)gamepadIndex);

    b8 isPressed = buttonEvent.down;
    switch (buttonEvent.button) {
    case SDL_GAMEPAD_BUTTON_SOUTH: {
      controller->a = (u8)(isPressed & 0x1);
    } break;
    case SDL_GAMEPAD_BUTTON_EAST: {
      controller->b = (u8)(isPressed & 0x1);
    } break;
    case SDL_GAMEPAD_BUTTON_WEST: {
      controller->x = (u8)(isPressed & 0x1);
    } break;
    case SDL_GAMEPAD_BUTTON_NORTH: {
      controller->y = (u8)(isPressed & 0x1);
    } break;
    case SDL_GAMEPAD_BUTTON_BACK: {
      controller->back = (u8)(isPressed & 0x1);
    } break;
    case SDL_GAMEPAD_BUTTON_GUIDE: {
      controller->home = (u8)(isPressed & 0x1);
    } break;
    case SDL_GAMEPAD_BUTTON_START: {
      controller->start = (u8)(isPressed & 0x1);
    } break;
    case SDL_GAMEPAD_BUTTON_LEFT_STICK: {
      controller->ls = (u8)(isPressed & 0x1);
    } break;
    case SDL_GAMEPAD_BUTTON_RIGHT_STICK: {
      controller->rs = (u8)(isPressed & 0x1);
    } break;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: {
      controller->lb = (u8)(isPressed & 0x1);
    } break;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: {
      controller->rb = (u8)(isPressed & 0x1);
    } break;
    case SDL_GAMEPAD_BUTTON_DPAD_UP: {
      controller->lsY = (f32)isPressed;
    } break;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN: {
      controller->lsY = -(f32)isPressed;
    } break;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT: {
      controller->lsX = -(f32)isPressed;
    } break;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: {
      controller->lsX = (f32)isPressed;
    } break;
    }
  } break;

  case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
    SDL_GamepadAxisEvent axisEvent = event->gaxis;

    s32 gamepadIndex = SDL_GetGamepadPlayerIndexForID(axisEvent.which);
    debug_assert(gamepadIndex >= 0);
    if (gamepadIndex < 0)
      break;

    // normalize
    //   ((value - min) / range)
    // convert to [-1.0, 1.0] range
    //   2.0 * value - 1.0
    f32 value = ((f32)(axisEvent.value - S16_MIN) / U16_MAX) * 2.0f - 1.0f;

    game_controller *controller =
        GameControllerGetGamepad(input->controllers, ARRAY_COUNT(input->controllers), (u32)gamepadIndex);
    switch (axisEvent.axis) {
    case SDL_GAMEPAD_AXIS_LEFTX: {
      controller->lsX = value;
    } break;
    case SDL_GAMEPAD_AXIS_LEFTY: {
      controller->lsY = -value;
    } break;
    case SDL_GAMEPAD_AXIS_RIGHTX: {
      controller->rsX = value;
    } break;
    case SDL_GAMEPAD_AXIS_RIGHTY: {
      controller->rsY = -value;
    } break;
    case SDL_GAMEPAD_AXIS_LEFT_TRIGGER: {
      controller->lt = value;
    } break;
    case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER: {
      controller->rt = value;
    } break;
    }
  } break;
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult
SDL_AppInit(void **appstate, int argc, char *argv[])
{
  // metadata
  {
    if (!SDL_SetAppMetadata("Example game", "1.0", "com.example.game")) {
      return SDL_APP_FAILURE;
    }

    static struct {
      const char *key;
      const char *value;
    } metadata[] = {
        {SDL_PROP_APP_METADATA_URL_STRING, "https://examples.libsdl.org/SDL3/game/01-"},
        {SDL_PROP_APP_METADATA_CREATOR_STRING, "SDL team"},
        {SDL_PROP_APP_METADATA_COPYRIGHT_STRING, "Copyright © 2024"},
        {SDL_PROP_APP_METADATA_TYPE_STRING, "game"},
    };
    for (u32 index = 0; index < ARRAY_COUNT(metadata); index++) {
      if (!SDL_SetAppMetadataProperty(metadata[index].key, metadata[index].value)) {
        return SDL_APP_FAILURE;
      }
    }
  }

  // setup memory
  const u64 KILOBYTES = 1 << 10;
  const u64 MEGABYTES = 1 << 20;
  const u64 PERMANANT_MEMORY_USAGE = 8 * MEGABYTES;
  const u64 TRANSIENT_MEMORY_USAGE = 32 * MEGABYTES;
  const u64 RENDERER_MEMORY_USAGE = 1 * MEGABYTES;
  const u64 STRING_BUILDER_MEMORY_USAGE = 1 * KILOBYTES;

  memory_arena memory = {};
  {
    memory.total =
        PERMANANT_MEMORY_USAGE + TRANSIENT_MEMORY_USAGE + RENDERER_MEMORY_USAGE + STRING_BUILDER_MEMORY_USAGE;
    memory.total += sizeof(sdl_state); // for app state tracking
    memory.block = SDL_malloc(memory.total);
    if (memory.block == 0) {
      return SDL_APP_FAILURE;
    }
  }

  // setup game state
  sdl_state *state = MemoryArenaPush(&memory, sizeof(*state), 4);
  memset(state, 0, sizeof(*state));

  const s32 windowWidth = 1280;
  const s32 windowHeight = 720;
  state->invWindowWidth = 1.0f / (f32)windowWidth;
  state->invWindowHeight = 1.0f / (f32)windowHeight;

  game_renderer *renderer = &state->renderer;
  {
    renderer->memory = MemoryArenaSub(&memory, RENDERER_MEMORY_USAGE);
    memset(renderer->memory.block, 0, renderer->memory.total);

    renderer->screenCenter = (v2){(f32)windowWidth * 0.5f, (f32)windowHeight * 0.5f};
  }

  { // - string builder
    memory_arena sbMemory = MemoryArenaSub(&memory, STRING_BUILDER_MEMORY_USAGE);
    string *stringBuffer = MemoryArenaPush(&sbMemory, sizeof(*stringBuffer), 4);
    *stringBuffer = MemoryArenaPushString(&sbMemory, 32);
    string *outBuffer = MemoryArenaPush(&sbMemory, sizeof(*outBuffer), 4);
    *outBuffer = MemoryArenaPushString(&sbMemory, sbMemory.total - sbMemory.used);

    string_builder *sb = &state->sb;
    sb->outBuffer = outBuffer;
    sb->stringBuffer = stringBuffer;
  }

#if IS_BUILD_DEBUG
  state->executablePath = StringFromZeroTerminated((u8 *)argv[0], 1024);
  GameLibraryReload(&state->lib, state);
#endif

  { // setup game memory
    game_memory *gameMemory = &state->memory;
    gameMemory->permanentStorageSize = PERMANANT_MEMORY_USAGE;
    gameMemory->permanentStorage = MemoryArenaPush(&memory, gameMemory->permanentStorageSize, 4);
    memset(gameMemory->permanentStorage, 0, gameMemory->permanentStorageSize);

    gameMemory->transientStorageSize = TRANSIENT_MEMORY_USAGE;
    gameMemory->transientStorage = MemoryArenaPush(&memory, gameMemory->transientStorageSize, 4);

    transient_state *transientState = gameMemory->transientStorage;
    transientState->sb = &state->sb;
  }
  debug_assert(memory.used == memory.total && "Warning: you are not using specified memory amount");

  // SDL
  *appstate = state;

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_HAPTIC | SDL_INIT_GAMEPAD)) {
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer("Example Title", windowWidth, windowHeight, 0, &state->window,
                                   &renderer->renderer)) {
    return SDL_APP_FAILURE;
  }

  if (!SDL_SetRenderVSync(renderer->renderer, 1)) {
    return SDL_APP_FAILURE;
  }

  SDL_HideCursor();

  // if (!SDL_SetRenderScale(renderer->renderer, (f32)windowWidth * PIXELS_PER_METER,
  //                         (f32)windowHeight * PIXELS_PER_METER)) {
  //   return SDL_APP_FAILURE;
  // }

  state->lastTime = SDL_GetTicksNS();

  return SDL_APP_CONTINUE;
}

void
SDL_AppQuit(void *appstate, SDL_AppResult result)
{
  sdl_state *state = appstate;
  SDL_DestroyRenderer(state->renderer.renderer);
}
