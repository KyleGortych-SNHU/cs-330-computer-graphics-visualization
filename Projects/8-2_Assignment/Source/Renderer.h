///////////////////////////////////////////////////////////////////////////////
// Renderer.h
// ==========
// Utility rendering functions: gradient background, vector-font text,
// and the lives indicator (heart icons).
//
// AUTHOR : Kyle Gortych
// COURSE : CS-330 – Computational Graphics and Visualization
// DATE   : March 2026
///////////////////////////////////////////////////////////////////////////////

#ifndef RENDERER_H
#define RENDERER_H

// Draws a vertical gradient background (dark blue-gray)
void drawBackground();

// Draws a string of uppercase text using a minimal vector font
void drawText(float startX, float startY, float scale, const char* text);

// Draws heart icons in the bottom-left corner, one per remaining life
void drawLivesIndicator(int lives);

#endif // RENDERER_H
