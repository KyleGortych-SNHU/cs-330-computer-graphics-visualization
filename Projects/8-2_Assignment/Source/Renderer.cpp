///////////////////////////////////////////////////////////////////////////////
// Renderer.cpp
// ============
// Implementation of utility rendering functions. Uses legacy OpenGL
// immediate mode for all drawing (no shaders required).
//
// AUTHOR : Kyle Gortych
// COURSE : CS-330 – Computational Graphics and Visualization
// DATE   : March 2026
///////////////////////////////////////////////////////////////////////////////

#include "Renderer.h"
#include "Constants.h"
#include <GLFW/glfw3.h>
#include <cmath>

// ---------------------------------------------------------------------------
// Forward declaration for the per-character drawing helper
// ---------------------------------------------------------------------------
static void drawChar(float cx, float cy, float scale, char ch);

// ---------------------------------------------------------------------------
// drawBackground — subtle vertical gradient
// ---------------------------------------------------------------------------
void drawBackground()
{
    glBegin(GL_QUADS);
        // Bottom — dark blue-gray
        glColor3f(0.05f, 0.05f, 0.12f);
        glVertex2f(-1.0f, -1.0f);
        glVertex2f( 1.0f, -1.0f);
        // Top — slightly lighter
        glColor3f(0.10f, 0.10f, 0.18f);
        glVertex2f( 1.0f,  1.0f);
        glVertex2f(-1.0f,  1.0f);
    glEnd();
}

// ---------------------------------------------------------------------------
// drawText — renders a string using the minimal vector font
// ---------------------------------------------------------------------------
void drawText(float startX, float startY, float scale, const char* text)
{
    float cursor = startX;
    for (int i = 0; text[i] != '\0'; i++)
    {
        drawChar(cursor, startY, scale, text[i]);
        cursor += scale * 1.4f; // Character spacing
    }
}

