///////////////////////////////////////////////////////////////////////////////
// Brick.h
// =======
// Rectangular brick with multi-hit support, color degradation, and crack
// lines that appear as damage accumulates.
//
// AUTHOR : Kyle Gortych
// COURSE : CS-330 – Computational Graphics and Visualization
// DATE   : March 2026
///////////////////////////////////////////////////////////////////////////////

#ifndef BRICK_H
#define BRICK_H

#include "Constants.h"

class Brick
{
public:
    float red, green, blue;              // Current rendered color
    float origRed, origGreen, origBlue;  // Original color for interpolation
    float x, y;                          // Center position (normalized coords)
    float width, height;                 // Full width and height
    BRICKTYPE brick_type;
    ONOFF onoff;
    int hitPoints;                       // Remaining hits before destruction
    int maxHitPoints;                    // Starting hit count

    // Constructor
    Brick(BRICKTYPE bt, float xx, float yy, float ww, float hh,
          float rr, float gg, float bb, int hp = 1);

    // Reduces hit points; returns true when the brick is destroyed
    bool TakeHit();

    // Renders the brick (filled quad + outline + crack lines if damaged)
    void drawBrick();

private:
    // Draws dark crack lines that increase with damage level
    void drawCracks(float hw, float hh);
};

#endif // BRICK_H
