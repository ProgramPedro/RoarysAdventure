#pragma once

// Axis-aligned bounding boxes and the one movement routine both levels use.
// Movement is resolved one axis at a time rather than all three at once, which
// removes the ambiguity about whether a body should be pushed sideways or stood
// on top, and lets the caller react differently to landing and to hitting a wall.

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

inline bool overlaps(const Aabb& a, const Aabb& b) {
    const glm::vec3 aMin = a.min();
    const glm::vec3 aMax = a.max();
    const glm::vec3 bMin = b.min();
    const glm::vec3 bMax = b.max();

    return aMin.x < bMax.x && aMax.x > bMin.x &&
           aMin.y < bMax.y && aMax.y > bMin.y &&
           aMin.z < bMax.z && aMax.z > bMin.z;
}

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

                if (delta > 0.0f) {
                    box.center[axis] = solid.min()[axis] - box.halfExtents[axis];
                } else {
                    box.center[axis] = solid.max()[axis] + box.halfExtents[axis];
                }
                hitThisStep = true;
            }
        }

        if (hitThisStep) {
            return true;
        }
    }

    return false;
}
