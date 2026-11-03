#pragma once
//=============================================================================
// Collision - axis-aligned bounding boxes and the one movement routine the
// levels need.
//
// WHY PER-AXIS: moving a body along all three axes at once and then trying to
// untangle it from whatever it hit is ambiguous - you cannot tell whether the
// body should be pushed out sideways or stood on top. Moving ONE axis at a
// time and resolving immediately removes the ambiguity: if you were moving
// down and you overlap something, you landed on it. That is also what lets the
// caller react differently per axis (landing sets "grounded", hitting a wall
// just stops horizontal speed).
//=============================================================================

#include <glm/vec3.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

struct Aabb {
    glm::vec3 center{0.0f};
    glm::vec3 halfExtents{0.5f};

    static Aabb fromCenterSize(const glm::vec3& center, const glm::vec3& size) {
        return Aabb{center, size * 0.5f};
    }

    glm::vec3 min() const { return center - halfExtents; }
    glm::vec3 max() const { return center + halfExtents; }

    float top() const    { return center.y + halfExtents.y; }
    float bottom() const { return center.y - halfExtents.y; }
};

// Standard separating-axis test: the boxes overlap only if they overlap on all
// three axes at once.
inline bool overlaps(const Aabb& a, const Aabb& b) {
    const glm::vec3 aMin = a.min();
    const glm::vec3 aMax = a.max();
    const glm::vec3 bMin = b.min();
    const glm::vec3 bMax = b.max();

    return aMin.x < bMax.x && aMax.x > bMin.x &&
           aMin.y < bMax.y && aMax.y > bMin.y &&
           aMin.z < bMax.z && aMax.z > bMin.z;
}

// True if the two boxes overlap on the two axes that are NOT `axis`. Used to
// ask "is the player standing over this platform", ignoring height.
inline bool overlapsOnOtherAxes(const Aabb& a, const Aabb& b, int axis) {
    const glm::vec3 aMin = a.min();
    const glm::vec3 aMax = a.max();
    const glm::vec3 bMin = b.min();
    const glm::vec3 bMax = b.max();

    for (int i = 0; i < 3; ++i) {
        if (i == axis) continue;
        if (aMin[i] >= bMax[i] || aMax[i] <= bMin[i]) return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
// Moves `box` by `delta` along a single axis (0 = X, 1 = Y, 2 = Z) and pushes
// it back out of anything it ends up inside.
//
// Returns true if a solid stopped the movement, which is how the caller knows
// to zero that component of velocity. When moving down, a true result means
// the body landed.
//
// TUNNELLING: a single big step can jump clean over a thin platform - the body
// starts above it, ends below it, and never overlaps, so nothing is detected.
// At terminal velocity Roary covers ~0.7 units per frame while the platforms
// are 0.55 thick, so this is not hypothetical. The movement is therefore split
// into steps no longer than kMaxStep, which must stay below the thickness of
// the thinnest solid in the game.
//
// Two resolution passes per step: pushing out of one solid can shove the body
// into a neighbour, and a second pass settles that.
//-----------------------------------------------------------------------------
inline bool moveAxis(Aabb& box, int axis, float delta,
                     const std::vector<Aabb>& solids) {
    if (delta == 0.0f) {
        return false;
    }

    constexpr float kMaxStep = 0.20f;

    const float distance = std::abs(delta);
    const int steps = std::max(1, static_cast<int>(std::ceil(distance / kMaxStep)));
    const float step = delta / static_cast<float>(steps);

    for (int i = 0; i < steps; ++i) {
        box.center[axis] += step;

        bool hitThisStep = false;
        for (int pass = 0; pass < 2; ++pass) {
            for (const Aabb& solid : solids) {
                if (!overlaps(box, solid)) continue;

                // Push back against the direction of travel, so the body comes
                // to rest flush against the face it ran into.
                if (delta > 0.0f) {
                    box.center[axis] = solid.min()[axis] - box.halfExtents[axis];
                } else {
                    box.center[axis] = solid.max()[axis] + box.halfExtents[axis];
                }
                hitThisStep = true;
            }
        }

        // Once something has been hit we are resting against it; carrying on
        // would only push us back into the same surface.
        if (hitThisStep) {
            return true;
        }
    }

    return false;
}
