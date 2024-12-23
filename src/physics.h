#pragma once

/*
 * Physics assumes
 * - Every unit is in SI units.
 *   eg. length is meters
 * - Coordination system is in math space.
 *     x positive means right, negative left
 *     y positive means up, negative down
 */

typedef struct entity {
  v2 position;     // unit: m
  v2 velocity;     // unit: m/s
  v2 acceleration; // unit: m/s²
  f32 mass;        // unit: kg
  f32 invMass;     // computed from 1/mass. unit: kg⁻¹
  v2 netForce;
} entity;

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
