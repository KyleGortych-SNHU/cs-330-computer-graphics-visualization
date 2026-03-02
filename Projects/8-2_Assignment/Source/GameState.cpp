///////////////////////////////////////////////////////////////////////////////
// GameState.cpp
// =============
// Implementation of central game-state management. Owns the global
// collections (balls, bricks, paddle) and provides functions for layout
// construction, ball spawning, dead-ball removal, and input processing.
//
// AUTHOR : Kyle Gortych
// COURSE : CS-330 – Computational Graphics and Visualization
// DATE   : March 2026
///////////////////////////////////////////////////////////////////////////////

#include "GameState.h"
#include "Constants.h"
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <cmath>

using std::vector;

// ---------------------------------------------------------------------------
// Global state definitions
// ---------------------------------------------------------------------------
vector<Circle> world;
vector<Brick>  bricks;
Paddle* paddle   = nullptr;
int  lives       = 3;
bool gameOver    = false;
bool gameWon     = false;

// Internal — tracks space-bar edge to prevent rapid-fire spawning
static bool spaceHeld = false;

// ---------------------------------------------------------------------------
// buildBrickLayout
// ----------------
// Pyramid arrangement of colored rows with increasing hit points toward
// the top. Gold reflective bricks line both side walls.
// ---------------------------------------------------------------------------
void buildBrickLayout()
{
    bricks.clear();

    // Row configuration: { y-center, brick count, hit-points, R, G, B }
    struct RowDef { float y; int count; int hp; float r, g, b; };
    RowDef rows[] = {
        { 0.88f, 10, 3, 1.0f, 0.2f, 0.2f },  // Top — red, 3 hits
        { 0.76f, 10, 3, 1.0f, 0.4f, 0.1f },  // Orange, 3 hits
        { 0.64f,  9, 2, 1.0f, 0.8f, 0.1f },  // Yellow, 2 hits
        { 0.52f,  9, 2, 0.2f, 0.9f, 0.2f },  // Green, 2 hits
        { 0.40f,  8, 1, 0.2f, 0.6f, 1.0f },  // Blue, 1 hit
        { 0.28f,  8, 1, 0.6f, 0.3f, 0.9f },  // Purple, 1 hit
    };
    int numRows = sizeof(rows) / sizeof(rows[0]);

    float brickW = 0.17f;  // Brick width
    float brickH = 0.06f;  // Brick height
    float gap    = 0.02f;  // Gap between bricks

    for (int r = 0; r < numRows; r++)
    {
        int   count = rows[r].count;
        float rowW  = count * brickW + (count - 1) * gap;
        float startX = -rowW / 2.0f + brickW / 2.0f;

        for (int c = 0; c < count; c++)
        {
            float bx = startX + c * (brickW + gap);
            bricks.emplace_back(
                DESTRUCTABLE, bx, rows[r].y, brickW, brickH,
                rows[r].r, rows[r].g, rows[r].b, rows[r].hp
            );
        }
    }

    // Reflective (gold) wall bricks on left and right edges
    float wallW = 0.04f;
    float wallH = 0.08f;
    for (float wy = -0.9f; wy <= 0.95f; wy += wallH + 0.01f)
    {
        bricks.emplace_back(REFLECTIVE, -0.97f, wy, wallW, wallH,
                            0.85f, 0.7f, 0.2f, 1);
        bricks.emplace_back(REFLECTIVE,  0.97f, wy, wallW, wallH,
                            0.85f, 0.7f, 0.2f, 1);
    }
}

// ---------------------------------------------------------------------------
// spawnBall — random upward angle and vivid color
// ---------------------------------------------------------------------------
void spawnBall(float startX, float startY)
{
    // Random upward angle between 30 and 150 degrees
    float angle = (30.0f + static_cast<float>(rand() % 120)) * DEG2RAD;
    float spd   = 0.018f + static_cast<float>(rand() % 10) * 0.001f;

    // Random vivid color
    float r = 0.3f + static_cast<float>(rand() % 70) / 100.0f;
    float g = 0.3f + static_cast<float>(rand() % 70) / 100.0f;
    float b = 0.3f + static_cast<float>(rand() % 70) / 100.0f;

    world.emplace_back(startX, startY, 0.025f, angle, spd, r, g, b);
}

// ---------------------------------------------------------------------------
// removeDeadBalls — erase inactive entries from the world vector
// ---------------------------------------------------------------------------
void removeDeadBalls()
{
    for (int i = static_cast<int>(world.size()) - 1; i >= 0; i--)
    {
        if (!world[i].active)
            world.erase(world.begin() + i);
    }
}

// ---------------------------------------------------------------------------
// countActiveBricks — number of destructable bricks still ON
// ---------------------------------------------------------------------------
int countActiveBricks()
{
    int count = 0;
    for (size_t i = 0; i < bricks.size(); i++)
    {
        if (bricks[i].brick_type == DESTRUCTABLE && bricks[i].onoff == ON)
            count++;
    }
    return count;
}

// ---------------------------------------------------------------------------
// processInput
// ------------
// ESC         → Close window
// A / LEFT    → Move paddle left
// D / RIGHT   → Move paddle right
// SPACE       → Spawn ball (one per press) / Restart on end screens
// ---------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    // Exit
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Restart from game-over or win screen
    if (gameOver || gameWon)
    {
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            if (!spaceHeld)
            {
                lives    = 3;
                gameOver = false;
                gameWon  = false;
                world.clear();
                buildBrickLayout();
                paddle->x = 0.0f;
                spawnBall(paddle->x - 0.10f, paddle->y + 0.10f);
                spawnBall(paddle->x,         paddle->y + 0.10f);
                spawnBall(paddle->x + 0.10f, paddle->y + 0.10f);
                spaceHeld = true;
            }
        }
        else
        {
            spaceHeld = false;
        }
        return; // Don't process gameplay input during end screens
    }

    // Paddle movement (continuous while held)
    if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_A)     == GLFW_PRESS)
    {
        paddle->MoveLeft();
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_D)     == GLFW_PRESS)
    {
        paddle->MoveRight();
    }

    // Spawn ball — rising edge of SPACE only
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        if (!spaceHeld && world.size() < 25)
        {
            spawnBall(paddle->x, paddle->y + 0.08f);
            spaceHeld = true;
        }
    }
    else
    {
        spaceHeld = false;
    }
}
