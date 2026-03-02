///////////////////////////////////////////////////////////////////////////////
// Circle.cpp
// ==========
// Implementation of the Circle (ball) class. Handles movement, wall
// reflection with speed ramping, AABB brick collision, and elastic
// circle-circle collision with color swapping.
//
// AUTHOR : Kyle Gortych
// COURSE : CS-330 – Computational Graphics and Visualization
// DATE   : March 2026
///////////////////////////////////////////////////////////////////////////////

#include "Circle.h"
#include "Brick.h"
#include <GLFW/glfw3.h>
#include <cmath>

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
Circle::Circle(float xx, float yy, float rad, float angle, float spd,
               float r, float g, float b)
    : x(xx), y(yy), radius(rad), speed(spd),
      red(r), green(g), blue(b), active(true)
{
    vx = cosf(angle) * speed;
    vy = sinf(angle) * speed;
}

// ---------------------------------------------------------------------------
// CheckBrickCollision — AABB closest-point method
// ---------------------------------------------------------------------------
bool Circle::CheckBrickCollision(Brick* brk)
{
    if (brk->onoff == OFF || !active) return false;

    float hw = brk->width  / 2.0f;
    float hh = brk->height / 2.0f;

    // Closest point on the brick's AABB to the circle center
    float closestX = fmaxf(brk->x - hw, fminf(x, brk->x + hw));
    float closestY = fmaxf(brk->y - hh, fminf(y, brk->y + hh));

    float dx   = x - closestX;
    float dy   = y - closestY;
    float dist = sqrtf(dx * dx + dy * dy);

    if (dist >= radius) return false; // No overlap

    // Determine which face was hit and reflect accordingly
    if (fabsf(dx) > fabsf(dy))
    {
        vx = -vx; // Side hit — flip horizontal
        x += (dx > 0 ? 1.0f : -1.0f) * (radius - dist + 0.002f);
    }
    else
    {
        vy = -vy; // Top/bottom hit — flip vertical
        y += (dy > 0 ? 1.0f : -1.0f) * (radius - dist + 0.002f);
    }

    if (brk->brick_type == REFLECTIVE)
    {
        // Small speed boost on reflective walls
        speed = fminf(speed * 1.02f, MAX_SPEED);
    }
    else
    {
        // Friction on destructable bricks slows the ball slightly
        speed = fmaxf(speed * 0.97f, MIN_SPEED);
        brk->TakeHit();
    }
    normalizeVelocity();
    return true;
}

// ---------------------------------------------------------------------------
// CheckCircleCollision — elastic collision + color swap
// ---------------------------------------------------------------------------
bool Circle::CheckCircleCollision(Circle& other)
{
    if (!active || !other.active) return false;

    float dx   = x - other.x;
    float dy   = y - other.y;
    float dist = sqrtf(dx * dx + dy * dy);
    float minD = radius + other.radius;

    if (dist >= minD || dist < 0.0001f) return false;

    // Unit normal from other → this
    float nx = dx / dist;
    float ny = dy / dist;

    // Separate overlapping circles
    float overlap = minD - dist;
    x       += nx * overlap * 0.5f;
    y       += ny * overlap * 0.5f;
    other.x -= nx * overlap * 0.5f;
    other.y -= ny * overlap * 0.5f;

    // Relative velocity projected onto collision normal
    float relVn = (vx - other.vx) * nx + (vy - other.vy) * ny;

    // Apply impulse (equal mass elastic collision)
    vx       -= relVn * nx;
    vy       -= relVn * ny;
    other.vx += relVn * nx;
    other.vy += relVn * ny;

    // State change: swap colors on collision
    float tr = red,   tg = green,   tb = blue;
    red   = other.red;   green = other.green;   blue  = other.blue;
    other.red = tr;      other.green = tg;       other.blue = tb;

    return true;
}

// ---------------------------------------------------------------------------
// MoveOneStep — advance position; reflect off 3 walls, fall through bottom
// ---------------------------------------------------------------------------
void Circle::MoveOneStep()
{
    if (!active) return;

    x += vx;
    y += vy;

    // Right wall
    if (x + radius > BOUND)
    {
        x  = BOUND - radius;
        vx = -vx;
        speed = fminf(speed * 1.03f, MAX_SPEED);
        normalizeVelocity();
    }
    // Left wall
    if (x - radius < -BOUND)
    {
        x  = -BOUND + radius;
        vx = -vx;
        speed = fminf(speed * 1.03f, MAX_SPEED);
        normalizeVelocity();
    }
    // Top wall
    if (y + radius > BOUND)
    {
        y  = BOUND - radius;
        vy = -vy;
        speed = fminf(speed * 1.03f, MAX_SPEED);
        normalizeVelocity();
    }
    // Bottom — ball falls through (death zone). Mark inactive for removal.
    if (y - radius < -BOUND)
    {
        active = false;
    }
}

// ---------------------------------------------------------------------------
// DrawCircle — filled circle via GL_TRIANGLE_FAN + outline
// ---------------------------------------------------------------------------
void Circle::DrawCircle()
{
    if (!active) return;

    // Filled body
    glColor3f(red, green, blue);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 360; i += 8)
    {
        float rad = i * DEG2RAD;
        glVertex2f(cosf(rad) * radius + x, sinf(rad) * radius + y);
    }
    glEnd();

    // Darker outline
    glColor3f(red * 0.5f, green * 0.5f, blue * 0.5f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 360; i += 8)
    {
        float rad = i * DEG2RAD;
        glVertex2f(cosf(rad) * radius + x, sinf(rad) * radius + y);
    }
    glEnd();
}

// ---------------------------------------------------------------------------
// normalizeVelocity — keep direction, scale magnitude to current speed
// ---------------------------------------------------------------------------
void Circle::normalizeVelocity()
{
    float mag = sqrtf(vx * vx + vy * vy);
    if (mag > 0.0001f)
    {
        vx = (vx / mag) * speed;
        vy = (vy / mag) * speed;
    }
}
