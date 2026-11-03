#pragma once
//=============================================================================
// Angles.h - small helpers for working with facing directions.
//
// Facing angles in this game are DEGREES OF ROTATION ABOUT THE Y AXIS, and 0
// means looking down +Z (toward the camera). They are produced with
// atan2(x, z) - note x first, because our "forward" is +Z rather than the +X
// a maths class would assume.
//=============================================================================

#include <cmath>

//-----------------------------------------------------------------------------
// The shortest way to turn from one angle to another, as a signed number of
// degrees in -180..180.
//
// Without this, turning from 350 degrees to 10 goes the LONG way round: the
// naive subtraction gives -340, so a character spins almost a full circle to
// reach a heading only 20 degrees away.
//
// The +540 is the trick. It is 360 + 180, so adding it before the modulo
// guarantees a positive value going in (fmod keeps the sign of its argument in
// C++), and subtracting the 180 afterwards recentres the result on zero. That
// gets the answer into -180..180 without an if-statement for each of the ways
// the angle could have wrapped.
//-----------------------------------------------------------------------------
inline float shortestAngleDelta(float fromDeg, float toDeg) {
    return std::fmod(toDeg - fromDeg + 540.0f, 360.0f) - 180.0f;
}
