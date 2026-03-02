///////////////////////////////////////////////////////////////////////////////
// Circle.h
// ========
// Ball entity with continuous velocity for physics-accurate reflection.
// Supports brick collision (AABB), circle-circle elastic collision with
// color swap, and wall bounces with speed ramping.
//
// AUTHOR : Kyle Gortych
// COURSE : CS-330 – Computational Graphics and Visualization
// DATE   : March 2026
///////////////////////////////////////////////////////////////////////////////

#ifndef CIRCLE_H
#define CIRCLE_H

#include "Constants.h"

// Forward declaration — avoids circular include
class Brick;

class Circle
{
public:
    float red, green, blue;
    float radius;
    float x, y;
    float vx, vy;   // Velocity components
    float speed;     // Magnitude of velocity
    bool  active;

    // angle = initial direction in radians, spd = initial speed
    Circle(float xx, float yy, float rad, float angle, float spd,
           float r, float g, float b);

    // AABB vs circle collision; reflects velocity and damages the brick
    bool CheckBrickCollision(Brick* brk);

    // Elastic collision with another circle; swaps colors on contact
    bool CheckCircleCollision(Circle& other);

    // Advances position; reflects off left/right/top walls, falls through bottom
    void MoveOneStep();

    // Renders a filled circle with an outline
    void DrawCircle();

private:
    // Scales velocity vector to match current speed magnitude
    void normalizeVelocity();
};

#endif // CIRCLE_H