// ---------------------------------------------------------------------------
// drawChar — minimal vector font drawn with GL_LINES
// ---------------------------------------------------------------------------
// Supports uppercase A-Z, digits 0-9, colon, and space.
// Each character is drawn relative to (cx, cy) with the given scale.
// ---------------------------------------------------------------------------
static void drawChar(float cx, float cy, float s, char ch)
{
    // Macro: draw a line segment from (x1,y1) to (x2,y2), scaled and offset
    #define L(x1,y1,x2,y2) glVertex2f(cx+s*(x1),cy+s*(y1)); \
                            glVertex2f(cx+s*(x2),cy+s*(y2));
    glBegin(GL_LINES);
    switch (ch)
    {
    case 'A': L(0,0,0.3,1) L(0.3,1,0.6,0) L(0.1,0.4,0.5,0.4) break;
    case 'B': L(0,0,0,1) L(0,1,0.5,1) L(0.5,1,0.6,0.8) L(0.6,0.8,0.5,0.5)
              L(0.5,0.5,0,0.5) L(0.5,0.5,0.6,0.3) L(0.6,0.3,0.5,0) L(0.5,0,0,0) break;
    case 'C': L(0.6,1,0,1) L(0,1,0,0) L(0,0,0.6,0) break;
    case 'D': L(0,0,0,1) L(0,1,0.4,1) L(0.4,1,0.6,0.7) L(0.6,0.7,0.6,0.3)
              L(0.6,0.3,0.4,0) L(0.4,0,0,0) break;
    case 'E': L(0,0,0,1) L(0,1,0.6,1) L(0,0.5,0.5,0.5) L(0,0,0.6,0) break;
    case 'F': L(0,0,0,1) L(0,1,0.6,1) L(0,0.5,0.5,0.5) break;
    case 'G': L(0.6,1,0,1) L(0,1,0,0) L(0,0,0.6,0) L(0.6,0,0.6,0.5) L(0.6,0.5,0.3,0.5) break;
    case 'H': L(0,0,0,1) L(0,0.5,0.6,0.5) L(0.6,0,0.6,1) break;
    case 'I': L(0,1,0,0) break;
    case 'K': L(0,0,0,1) L(0,0.5,0.6,1) L(0,0.5,0.6,0) break;
    case 'L': L(0,1,0,0) L(0,0,0.6,0) break;
    case 'M': L(0,0,0,1) L(0,1,0.3,0.5) L(0.3,0.5,0.6,1) L(0.6,1,0.6,0) break;
    case 'N': L(0,0,0,1) L(0,1,0.6,0) L(0.6,0,0.6,1) break;
    case 'O': L(0,0,0,1) L(0,1,0.6,1) L(0.6,1,0.6,0) L(0.6,0,0,0) break;
    case 'P': L(0,0,0,1) L(0,1,0.6,1) L(0.6,1,0.6,0.5) L(0.6,0.5,0,0.5) break;
    case 'R': L(0,0,0,1) L(0,1,0.6,1) L(0.6,1,0.6,0.5) L(0.6,0.5,0,0.5) L(0,0.5,0.6,0) break;
    case 'S': L(0.6,1,0,1) L(0,1,0,0.5) L(0,0.5,0.6,0.5) L(0.6,0.5,0.6,0) L(0.6,0,0,0) break;
    case 'T': L(0,1,0.6,1) L(0.3,1,0.3,0) break;
    case 'U': L(0,1,0,0) L(0,0,0.6,0) L(0.6,0,0.6,1) break;
    case 'V': L(0,1,0.3,0) L(0.3,0,0.6,1) break;
    case 'W': L(0,1,0.15,0) L(0.15,0,0.3,0.6) L(0.3,0.6,0.45,0) L(0.45,0,0.6,1) break;
    case 'X': L(0,0,0.6,1) L(0,1,0.6,0) break;
    case 'Y': L(0,1,0.3,0.5) L(0.3,0.5,0.6,1) L(0.3,0.5,0.3,0) break;
    case ':': L(0.25,0.7,0.35,0.7) L(0.25,0.3,0.35,0.3) break;
    case ' ': break;
    // Digits
    case '0': L(0,0,0,1) L(0,1,0.6,1) L(0.6,1,0.6,0) L(0.6,0,0,0) break;
    case '1': L(0.3,0,0.3,1) break;
    case '2': L(0,1,0.6,1) L(0.6,1,0.6,0.5) L(0.6,0.5,0,0.5) L(0,0.5,0,0) L(0,0,0.6,0) break;
    case '3': L(0,1,0.6,1) L(0.6,1,0.6,0) L(0.6,0,0,0) L(0,0.5,0.6,0.5) break;
    case '4': L(0,1,0,0.5) L(0,0.5,0.6,0.5) L(0.6,1,0.6,0) break;
    case '5': L(0.6,1,0,1) L(0,1,0,0.5) L(0,0.5,0.6,0.5) L(0.6,0.5,0.6,0) L(0.6,0,0,0) break;
    case '6': L(0.6,1,0,1) L(0,1,0,0) L(0,0,0.6,0) L(0.6,0,0.6,0.5) L(0.6,0.5,0,0.5) break;
    case '7': L(0,1,0.6,1) L(0.6,1,0.6,0) break;
    case '8': L(0,0,0,1) L(0,1,0.6,1) L(0.6,1,0.6,0) L(0.6,0,0,0) L(0,0.5,0.6,0.5) break;
    case '9': L(0,0.5,0,1) L(0,1,0.6,1) L(0.6,1,0.6,0) L(0.6,0,0,0) L(0,0.5,0.6,0.5) break;
    default: break;
    }
    glEnd();
    #undef L
}

// ---------------------------------------------------------------------------
// drawLivesIndicator — heart icons in the bottom-left corner
// ---------------------------------------------------------------------------
void drawLivesIndicator(int lives)
{
    float startX = -0.95f;
    float baseY  = -0.95f;
    float size   = 0.025f;

    for (int i = 0; i < lives; i++)
    {
        float hx = startX + i * (size * 3.5f);

        glColor3f(1.0f, 0.2f, 0.3f);

        // Left bump
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(hx - size * 0.5f, baseY + size * 0.5f);
        for (int a = 0; a <= 360; a += 30)
        {
            float rad = a * DEG2RAD;
            glVertex2f(cosf(rad) * size * 0.5f + hx - size * 0.5f,
                       sinf(rad) * size * 0.5f + baseY + size * 0.5f);
        }
        glEnd();

        // Right bump
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(hx + size * 0.5f, baseY + size * 0.5f);
        for (int a = 0; a <= 360; a += 30)
        {
            float rad = a * DEG2RAD;
            glVertex2f(cosf(rad) * size * 0.5f + hx + size * 0.5f,
                       sinf(rad) * size * 0.5f + baseY + size * 0.5f);
        }
        glEnd();

        // Bottom triangle
        glBegin(GL_TRIANGLES);
            glVertex2f(hx - size, baseY + size * 0.4f);
            glVertex2f(hx + size, baseY + size * 0.4f);
            glVertex2f(hx, baseY - size * 0.8f);
        glEnd();
    }
}
