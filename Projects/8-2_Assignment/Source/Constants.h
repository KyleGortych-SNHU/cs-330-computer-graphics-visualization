///////////////////////////////////////////////////////////////////////////////
// Constants.h
// ===========
// Shared constants and enumerations for the 2D Breakout animation.
//
// AUTHOR : Kyle Gortych
// COURSE : CS-330 – Computational Graphics and Visualization
// DATE   : March 2026
///////////////////////////////////////////////////////////////////////////////

#ifndef CONSTANTS_H
#define CONSTANTS_H

// ---------------------------------------------------------------------------
// Math constants
// ---------------------------------------------------------------------------
static const float PI      = 3.14159265f;
static const float DEG2RAD = PI / 180.0f;

// ---------------------------------------------------------------------------
// Gameplay constants
// ---------------------------------------------------------------------------
static const float BOUND     = 1.0f;    // Screen boundary (normalized coords)
static const float MAX_SPEED = 0.055f;  // Maximum ball speed clamp
static const float MIN_SPEED = 0.012f;  // Minimum ball speed clamp

// ---------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------
enum BRICKTYPE { REFLECTIVE, DESTRUCTABLE };
enum ONOFF     { ON, OFF };

#endif // CONSTANTS_H
