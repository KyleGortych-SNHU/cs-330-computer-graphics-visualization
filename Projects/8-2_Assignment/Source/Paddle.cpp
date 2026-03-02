///////////////////////////////////////////////////////////////////////////////
// Paddle.cpp
// ==========
// Implementation of the player Paddle. Renders with a highlight edge and
// redirects balls at an angle proportional to the horizontal hit offset.
//
// AUTHOR : Kyle Gortych
// COURSE : CS-330 – Computational Graphics and Visualization
// DATE   : March 2026
///////////////////////////////////////////////////////////////////////////////

#include "Paddle.h"
#include "Circle.h"
#include "Constants.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
Paddle::Paddle(float xx, float yy, float w, float h,
               float r, float g, float b)
    : x(xx), y(yy), width(w), height(h),
      moveSpeed(0.045f), red(r), green(g), blue(b)
{}

// ---------------------------------------------------------------------------
// Movement — clamped to screen bounds
// ---------------------------------------------------------------------------
void Paddle::MoveLeft()
{
    if (x - width / 2.0f > -BOUND)
        x -= moveSpeed;
}

void Paddle::MoveRight()
{
    if (x + width / 2.0f < BOUND)
        x += moveSpeed;
}

// ---------------------------------------------------------------------------
// CheckCollision — AABB vs circle with directional redirect
// ---------------------------------------------------------------------------
bool Paddle::CheckCollision(Circle& c)
{
    if (!c.active) return false;

    float hw = width  / 2.0f;
    float hh = height / 2.0f;

    float closestX = fmaxf(x - hw, fminf(c.x, x + hw));
    float closestY = fmaxf(y - hh, fminf(c.y, y + hh));

    float dx   = c.x - closestX;
    float dy   = c.y - closestY;
    float dist = sqrtf(dx * dx + dy * dy);

    if (dist >= c.radius) return false;

    // Map hit position (-1 left .. +1 right) to an exit angle
    float hitPos = (c.x - x) / hw;
    hitPos = fmaxf(-1.0f, fminf(hitPos, 1.0f));

    // Angle range: ~120° (left) down to ~60° (right) — always upward
    float angle = (PI / 2.0f) + hitPos * (PI / 3.5f);

    c.vx = cosf(angle) * c.speed;
    c.vy = fabsf(sinf(angle) * c.speed); // Ensure upward
    c.y  = y + hh + c.radius + 0.002f;   // Separate from paddle

    return true;
}

// ---------------------------------------------------------------------------
// Draw — filled body + top-edge highlight + outline
// ---------------------------------------------------------------------------
void Paddle::Draw()
{
    float hw = width  / 2.0f;
    float hh = height / 2.0f;

    // Main body
    glColor3f(red, green, blue);
    glBegin(GL_POLYGON);
        glVertex2f(x - hw, y - hh);
        glVertex2f(x + hw, y - hh);
        glVertex2f(x + hw, y + hh);
        glVertex2f(x - hw, y + hh);
    glEnd();

    // Lighter top-edge highlight
    glColor3f(fminf(red + 0.25f, 1.0f),
              fminf(green + 0.25f, 1.0f),
              fminf(blue + 0.25f, 1.0f));
    glLineWidth(2.0f);
    glBegin(GL_LINES);
        glVertex2f(x - hw + 0.005f, y + hh);
        glVertex2f(x + hw - 0.005f, y + hh);
    glEnd();
    glLineWidth(1.0f);

    // Outline
    glColor3f(red * 0.4f, green * 0.4f, blue * 0.4f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x - hw, y - hh);
        glVertex2f(x + hw, y - hh);
        glVertex2f(x + hw, y + hh);
        glVertex2f(x - hw, y + hh);
    glEnd();
}
