#include "physics.h"
#include "compiler.h"
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
  debug_assert(volume->type == VOLUME_TYPE_CIRCLE);
  return (volume_circle *)VolumeGetType(volume);
}

static volume_polygon *
VolumeGetPolygon(volume *volume)
{
  debug_assert(volume->type == VOLUME_TYPE_POLYGON);
  return (volume_polygon *)VolumeGetType(volume);
}

static volume_box *
VolumeGetBox(volume *volume)
{
  debug_assert(volume->type == VOLUME_TYPE_BOX);
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

  circle = VolumeGetCircle(volume);
  circle->radius = radius;

  return volume;
}

static volume *
VolumePolygon(memory_arena *memory, u32 vertexCount, v2 verticies[static vertexCount])
{
  volume_polygon *polygon;
  volume *volume = Volume(memory, VOLUME_TYPE_POLYGON, sizeof(*polygon));
  if (!volume)
    return 0;

  polygon = VolumeGetPolygon(volume);

  polygon->vertexCount = vertexCount;
  v2 *allocatedVerticies = MemoryArenaPushUnaligned(memory, sizeof(*allocatedVerticies) * polygon->vertexCount);
  for (u32 vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++) {
    allocatedVerticies[vertexIndex] = verticies[vertexIndex];
  }
  polygon->verticies = allocatedVerticies;

  return volume;
}

static volume *
VolumeBox(memory_arena *memory, f32 width, f32 height)
{
  volume_box *box;
  volume *volume = Volume(memory, VOLUME_TYPE_BOX, sizeof(*box));
  if (!volume)
    return 0;

  box = VolumeGetBox(volume);
  box->width = width;
  box->height = height;

  return volume;
}

static inline f32
VolumeCircleGetMomentOfInertia(volume *volume, f32 mass)
{
  volume_circle *circle = VolumeGetCircle(volume);
  /*
   * Thin, solid disk of radius r and mass m.
   * see: https://en.wikipedia.org/wiki/List_of_moments_of_inertia
   *
   * I = ½ m r²
   */
  f32 I = 0.5f * mass * Square(circle->radius);
  return I;
}

static inline f32
VolumeBoxGetMomentOfInertia(volume *volume, f32 mass)
{
  volume_box *box = VolumeGetBox(volume);

  /*
   * Thin rectangular plate of height h, width w and mass m (Axis of rotation
   * at the center)
   * see: https://en.wikipedia.org/wiki/List_of_moments_of_inertia
   *
   * I = (1/12) m (h² + w²)
   */
  f32 I = (1.0f / 12.0f) * mass * (Square(box->width) + Square(box->height));
  return I;
}

static inline f32
VolumeGetMomentOfInertia(volume *volume, f32 mass)
{
  debug_assert(mass != 0.0f);

  switch (volume->type) {
  case VOLUME_TYPE_CIRCLE: {
    return VolumeCircleGetMomentOfInertia(volume, mass);
  } break;
  case VOLUME_TYPE_BOX: {
    return VolumeBoxGetMomentOfInertia(volume, mass);
  } break;

  default: {
    breakpoint("don't know how to calculate moment of inertia for this volume");
    return 0.0f;
  } break;
  }
}

