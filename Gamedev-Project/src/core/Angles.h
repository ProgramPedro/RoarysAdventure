#pragma once
// Angles.h - helper for facing angles
// Facing angles are degrees around the Y axis, 0 = looking down +Z

#include <cmath>

// Shortest turn from one angle to another, in -180..180
// (so 350 -> 10 is +20, not -340)
inline float shortestAngleDelta(float fromDeg, float toDeg) {
    return std::fmod(toDeg - fromDeg + 540.0f, 360.0f) - 180.0f;
}
