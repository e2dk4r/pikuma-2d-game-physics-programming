#pragma once

#include "type.h"

typedef struct {
  b8 isDown : 1;  // holding button down, being pressed
  b8 wasDown : 1; // released or clicked
} game_controller_button;

typedef struct {
  union {
    struct {
      game_controller_button a;
      game_controller_button b;
      game_controller_button x;
      game_controller_button y;

      game_controller_button back;
      game_controller_button start;
      game_controller_button home;

      game_controller_button ls;
      game_controller_button rs;

      game_controller_button lb;
      game_controller_button rb;
    };

    game_controller_button buttons[11];
  };

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
