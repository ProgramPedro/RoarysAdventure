#pragma once
//=============================================================================
// Characters.h - drawing code shared by more than one scene.
//
// WHY THIS FILE EXISTS
//
// Roary is drawn in the stealth level and again in the boss level. The rival
// mascots are drawn on the level-select tower and will be drawn in the boss
// fight. Code that two scenes both need does not belong inside either of them,
// or the day you change Roary's jersey you have to remember to change it
// twice - and the two copies quietly drift apart.
//
// So anything that draws a CHARACTER and is wanted by more than one screen
// lives here, as plain functions that take a Renderer. They hold no state of
// their own: everything they need is passed in, which is what makes them
// usable from any scene.
//=============================================================================

#include <glm/vec3.hpp>

class Renderer;

//-----------------------------------------------------------------------------
// The shortest way to turn from one angle to another, in degrees, as a signed
// number in -180..180.
//
// Without this, turning from 350 degrees to 10 goes the LONG way round - the
// naive subtraction gives -340, so the character spins almost a full circle to
// reach a heading 20 degrees away. Every turning character in the game runs
// its angle through here.
//-----------------------------------------------------------------------------
float shortestAngleDelta(float fromDeg, float toDeg);

//-----------------------------------------------------------------------------
// Draws Roary standing with his feet at `feetPosition`.
//
//   facingDeg    which way he looks, degrees about Y. 0 looks down +Z, toward
//                the camera. Same convention as everything else in the game.
//   walkCycle    a number that grows while he walks and stops when he does.
//                Drives the bounce and the leg/arm swing; pass 0 for a
//                character standing still.
//   walking      whether to animate at all.
//   scale        multiplies his natural height of 1.42 world units.
//   tint         blended over his colours, for effects like flashing white
//                after being hit. Pass 0 for his normal appearance.
//   tintColor    what to blend toward when `tint` is above 0.
//
// He is built from squashed spheres rather than a loaded model because the
// real mascot is a plush costume, and that is what plush looks like.
//-----------------------------------------------------------------------------
void drawRoary(Renderer& renderer,
               const glm::vec3& feetPosition,
               float facingDeg,
               float walkCycle,
               bool walking,
               float scale,
               float tint = 0.0f,
               const glm::vec3& tintColor = glm::vec3(1.0f));
