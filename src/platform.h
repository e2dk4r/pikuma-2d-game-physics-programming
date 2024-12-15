#pragma once

#include "type.h"

typedef struct {
  b8 a : 1;
  b8 b : 1;
  b8 x : 1;
  b8 y : 1;

  b8 back : 1;
  b8 start : 1;
  b8 home : 1;

  b8 ls : 1;
  b8 rs : 1;

  b8 lb : 1;
  b8 rb : 1;

  // [-1, 1] math coordinates
  f32 lsX;
  // [-1, 1] math coordinates
  f32 lsY;
  // [-1, 1] math coordinates
  f32 rsX;
  // [-1, 1] math coordinates
  f32 rsY;

  // [0, 1] math coordinates
  f32 lt;
  // [0, 1] math coordinates
  f32 rt;
} game_controller;

#define GAME_CONTROLLER_KEYBOARD_AND_MOUSE_INDEX 0

static inline game_controller *
GameControllerGetKeyboardAndMouse(game_controller *controllers, u32 controllerCount)
{
  debug_assert(controllerCount > 0);
  u32 index = GAME_CONTROLLER_KEYBOARD_AND_MOUSE_INDEX;
  debug_assert(index < controllerCount);
  game_controller *controller = controllers + index;
  return controller;
}

static inline game_controller *
GameControllerGetGamepad(game_controller *controllers, u32 controllerCount, u32 index)
{
  debug_assert(controllerCount > 0);
  index += 1; // 0 is reserved for keyboard
  debug_assert(index < controllerCount);
  game_controller *controller = controllers + index;
  return controller;
}

typedef struct {
  f32 dt;                         // in seconds
  game_controller controllers[3]; // 1 keyboard + 2 controllers
} game_input;

typedef struct {
  void *permanentStorage; // required to be to zero
  u64 permanentStorageSize;

  void *transientStorage;
  u64 transientStorageSize;
} game_memory;
