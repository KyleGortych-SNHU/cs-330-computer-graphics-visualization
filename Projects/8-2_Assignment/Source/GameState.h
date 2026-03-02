///////////////////////////////////////////////////////////////////////////////
// GameState.h
// ===========
// Central game state management: globals (balls, bricks, paddle, lives),
// brick layout construction, ball spawning, dead-ball cleanup, input
// handling, and game-reset logic.
//
// AUTHOR : Kyle Gortych
// COURSE : CS-330 – Computational Graphics and Visualization
// DATE   : March 2026
///////////////////////////////////////////////////////////////////////////////

#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <vector>
#include "Circle.h"
#include "Brick.h"
#include "Paddle.h"

struct GLFWwindow; // Forward declaration

// ---------------------------------------------------------------------------
// Global game-state variables (defined in GameState.cpp)
// ---------------------------------------------------------------------------
extern std::vector<Circle> world;   // All active balls
extern std::vector<Brick>  bricks;  // All bricks
extern Paddle* paddle;              // Player paddle
extern int  lives;
extern bool gameOver;
extern bool gameWon;

// ---------------------------------------------------------------------------
// Functions
// ---------------------------------------------------------------------------

// Builds the pyramid brick layout + reflective side walls
void buildBrickLayout();

// Spawns a ball at the given position with a random upward angle and color
void spawnBall(float startX, float startY);

// Removes inactive (fallen) balls from the world vector
void removeDeadBalls();

// Returns the number of destructable bricks still alive
int countActiveBricks();

// Handles keyboard input: paddle movement, ball spawning, restart
void processInput(GLFWwindow* window);

#endif // GAMESTATE_H
