#pragma once

#include "memory.h"
#include "type.h"

/*
 * Physics assumes
 * - Every unit is in SI units.
 *   eg. length is meters
 * - Coordination system is in math space.
 *     x positive means right, negative left
 *     y positive means up, negative down
 */

typedef enum volume_type {
  VOLUME_TYPE_CIRCLE,
  VOLUME_TYPE_POLYGON,
  VOLUME_TYPE_BOX,
} volume_type;

// Tagged union. see: volume_*
typedef struct volume {
  volume_type type;
} volume;

typedef struct volume_circle {
  f32 radius;
} volume_circle;

typedef struct volume_polygon {
  v2 *verticies;
  u32 vertexCount;
} volume_polygon;

typedef struct volume_box {
  f32 width;
  f32 height;
} volume_box;

static volume_circle *
VolumeGetCircle(volume *volume);
static volume_polygon *
VolumeGetPolygon(volume *volume);
static volume_box *
VolumeGetBox(volume *volume);

static volume *
VolumeCircle(memory_arena *memory, f32 radius);

static volume *
VolumePolygon(memory_arena *memory, u32 vertexCount, v2 verticies[static vertexCount]);

static volume *
VolumeBox(memory_arena *memory, f32 width, f32 height);

static f32
VolumeGetMomentOfInertia(volume *volume, f32 mass);

typedef struct contact {
  v2 start;
  v2 end;
  v2 normal;
  f32 depth;
} contact;

typedef struct entity {
  /* LINEAR KINEMATICS */
  v2 position;     // unit: m
  v2 velocity;     // unit: m/s
  v2 acceleration; // unit: m/s²
  f32 mass;        // unit: kg
  f32 invMass;     // computed from 1/mass. unit: kg⁻¹
  v2 netForce;     // sum of all forces applied

  /* ANGULAR KINEMATICS */
  f32 rotation;            // θ, unit: rad
  f32 angularVelocity;     // ω, unit: rad/s
  f32 angularAcceleration; // α, unit: rad/s²
  f32 netTorque;           // sum of all torque forces applied
  f32 I;                   // moment of inertia. unit: kg m²
  f32 invI;                // computed from 1/I. unit: kg⁻¹ m⁻²

  b8 isColliding;
  v4 color;
  volume *volume;
} entity;

#define ENTITY_STATIC_MASS 0.0f

static b8
IsEntityStatic(struct entity *entity);

/* Generate weight force */
static v2
GenerateWeightForce(struct entity *entity);

/* Generate wind force */
static v2
GenerateWindForce(void);

/* Generate friction force
 * @param k friction constant. unit: kg
 */
static v2
GenerateFrictionForce(struct entity *entity, f32 k);

/* Generate drag force
 * @param k drag constant
 */
static v2
GenerateDragForce(struct entity *entity, f32 k);

/* Generate gravitational attraction force
 * @param G is universal gravitational constant. unit: m³ kg⁻¹ s⁻²
 */
static v2
GenerateGravitationalAttractionForce(struct entity *a, struct entity *b, f32 G);

/* Generate spring force
 * @param entity entity that attached to anchor
 * @param anchorPosition position of anchor
 * @param equilibrium length of spring when not compressed or extended.
 * @param k spring constant. unit: kg/m. Higher values makes spring more stiff.
 */
static v2
GenerateSpringForce(struct entity *entity, v2 anchorPosition, f32 equilibrium, f32 k);

/* Generate damping force
 * @param k damping constant. unit: kg/m
 */
static v2
GenerateDampingForce(struct entity *entity, f32 k);
