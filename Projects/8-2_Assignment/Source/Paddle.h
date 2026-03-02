///////////////////////////////////////////////////////////////////////////////
// Paddle.h
// ========
// Player-controlled paddle. Ball reflection angle depends on the hit
// position relative to the paddle center, giving directional control.
//
// AUTHOR : Kyle Gortych
// COURSE : CS-330 – Computational Graphics and Visualization
// DATE   : March 2026
///////////////////////////////////////////////////////////////////////////////

#ifndef PADDLE_H
#define PADDLE_H

// Forward declaration
class Circle;

class Paddle
{
public:
    float x, y;
    float width, height;
    float moveSpeed;
    float red, green, blue;

    Paddle(float xx, float yy, float w, float h,
           float r, float g, float b);

    void MoveLeft();
    void MoveRight();

    // AABB vs circle; redirects ball upward at a position-dependent angle
    bool CheckCollision(Circle& c);

    void Draw();
};

#endif // PADDLE_H
