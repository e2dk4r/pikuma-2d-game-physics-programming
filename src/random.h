#pragma once

#include "type.h"

typedef struct random_series {
  u32 randomNumberIndex;
} random_series;

static struct random_series
RandomSeed(u32 value);

// [0, U32_MAX]
static u32
RandomNumber(struct random_series *series);

// [0, choiceCount)
static u32
RandomChoice(struct random_series *series, u32 choiceCount);

// [0, 1]
static f32
RandomNormal(struct random_series *series);

// [-1, 1]
static f32
RandomUnit(struct random_series *series);

// [min, max]
static f32
RandomBetween(struct random_series *series, f32 min, f32 max);

// [min, max]
static s32
RandomBetweens32(struct random_series *series, s32 min, s32 max);
