#include "physics.h"
#include "math.h"

static u8 *
VolumeGetType(volume *volume)
{
  u8 *ptr = (u8 *)volume + sizeof(*volume);
  return ptr;
}

static volume_circle *
VolumeGetCircle(volume *volume)
{
  return (volume_circle *)VolumeGetType(volume);
}

static volume_polygon *
VolumeGetPolygon(volume *volume)
{
  return (volume_polygon *)VolumeGetType(volume);
}

static volume_box *
VolumeGetBox(volume *volume)
{
  return (volume_box *)VolumeGetType(volume);
}

static volume *
Volume(memory_arena *memory, volume_type type, u64 typeSize)
{
  volume *result = 0;

  u64 size = sizeof(*result) + typeSize;
  result = MemoryArenaPushUnaligned(memory, size);
  result->type = type;

  return result;
}

static volume *
VolumeCircle(memory_arena *memory, f32 radius)
{
  volume_circle *circle;
  volume *volume = Volume(memory, VOLUME_TYPE_CIRCLE, sizeof(*circle));
  if (!volume)
    return 0;

  circle = (void *)volume + sizeof(*volume);
  circle->radius = radius;

  return volume;
}

static volume *
VolumePolygon(memory_arena *memory, u32 vertexCount, v2 verticies[static vertexCount])
{
  volume_polygon *polygon;
  volume *volume = Volume(memory, VOLUME_TYPE_CIRCLE, sizeof(*polygon));
  if (!volume)
    return 0;

  polygon = (void *)volume + sizeof(*volume);

  polygon->vertexCount = vertexCount;
  v2 *allocatedVerticies = MemoryArenaPushUnaligned(memory, sizeof(*allocatedVerticies) * polygon->vertexCount);
  for (u32 vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++) {
    allocatedVerticies[vertexIndex] = verticies[vertexIndex];
  }
  polygon->verticies = allocatedVerticies;

  return volume;
}

static volume *
VolumeBox(memory_arena *memory, v2 verticies[static 4])
{
  volume_polygon *box;
  volume *volume = Volume(memory, VOLUME_TYPE_BOX, sizeof(*box));
  if (!volume)
    return 0;

  box = (void *)volume + sizeof(*volume);

  u32 vertexCount = 4;
  for (u32 vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++) {
    box->verticies[vertexIndex] = verticies[vertexIndex];
  }

  return volume;
}

static v2
GenerateWeightForce(struct entity *entity)
{
  // see: https://en.wikipedia.org/wiki/Gravity_of_Earth
  // unit: m/s²
  const v2 earthGravityForce = {0.0f, -9.80665f};

  /* Generate weight force
   *   F = mg
   *   where m is mass
   *         g is gravity
   */

  v2 weightForce = v2_scale(earthGravityForce, entity->mass);
  return weightForce;
}

static v2
GenerateWindForce(void)
{
  v2 windForce = {2.0f, 0.0f};
  return windForce;
}

static v2
GenerateFrictionForce(struct entity *entity, f32 k)
{
  /* Generate friction force
   *   F = μ ‖Fn‖ (-normalized(v))
   *   where μ is coefficent of friction
   *         Fn is normal force applied by surface
   *         v is velocity
   * We can simplfy this equation to
   *   F = k (-v)
   *   where k is magnitude of friction
   */

  v2 frictionDirection = v2_neg(v2_normalize(entity->velocity));
  f32 frictionMagnitude = 0.2f;
  v2 frictionForce = v2_scale(frictionDirection, frictionMagnitude);
  return frictionForce;
}
static v2
GenerateDragForce(struct entity *entity, f32 k)
{
  /* Generate drag force
   * Drag force law is:
   *   F = ½ ρ K A ‖v‖² (-normalized(v))
   *   where ρ is fluid density
   *         K is coefficient of drag
   *         A is cross-sectional area
   *         v is velocity
   *         ‖v‖² is velocity's magnitude squared
   * We can simplify this formul by replacing all constant to:
   *   F = k ‖v‖² (-normalized(v))
   */

  v2 dragForce = {0.0f, 0.0f};
  if (v2_length_square(entity->velocity) > 0.0f) {
    v2 dragDirection = v2_neg(v2_normalize(entity->velocity));
    f32 dragMagnitude = k * v2_length_square(entity->velocity);
    dragForce = v2_scale(dragDirection, dragMagnitude);
  }
  return dragForce;
}

static v2
GenerateGravitationalAttractionForce(struct entity *a, struct entity *b, f32 G)
{
  /* unit: m³ kg⁻¹ s⁻²
   * see: https://en.wikipedia.org/wiki/Gravitational_constant#Modern_value
   */
  const f32 UNIVERSAL_GRAVITATIONAL_CONSTANT = 6.6743015e-11f;

  /* Generate gravitational attraction force
   *   F = G ((m₁ m₂) / ‖d‖²) normalized(d)
   *   where G is universal gravitational constant. unit: m³ kg⁻¹ s⁻²
   *         d is distance or attraction force
   */

  v2 distance = v2_sub(b->position, a->position);
  f32 distanceSquared = v2_length_square(distance);

  // Avoid division by zero or excessively large forces when entities are too
  // close. Not physically accurate.
  distanceSquared = Clamp(distanceSquared, 0.1f, 8.0f);

  f32 attractionMagnitude = G * (a->mass * b->mass) / distanceSquared;
  v2 attractionDirection = v2_normalize(distance);
  v2 attractionForce = v2_scale(attractionDirection, attractionMagnitude);

  return attractionForce;
}

static v2
GenerateSpringForce(struct entity *entity, v2 anchorPosition, f32 equilibrium, f32 k)
{
  /* Generate spring force
   * Hooke's Law:
   *   F = -k ∆l
   *   where k is spring constant
   *         ∆l is the displacement of the system from its equilibrium position
   */

  v2 distance = v2_sub(entity->position, anchorPosition);
  f32 displacement = v2_length(distance) - equilibrium;

  v2 springDirection = v2_normalize(distance);
  f32 springMagnitude = -k * displacement;
  v2 springForce = v2_scale(springDirection, springMagnitude);
  return springForce;
}

static v2
GenerateDampingForce(struct entity *entity, f32 k)
{
  /* Damping force:
   *   F = -kv
   *   where k is damping constant
   *         v is velocity
   */

  v2 dampingForce = v2_scale(entity->velocity, -k);
  return dampingForce;
}