static b8
IsEntityStatic(struct entity *entity)
{
#if 1
  return entity->invMass == ENTITY_STATIC_MASS;
#else
  // TODO: Is entity can become a static object after initialization?
  f32 epsilon = 0.005f;
  return Absolute(entity->mass) < epsilon;
#endif
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

static void
ApplyImpulse(struct entity *a, v2 j)
{
  /* # MOMENTUM
   * Momentum p is a measurement of mass in motion
   *
   *   p = m v
   *   where m is mass
   *         v is velocity
   *
   * As objects collide, they transfer some of their momentum to other
   * but total momentum of system should always be constant.
   * a.k.a. conservation of momentum
   *
   *    m₁v₁ + m₂v₂ = m₁v'₁ + m₂v'₂
   *
   * NOTE: We assume mass does not change
   *
   * # THE IMPULSE METHOD
   * Impulse J is change in momentum by performing a direct change in velocity
   *
   *   J = F ∆t
   *     = ∆p
   *     = mv' - mv
   *     = m ∆v
   *
   * If we solve for force over time we get the same result. This means
   * if we apply force over time object will get momentum.
   *
   *   J = F ∆t
   *     = m a ∆t
   *     = m (∆v/∆t) ∆t
   *     = m ∆v
   *
   *   ∆v = J / m
   */
  if (IsEntityStatic(a))
    return;

  v2_add_ref(&a->velocity, v2_scale(j, a->invMass));
}

static v2
FindFurthestPoint(struct entity *entity, v2 direction)
{
  volume *volume = entity->volume;

  switch (volume->type) {
  case VOLUME_TYPE_CIRCLE: {
    volume_circle *circle = VolumeGetCircle(volume);
    v2 maxPoint = v2_scale(direction, circle->radius);
    return v2_add(entity->position, maxPoint);
  } break;

  case VOLUME_TYPE_BOX: {
    volume_box *box = VolumeGetBox(volume);
    v2 halfSize = V2(box->width * 0.5f, box->height * 0.5f);
    v2 pointInDirection = v2_hadamard(halfSize, V2(SignOf(direction.x), SignOf(direction.y)));
    return v2_add(entity->position, pointInDirection);
  } break;

  case VOLUME_TYPE_POLYGON: {
    volume_polygon *polygon = VolumeGetPolygon(volume);
    v2 *verticies = polygon->verticies;
    u32 vertexCount = polygon->vertexCount;

    v2 maxPoint = verticies[0];
    f32 maxDistance = v2_dot(maxPoint, direction);
    for (u32 vertexIndex = 1; vertexIndex < vertexCount; vertexIndex++) {
      v2 point = verticies[vertexIndex];
      f32 distance = v2_dot(point, direction);
      if (distance > maxDistance) {
        maxPoint = point;
        maxDistance = distance;
      }
    }

    return v2_add(entity->position, maxPoint);
  } break;

  default: {
    breakpoint("unsupported volume");
    return V2(0.0f, 0.0f);
  } break;
  }
}

static v2
Support(struct entity *a, struct entity *b, v2 direction)
{
  return v2_sub(FindFurthestPoint(a, direction), FindFurthestPoint(b, v2_neg(direction)));
}

static b8
IsSameDirection(v3 direction, v3 ao)
{
  return v3_dot(direction, ao) > 0.0f;
}

static b8
CollisionDetect(struct entity *entityA, struct entity *entityB, contact *contact)
{
  if (entityA->volume->type > entityB->volume->type) {
    struct entity *tmp = entityA;
    entityA = entityB;
    entityB = tmp;
  }

  b8 isColliding = 0;

  switch (entityA->volume->type | entityB->volume->type) {
  case VOLUME_TYPE_CIRCLE | VOLUME_TYPE_CIRCLE: {
    volume_circle *circleA = VolumeGetCircle(entityA->volume);
    volume_circle *circleB = VolumeGetCircle(entityB->volume);

    v2 positionA = entityA->position;
    v2 positionB = entityB->position;

    v2 distance = v2_sub(positionB, positionA);
    isColliding = v2_length_square(distance) <= Square(circleA->radius + circleB->radius);
    if (!isColliding)
      return isColliding;

    contact->normal = v2_normalize(distance);
    contact->start = v2_sub(positionB, v2_scale(contact->normal, circleB->radius));
    contact->end = v2_add(positionA, v2_scale(contact->normal, circleA->radius));
    contact->depth = v2_length(v2_sub(contact->end, contact->start));
  } break;

  case VOLUME_TYPE_BOX | VOLUME_TYPE_BOX: {
#if 0
    /* see:
     * - https://www.youtube.com/watch?v=MDusDn8oTSE "GJK Algorithm Explanation & Implementation"
     *   https://winter.dev/articles/gjk-algorithm
     * - https://www.youtube.com/watch?v=Qupqu1xe7Io "Implementing GJK - 2006"
     */
    // get initial support point in any direction
    v2 support = Support(entityA, entityB, V2(1.0f, 0.0f));

    // simplex is array of points, max of 3 for 2D
    v2 points[3];
    points[0] = support;
    u32 pointCount = 1;

    // new direction is towards origin
    v2 direction = v2_neg(support);

    while (!isColliding) {
      support = Support(entityA, entityB, direction);
      if (v2_dot(support, direction) <= 0) {
        // did not pass origin in search direction
        // no collision / no intersection
        isColliding = 0;
        return isColliding;
      }

      points[pointCount] = support;
      pointCount++;
      debug_assert(pointCount <= ARRAY_COUNT(points));

      switch (pointCount) {
      case 2: {
        // when simplex is line segment
        v3 a = {.xy = points[1]};
        v3 b = {.xy = points[0]};

        v3 ab = v3_sub(b, a);
        v3 ao = v3_neg(a);

        v2 abPerp = v3_cross(v3_cross(ab, ao), ab).xy;
        if (v2_length_square(abPerp) == 0.0f) {
          isColliding = 1;
        } else {
          direction = abPerp;
        }

      } break;
      case 3: {
        // when simplex forms triangle
        v3 a = {.xy = points[2]}; // new point, that just got added
        v3 b = {.xy = points[1]};
        v3 c = {.xy = points[0]};

        v3 ab = v3_sub(b, a);
        v3 ac = v3_sub(c, a);
        v3 ao = v3_neg(a);

        v3 abcNormal = v3_cross(ab, ac);
        v3 abNormal = v3_cross(ab, abcNormal);
        v3 acNormal = v3_cross(abcNormal, ac);

        if (IsSameDirection(abNormal, ao)) {
          direction = v3_cross(v3_cross(ab, ao), ab).xy;

          points[0] = b.xy;
          points[1] = a.xy;
          pointCount = 2;
        } else if (IsSameDirection(acNormal, ao)) {
          direction = v3_cross(v3_cross(ac, ao), ac).xy;

          points[0] = c.xy;
          points[1] = a.xy;
          pointCount = 2;
        } else {
          // must be inside of triangle
          isColliding = 1;
          break;
        }
      } break;
      }
    }

    /*
     * Entities collided.
     * Find penetration depth using Expanding Polytope Algorithm (EPA)
     * see: https://winter.dev/articles/epa-algorithm
     */

    struct v2 polytope[16];
    u8 vertexCount = ARRAY_COUNT(points);
    memcpy(polytope, points, sizeof(*points) * vertexCount);

    struct edge {
      u8 index;
      f32 distance;
      v2 normal;
    } closestEdge;

    while (vertexCount < ARRAY_COUNT(polytope)) {
      closestEdge.distance = F32_MAX;

      // find the edge closest to the origin
      for (u8 vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++) {
        u8 Aindex = vertexIndex;
        u8 Bindex = (u8)((vertexIndex + 1) % vertexCount);
        v2 A = polytope[Aindex];
        v2 B = polytope[Bindex];
        v2 AB = v2_sub(B, A);
        v2 normal = v2_normalize(v2_perp(AB));
        debug_assert(v2_length_square(normal) != 0);
        f32 distance = v2_dot(A, normal);
        if (distance < 0) {
          distance *= -1.0f;
          v2_neg_ref(&normal);
        }

        if (distance < closestEdge.distance) {
          closestEdge.distance = distance;
          closestEdge.normal = normal;
          closestEdge.index = Bindex;
        }
      }

      // test if there is point further out in normal's direction
      support = Support(entityA, entityB, closestEdge.normal);
      f32 supportDistance = v2_dot(support, closestEdge.normal);

      f32 epsilon = 0.0001f;
      if (Absolute(supportDistance - closestEdge.distance) <= epsilon) {
        break;
      }
      /* expand polytope, insert support to polytope
       * CASE 1:
       *   1 2 3 4
       * 0
       * ---------
       * 0 1 2 3 4
       *
       * CASE 2:
       * 0   2 3 4
       *   1
       * ---------
       * 0 1 2 3 4
       */
      u8 before = closestEdge.index;
      u8 after = vertexCount - before;
      memcpy(polytope + (before + 1), polytope + before, sizeof(*polytope) * after);
      polytope[before] = support;
      vertexCount++;
    }

    // BUG: TODO: Entity partially stucks at corner after colliding.
    //            Is Minkowski Difference correct?

    // TODO: Is contact information correct?
    // see:
    //   - "Dirk Gregorius - Robust Contact Creation for Physics Simulation"
    //     https://gdcvault.com/play/1022193/Physics-for-Game-Programmers-Robust
    //     https://cdn-a.blazestreaming.com/out/v1/a914e787bbfd48af89b2fdc4721cb75c/38549d9a185b445888c6fbc2e2e792e9/50413c5b8a95441e919f0a1579606e47/index.m3u8
    contact->normal = closestEdge.normal;
    contact->depth = closestEdge.distance;
    contact->start = FindFurthestPoint(entityB, v2_neg(contact->normal));
    contact->end = FindFurthestPoint(entityA, contact->normal);
#else
    /* From paper:
     *   2024 - Wei Gao - Efficient Incremental Penetration Depth Estimation between Convex Geometries
     * Reference implementation:
     *   https://github.com/weigao95/mind-fcl/blob/86edd6c89db0682cc9f99ae852117c8992ad966a/include/fcl/cvx_collide/mpr.hpp#L501
     *   See below for how it used in test
     *   https://github.com/weigao95/mind-fcl/blob/86edd6c89db0682cc9f99ae852117c8992ad966a/test/cvx_collide/test_mpr_penetration.cpp#L65
     */
    u32 maxIterations = 1000;
    f32 tolerance = 1e-6f;

    v3 d = {.xy = v2_normalize(entityB->velocity)};
    if (v3_length_square(d) == 0.0f) {
      d = (v3){.xy = v2_normalize(v2_sub(entityB->position, entityA->position))};
    }
    debug_assert(v3_length_square(d) != 0);

    // v0_to_O is the direction. This v0 is actually mock.
    // It may NOT lie in the MinkowskiDiff shape
    v3 v0 = v3_neg(d);

    v3 v1_direction = v3_normalize(d);
    v3 v1 = {.xy = Support(entityA, entityB, v1_direction.xy)};

    // no intersect case 1
    if (v3_dot(v1_direction, v1) < 0) {
      return 0;
    }

    v3 v2_direction = v3_cross(v0, v1);
    // LEFTOFF: v2_direction is 0? Is it implementation issue?
    debug_assert(v3_length_square(v2_direction) != 0);
    // o_to_v0 and o_to_v1 can be co-linear, check it
    // Note that v0 MUST have norm one
    // This equation might be written as
    //   cross(o_to_v0.normalized(), o_to_v1.normalized()).norm() <= tolerance
    // which can be further expanded as
    //   cross(o_to_v0, o_to_v1).norm() <= tolerance * v0.norm() * v1.norm()
    // However, we do not want to compute the L2 norm, thus replace it with
    // abs norm, which is much easier to compute.
    if (v3_absolute_norm(v2_direction) <= v3_absolute_norm(v1) * tolerance) {
      // o_to_v0 and o_to_v1 can be co-linear, from the condition above
      // v1_direction.dot(v1) = - v0_interior.dot(v1) < 0 is False
      // which implies
      // - v0_interior.dot(v1) > 0  --> v0_interior.dot(v1) < 0
      // As v0 = o_to_v0, v1 = o_to_v1, we have
      // o_to_v0.dot(o_to_v1) < 0, o is in the middle of v0/v1
      // As v0 is an interior point, v1 is a boundary point
      // We conclude O must be within the shape
      contact->depth = v3_dot(v1, d);
      contact->normal = v3_normalize(v1_direction).xy;
      contact->start = FindFurthestPoint(entityB, v2_neg(contact->normal));
      contact->end = FindFurthestPoint(entityA, contact->normal);
      return 1;
    }

    v3 v2 = {.xy = Support(entityA, entityB, v3_normalize(v2_direction).xy)};

    // no intersect
    if (v3_dot(v2_direction, v2) < 0)
      return 0;

    // it is better to form portal faces to be oriented "outside" origin
    // Here O must be an interior point in penetration query
    v3 v3_direction = v3_cross(v1, v2);
    if (v3_dot(v3_direction, v0) > 0) {
      swap(v1, v2);
      swap(v1_direction, v2_direction);
      v3_neg_ref(&v3_direction);
    }

    v3 v3 = {.xy = Support(entityA, entityB, v3_normalize(v3_direction).xy)};

    // no intersect
    if (v3_dot(v3_direction, v3) < 0) {
      return 0;
    }

    // Scale the v0 to the max of v1/v2/v3
    struct v3 v0_scaled = v0;
    {
      f32 squaredNorms[3] = {v3_length_square(v1), v3_length_square(v2), v3_length_square(v3)};
      f32 max = squaredNorms[0];
      for (u32 index = 1; index < ARRAY_COUNT(squaredNorms); index++) {
        f32 squaredNorm = squaredNorms[index];
        if (squaredNorm > max)
          max = squaredNorm;
      }
      v3_scale_ref(&v0_scaled, SquareRoot(max));
    }

    // The loop to find the portal
    struct v3 o_to_v0 = v0;
    f32 v0_absoluteNorm = v3_absolute_norm(v0);
    for (u32 findCandidatePortalIteration = 0;; findCandidatePortalIteration++) {
      if (findCandidatePortalIteration == maxIterations) {
        // Iteration limit
        return 0;
      }

      struct v3 v0_to_v1 = v3_sub(v1, v0);
      struct v3 v0_to_v2 = v3_sub(v2, v0);
      struct v3 v0_to_v3 = v3_sub(v3, v0);

      // Update the corresponded vertex
      // These normal are not oriented
      struct v3 v031_normal = v3_cross(v0_to_v3, v0_to_v1);
      struct v3 v012_normal = v3_cross(v0_to_v1, v0_to_v2);
      // Orient it
      if (v3_dot(v0_to_v2, v031_normal) < 0) {
        swap(v2, v3);
        swap(v2_direction, v3_direction);
        swap(v0_to_v2, v0_to_v3);

        // Something tricky here, note the changing of vectors
        swap(v012_normal, v031_normal);
        v3_neg_ref(&v031_normal);
        v3_neg_ref(&v012_normal);
      }

      debug_assert(v3_dot(v0_to_v2, v031_normal) >= 0);
      b8 is_v031_seperated_v2_and_o =
          v3_dot(o_to_v0, v031_normal) > F32_EPSILON * v0_absoluteNorm * v3_absolute_norm(v031_normal);
      if (is_v031_seperated_v2_and_o) {
        // Orient the normal towards O
        debug_assert(v3_dot(o_to_v0, v012_normal) > 0);
        struct v3 search_v3_direction = v012_normal;
        v3_neg_ref(&search_v3_direction);

        // Find a new v3 in that direction
        v3 = (struct v3){.xy = Support(entityA, entityB, search_v3_direction.xy)};
        v3_direction = search_v3_direction;

        // Miss detection
        if (v3_dot(v3, search_v3_direction) < 0) {
          // detect seperated
          return 0;
        }

        // Loop again
        continue;
      }

      // case 023
      struct v3 v023_normal = v3_cross(v0_to_v2, v0_to_v3);
      debug_assert(v3_dot(v0_to_v3, v012_normal) >= 0);
      b8 is_v023_seperated_v1_and_o =
          v3_dot(o_to_v0, v023_normal) > F32_EPSILON * v0_absoluteNorm * v3_absolute_norm(v023_normal);
      if (is_v023_seperated_v1_and_o) {
        // Orient the normal towards O
        debug_assert(v3_dot(o_to_v0, v023_normal) > 0);
        struct v3 search_v1_direction = v023_normal;
        v3_neg_ref(&search_v1_direction);

        // Find a new v2 in that direction
        v1 = (struct v3){.xy = Support(entityA, entityB, search_v1_direction.xy)};
        v1_direction = search_v1_direction;

        // Miss detection
        if (v3_dot(v1, search_v1_direction) < 0) {
          // detect seperated
          return 0;
        }

        // Loop again
        continue;
      }

      // No seperation, we are done.
      break;
    }

    // You can safely assume that portal found from now on.

    // Portal refinement
    for (u32 portalRefinementIteration = 0;; portalRefinementIteration++) {
      if (portalRefinementIteration == maxIterations) {
        // iteration limit
        return 0;
      }

      // Compute the normal
      // The v123_normal must be oriented in the same side with O
      struct v3 v123_normal = v3_cross(v3_sub(v2, v1), v3_sub(v3, v1));
      if (v3_dot(v123_normal, d) < 0) {
        swap(v2, v3);
        swap(v2_direction, v3_direction);
        v3_neg_ref(&v123_normal);
      }

      // A new point v4 on that direction
      struct v3 v4 = {.xy = Support(entityA, entityB, v3_normalize(v123_normal).xy)};
      if (v3_dot(v4, v123_normal) < 0) {
        return 0;
      }

      // Seperation plane very close to the new (candidate) portal
      // Note that v123_normal can be un-normalized, thus its length
      // must be considered
      struct v3 v1_to_v4 = v3_sub(v4, v1);
      if (Absolute(v3_dot(v1_to_v4, v123_normal)) < tolerance * v3_absolute_norm(v123_normal)) {
        contact->normal = v123_normal.xy;
        f32 v123_normal_dot_d = v3_dot(v123_normal, d);

        // Very unlikely case that can not divide the dot
        if (unlikely(Absolute(v123_normal_dot_d) == 0)) {
          f32 d_dot_v123[] = {
              v3_dot(v1, d),
              v3_dot(v2, d),
              v3_dot(v3, d),
          };
          struct v3 d_for_v123[] = {
              v1_direction,
              v2_direction,
              v3_direction,
          };

          u32 maxDistanceIndex = 0;
          f32 maxDistance = d_dot_v123[maxDistanceIndex];
          for (u32 index = 1; index < ARRAY_COUNT(d_dot_v123); index++) {
            if (d_dot_v123[index] > maxDistance) {
              maxDistance = d_dot_v123[index];
              maxDistanceIndex = index;
            }
          }

          contact->normal = d_for_v123[maxDistanceIndex].xy;
          contact->depth = maxDistance;
          contact->start = FindFurthestPoint(entityB, contact->normal);
          contact->end = FindFurthestPoint(entityA, v2_neg(contact->normal));
        } else {
          contact->depth = v3_dot(v4, v123_normal) / v123_normal_dot_d;

          f32 o_to_v1_dot_v123_normal = v3_dot(v1, v123_normal);
          debug_assert(o_to_v1_dot_v123_normal >= 0);
          f32 distance_to_v123 = o_to_v1_dot_v123_normal / v123_normal_dot_d;

          struct v3 o_projected = v3_scale(d, distance_to_v123);
          struct v3 v1_to_v2 = v3_sub(v2, v1);
          struct v3 v1_to_v3 = v3_sub(v3, v1);
          struct v3 s1s2s3_planeNormal = v3_cross(v1_to_v2, v1_to_v3);
          f32 area = v3_length(s1s2s3_planeNormal);
          f32 s2_weight = v3_length(v3_cross(v1_to_v3, v3_sub(v1, o_projected))) / area;
          f32 s3_weight = v3_length(v3_cross(v1_to_v2, v3_sub(v1, o_projected))) / area;
          f32 s1_weight = 1.0f - s2_weight - s3_weight;

          struct v2 aPointsWeighted[] = {
              v2_scale(FindFurthestPoint(entityA, v1_direction.xy), s1_weight),
              v2_scale(FindFurthestPoint(entityA, v2_direction.xy), s2_weight),
              v2_scale(FindFurthestPoint(entityA, v3_direction.xy), s3_weight),
          };
          contact->end = v2_add_multiple(ARRAY_COUNT(aPointsWeighted), aPointsWeighted);
          struct v2 bPointsWeighted[] = {
              v2_scale(FindFurthestPoint(entityB, v2_neg(v1_direction.xy)), s1_weight),
              v2_scale(FindFurthestPoint(entityB, v2_neg(v2_direction.xy)), s2_weight),
              v2_scale(FindFurthestPoint(entityB, v2_neg(v3_direction.xy)), s3_weight),
          };
          contact->start = v2_add_multiple(ARRAY_COUNT(bPointsWeighted), bPointsWeighted);
        }

        return 1;
      }

      // Update the portal

      // v4 must appear in the next portal
      // select two in v1, v2 and v3
      // First do a seperation in with plane v0_v4_o
      struct v3 o_to_v4 = v4;
      // struct v3 o_to_v0 = v0;
      struct v3 o_to_v1 = v1;
      struct v3 o_to_v2 = v2;
      struct v3 o_to_v3 = v3;

      struct v3 v0_v4_o_normal = v3_cross(o_to_v4, o_to_v0);
      if (v3_dot(o_to_v1, v0_v4_o_normal) > 0) {
        if (v3_dot(o_to_v2, v0_v4_o_normal) > 0) {
          // Discard v1
          v1 = v4;
          v1_direction = v123_normal;
        } else {
          // Discard v3
          v3 = v4;
          v3_direction = v123_normal;
        }
      } else {
        if (v3_dot(o_to_v3, v0_v4_o_normal) > 0) {
          // Discard v2
          v2 = v4;
          v2_direction = v123_normal;
        } else {
          // Discard v1
          v1 = v4;
          v1_direction = v123_normal;
        }
      }
    }

    //
    return 0;

#endif
  } break;

  default: {
    breakpoint("don't know how to detect collisition between volumes");
  }; break;
  }

  return isColliding;
}

static void
CollisionResolvePenetration(struct entity *a, struct entity *b, contact *contact)
{
  /* # THE PROJECTION METHOD
   * Adjust the "position" of colliding objects.
   *
   *   d₁ = depth m₂ / (m₁ + m₂)
   *   d₂ = depth m₁ / (m₁ + m₂)
   *
   * If we apply to convert to using only inverse of mass. Because we may
   * choose to store only inverse of mass of an entity.
   *
   *   d₁ = depth / (1/m₁ + 1/m₂) 1/m₁
   *   d₂ = depth / (1/m₁ + 1/m₂) 1/m₂
   *
   */
  f32 displacementA = contact->depth / (a->invMass + b->invMass) * a->invMass;
  f32 displacementB = contact->depth / (a->invMass + b->invMass) * b->invMass;

  v2_sub_ref(&a->position, v2_scale(contact->normal, displacementA));
  v2_add_ref(&b->position, v2_scale(contact->normal, displacementB));
}

static void
CollisionResolve(struct entity *a, struct entity *b, contact *contact)
{
  CollisionResolvePenetration(a, b, contact);

  /* # THE IMPULSE METHOD
   *
   * The impulse generated by the velocity at which two objects collided.
   *
   * Therefore we are interested in relative velocity of two objects.
   * (e.g. Think of two cars on highway going parallel to each other)
   *
   *   vRel = v₁ - v₂
   *
   * The impluse vector is relative velocity along the collision normal.
   *
   *   vRel∙n
   *   where ∙ is dot product
   *
   * Elasticty of collision is
   *
   *   v'Rel∙n = -ε (vRel∙n)
   *   where ε is the coefficient of restitution,
   *           which determines the elasticity (bounciness) of collision
   *           [0, 1]
   *
   * J is magnitude of impulse
   *
   * ∆p₁ = m₁ v'₁ - m₁ v₁ ⇒  Jn = m₁ v'₁ - m₁ v₁ ⇒ v'₁ = v₁ + (Jn / m₁) ┐
   *                                                                    ├ v'Rel∙n = (v'₁ - v'₂)∙n
   * ∆p₂ = m₂ v'₂ - m₂ v₂ ⇒ -Jn = m₂ v'₂ - m₂ v₂ ⇒ v'₂ = v₂ - (Jn / m₂) ┘
   *
   *       v'Rel = (v₁ - v₂) + (Jn/m₁ + Jn/m₂)
   *     v'Rel∙n = vRel∙n + (Jn/m₁ + Jn/m₂)∙n
   *  -ε(vRel∙n) = vRel∙n + Jn(1/m₁ + 1/m₂)∙n
   *           ┌────────────┘
   *           J = (-ε(vRel∙n) - (vRel)∙n) / ((1/m₁ + 1/m₂) n∙n)
   *             = (     -(1 + ε)(vRel∙n)) / ( 1/m₁ + 1/m₂     )
   *
   *
   */
  f32 e = Minimum(a->restitution, b->restitution);

  // relative velocity
  v2 vRel = v2_sub(a->velocity, b->velocity);

  // impulse vector along the normal
  f32 impulseMagnitude = -(1.0f + e) * v2_dot(vRel, contact->normal) / (a->invMass + b->invMass);
  v2 impulseDirection = contact->normal;

  v2 Jn = v2_scale(impulseDirection, impulseMagnitude);

  // apply impluse vector to both objects in opposite direction
  ApplyImpulse(a, Jn);
  ApplyImpulse(b, v2_neg(Jn));
}
