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
    f32 halfWidth = box->width * 0.5f;
    f32 halfHeight = box->height * 0.5f;
    v2 verticies[] = {
        // left bottom
        {-halfWidth, -halfHeight},
        // right bottom
        {halfWidth, -halfHeight},
        // right top
        {halfWidth, halfHeight},
        // left top
        {-halfWidth, halfHeight},
    };
    u32 vertexCount = ARRAY_COUNT(verticies);

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
IsSameDirection(v2 direction, v2 ao)
{
  return v2_dot(direction, ao) > 0.0f;
}

static v2
TripleCrossProduct(v2 a, v2 b, v2 c)
{
  v3 A = {.xy = a};
  v3 B = {.xy = b};
  v3 C = {.xy = c};
  v3 cross = v3_cross(v3_cross(A, B), C);
  return cross.xy;
}

static b8
CollisionDetect(struct entity *a, struct entity *b, contact *contact)
{
  if (a->volume->type > b->volume->type) {
    struct entity *tmp = a;
    a = b;
    b = tmp;
  }

  b8 isColliding = 0;

  switch (a->volume->type | b->volume->type) {
  case VOLUME_TYPE_CIRCLE | VOLUME_TYPE_CIRCLE: {
    volume_circle *circleA = VolumeGetCircle(a->volume);
    volume_circle *circleB = VolumeGetCircle(b->volume);

    v2 distance = v2_sub(b->position, a->position);
    isColliding = v2_length_square(distance) <= Square(circleA->radius + circleB->radius);
    if (!isColliding)
      return isColliding;

    contact->normal = v2_normalize(distance);
    contact->start = v2_sub(b->position, v2_scale(contact->normal, circleB->radius));
    contact->end = v2_add(a->position, v2_scale(contact->normal, circleA->radius));
    contact->depth = v2_length(v2_sub(contact->end, contact->start));
  } break;

  case VOLUME_TYPE_BOX | VOLUME_TYPE_BOX: {
    /* see:
     * - https://www.youtube.com/watch?v=MDusDn8oTSE "GJK Algorithm Explanation & Implementation"
     *   https://winter.dev/articles/gjk-algorithm
     * - https://www.youtube.com/watch?v=Qupqu1xe7Io "Implementing GJK - 2006"
     */
    // get initial support point in any direction
    v2 support = Support(a, b, V2(1.0f, 0.0f));

    // simplex is array of points, max of 3 for 2D
    v2 points[3] = {};
    points[0] = support;
    u32 pointCount = 1;

    // new direction is towards origin
    v2 direction = v2_neg(support);

    while (!isColliding) {
      support = Support(a, b, direction);
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
        // if simplex is line
        v2 pointA = points[1]; // new point, just got added
        v2 pointB = points[0]; // old point, was in the simplex before

        v2 ab = v2_sub(pointB, pointA); // a to b
        v2 ao = v2_neg(pointA);         // a to origin

        if (IsSameDirection(ab, ao)) {
          // if ab and ao is in same direction
          // direction is perpendicular to the edge that points to origin
          //   ab × ao × ab

          if (ab.y == 0.0f) {
            // edge case
            // TripleCrossProduct() fails
            direction = v2_perp(ab);
          } else {
            direction = TripleCrossProduct(ab, ao, ab);
          }
        } else {
          v2 newPoints[] = {pointA};
          pointCount = ARRAY_COUNT(newPoints);
          memcpy(points, newPoints, pointCount);

          direction = ao;
        }
      } break;
      case 3: {
        // if simplex is triangle
        v2 pointA = points[2]; // new point, that just got added
        v2 pointB = points[1];
        v2 pointC = points[0];

        v2 ab = v2_sub(pointB, pointA);
        v2 ac = v2_sub(pointC, pointA);
        v2 ao = v2_neg(pointA);

        v2 abPerp = TripleCrossProduct(ac, ab, ab);
        if (IsSameDirection(abPerp, ao)) {
          // Line({a, b}, direction)
          v2 newPoints[] = {pointB, pointA};
          pointCount = ARRAY_COUNT(newPoints);
          memcpy(points, newPoints, pointCount);

          direction = abPerp;
          continue;
        }

        v2 acPerp = TripleCrossProduct(ab, ac, ac);
        if (IsSameDirection(acPerp, ao)) {
          // Line({a, c}, direction)
          v2 newPoints[] = {pointC, pointA};
          pointCount = ARRAY_COUNT(newPoints);
          memcpy(points, newPoints, pointCount);

          direction = acPerp;
          continue;
        }

        // must be inside of triangle
        isColliding = 1;
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
    memcpy(polytope, points, sizeof(*polytope) * vertexCount);

    f32 minDistance = F32_MAX;
    v2 minNormal;
    u8 minIndex;

    while (minDistance == F32_MAX                 /* used for continuing search for closest normal */
           && vertexCount < ARRAY_COUNT(polytope) /* polytope limit */
    ) {

      // find the edge closest to the origin
      for (u8 vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++) {
        u8 Aindex = vertexIndex;
        u8 Bindex = (vertexIndex + 1) % vertexCount;
        v2 A = polytope[Aindex];
        v2 B = polytope[Bindex];
        v2 AB = v2_sub(B, A);
        v2 normal = v2_normalize(v2_perp(AB));
        debug_assert(!(normal.x == 0 && normal.y == 0));
        f32 distance = v2_dot(A, normal);
        if (distance < 0) {
          distance *= -1.0f;
          v2_neg_ref(&normal);
        }

        if (distance < minDistance) {
          minDistance = distance;
          minNormal = normal;
          minIndex = Bindex;
        }
      }

      // test if there is point further out in normal's direction
      support = Support(a, b, minNormal);
      f32 supportDistance = v2_dot(support, minNormal);

      f32 epsilon = 0.0001f;
      if (Absolute(supportDistance - minDistance) > epsilon) {
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
        u8 before = minIndex;
        u8 after = vertexCount - before;
        memcpy(polytope + (before + 1), polytope + before, sizeof(*polytope) * after);
        polytope[minIndex] = support;
        vertexCount++;

        minDistance = F32_MAX;
      }
      // else found closest normal
    }

    // TODO: Is contact information correct?
    // see:
    //   - "Dirk Gregorius - Robust Contact Creation for Physics Simulation"
    //     https://gdcvault.com/play/1022193/Physics-for-Game-Programmers-Robust
    //     https://cdn-a.blazestreaming.com/out/v1/a914e787bbfd48af89b2fdc4721cb75c/38549d9a185b445888c6fbc2e2e792e9/50413c5b8a95441e919f0a1579606e47/index.m3u8
    contact->normal = minNormal;
    contact->start = FindFurthestPoint(b, v2_neg(contact->normal));
    contact->end = FindFurthestPoint(a, contact->normal);
    contact->depth = minDistance;
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
