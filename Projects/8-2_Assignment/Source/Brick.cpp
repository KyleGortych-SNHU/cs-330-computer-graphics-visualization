///////////////////////////////////////////////////////////////////////////////
// Brick.cpp
// =========
// Implementation of the Brick class. Handles hit-point tracking, color
// degradation on damage, and rendering with crack overlays.
//
// AUTHOR : Kyle Gortych
// COURSE : CS-330 – Computational Graphics and Visualization
// DATE   : March 2026
///////////////////////////////////////////////////////////////////////////////

#include "Brick.h"
#include <GLFW/glfw3.h>

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
Brick::Brick(BRICKTYPE bt, float xx, float yy, float ww, float hh,
             float rr, float gg, float bb, int hp)
    : brick_type(bt), x(xx), y(yy), width(ww), height(hh),
      red(rr), green(gg), blue(bb),
      origRed(rr), origGreen(gg), origBlue(bb),
      onoff(ON), hitPoints(hp), maxHitPoints(hp)
{}

// ---------------------------------------------------------------------------
// TakeHit — reduces hit points and fades color toward dark red
// ---------------------------------------------------------------------------
bool Brick::TakeHit()
{
    if (brick_type == REFLECTIVE) return false; // Walls never break

    hitPoints--;
    if (hitPoints <= 0)
    {
        onoff = OFF;
        return true; // Brick destroyed
    }

    // Fade color toward dark red as damage increases
    float ratio = static_cast<float>(hitPoints) / static_cast<float>(maxHitPoints);
    red   = origRed   * ratio + 0.7f * (1.0f - ratio);
    green = origGreen * ratio * 0.25f;
    blue  = origBlue  * ratio * 0.25f;
    return false;
}

// ---------------------------------------------------------------------------
// drawBrick — filled quad + outline + crack lines when damaged
// ---------------------------------------------------------------------------
void Brick::drawBrick()
{
    if (onoff == OFF) return;

    float hw = width  / 2.0f;
    float hh = height / 2.0f;

    // Filled rectangle
    glColor3f(red, green, blue);
    glBegin(GL_POLYGON);
        glVertex2f(x - hw, y - hh);
        glVertex2f(x + hw, y - hh);
        glVertex2f(x + hw, y + hh);
        glVertex2f(x - hw, y + hh);
    glEnd();

    // Darker outline for visual definition
    glColor3f(red * 0.4f, green * 0.4f, blue * 0.4f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x - hw, y - hh);
        glVertex2f(x + hw, y - hh);
        glVertex2f(x + hw, y + hh);
        glVertex2f(x - hw, y + hh);
    glEnd();

    // Draw crack lines on damaged destructable bricks
    if (brick_type == DESTRUCTABLE && hitPoints < maxHitPoints)
    {
        drawCracks(hw, hh);
    }
}

// ---------------------------------------------------------------------------
// drawCracks — progressively more cracks as damage increases
// ---------------------------------------------------------------------------
void Brick::drawCracks(float hw, float hh)
{
    float damage = 1.0f - static_cast<float>(hitPoints)
                        / static_cast<float>(maxHitPoints);
    glColor3f(0.08f, 0.08f, 0.08f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);

    // First crack appears at 33% damage
    if (damage >= 0.33f)
    {
        glVertex2f(x - hw * 0.3f, y + hh);
        glVertex2f(x + hw * 0.2f, y - hh * 0.5f);
    }
    // Second crack appears at 66% damage
    if (damage >= 0.66f)
    {
        glVertex2f(x + hw * 0.5f, y + hh);
        glVertex2f(x - hw * 0.1f, y - hh);
    }

    glEnd();
    glLineWidth(1.0f);
}
