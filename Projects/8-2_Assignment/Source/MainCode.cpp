/* 
 * MainCode.cpp
 *
 * Entry point for the 2D Breakout-style collision animation.
 * This file contains only main() — all game logic, rendering, and state
 * management are delegated to their respective modules:
 *
 *   Constants.h
 *   Brick.h/.cpp
 *   Circle.h/.cpp
 *   Paddle.h/.cpp
 *   Renderer.h/.cpp
 *   GameState.h/.cpp
 *
 * Maintainer: Kyle Gortych
 * Date: 3/1/2026
 */

#include <GLFW/glfw3.h>
#include <cstdlib>
#include <ctime>
#include <iostream>

#include "Constants.h"
#include "Brick.h"
#include "Circle.h"
#include "Paddle.h"
#include "Renderer.h"
#include "GameState.h"

int main(void) {
    srand(static_cast<unsigned>(time(NULL)));

    // Initialize GLFW 
    if (!glfwInit()) {
        std::cerr << "ERROR: Failed to initialize GLFW" << std::endl;
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(
        640, 640, "CS-330  |  8-2 Breakout Animation", NULL, NULL);

    if (!window) {
        std::cerr << "ERROR: Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // VSync

    // Build initial game world
    buildBrickLayout();
    paddle = new Paddle(0.0f, -0.90f, 0.30f, 0.04f, 0.1f, 0.6f, 0.9f);
    spawnBall(-0.10f, -0.50f);
    spawnBall( 0.00f, -0.50f);
    spawnBall( 0.10f, -0.50f);

    // Enable line smoothing 
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Main game loop 
    while (!glfwWindowShouldClose(window)) {
        // Viewport and clear 
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);
        glClear(GL_COLOR_BUFFER_BIT);

        drawBackground();
        processInput(window);

        // Gameover screen 
        if (gameOver) {
            glColor3f(1.0f, 0.2f, 0.2f);
            glLineWidth(3.0f);
            drawText(-0.45f, 0.15f, 0.09f, "GAME OVER");
            glColor3f(0.8f, 0.8f, 0.8f);
            glLineWidth(1.5f);
            drawText(-0.55f, -0.05f, 0.04f, "PRESS SPACE TO RESTART");
            glLineWidth(1.0f);

            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }

        // Win screen 
        if (gameWon) {
            glColor3f(0.2f, 1.0f, 0.3f);
            glLineWidth(3.0f);
            drawText(-0.35f, 0.15f, 0.09f, "YOU WIN");
            glColor3f(0.8f, 0.8f, 0.8f);
            glLineWidth(1.5f);
            drawText(-0.55f, -0.05f, 0.04f, "PRESS SPACE TO RESTART");
            glLineWidth(1.0f);

            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }

        // Ball vs Brick collisions
        for (size_t i = 0; i < world.size(); i++) {
            for (size_t b = 0; b < bricks.size(); b++) {
                world[i].CheckBrickCollision(&bricks[b]);
            }
        }

        // Ball vs Paddle collision 
        for (size_t i = 0; i < world.size(); i++) {
            paddle->CheckCollision(world[i]);
        }

        // Ball vs Ball collisions
        static int circleCollisionCount = 0;
        for (size_t i = 0; i < world.size(); i++) {
            for (size_t j = i + 1; j < world.size(); j++) {
                if (world[i].CheckCircleCollision(world[j])) {
                    circleCollisionCount++;
                    // Spawn a small bonus ball every 5 circle to circle collision
                    if (circleCollisionCount % 5 == 0 && world.size() < 30) {
                        float midX = (world[i].x + world[j].x) / 2.0f;
                        float midY = (world[i].y + world[j].y) / 2.0f;
                        float ang  = static_cast<float>(rand() % 360) * DEG2RAD;
                        world.emplace_back(
                            midX, midY, 0.015f, ang, 0.02f,
                            0.9f, 0.9f, 0.2f);
                    }
                }
            }
        }

        // Move and draw balls 
        for (size_t i = 0; i < world.size(); i++) {
            world[i].MoveOneStep();
            world[i].DrawCircle();
        }

        // Remove fallen balls and check lives 
        removeDeadBalls();

        if (world.empty()) {
            lives--;
            if (lives <= 0) {
                gameOver = true;
            } else {
                spawnBall(paddle->x - 0.10f, paddle->y + 0.10f);
                spawnBall(paddle->x,         paddle->y + 0.10f);
                spawnBall(paddle->x + 0.10f, paddle->y + 0.10f);
            }
        }

        // Check win condition 
        if (countActiveBricks() == 0) {
            gameWon = true;
        }

        // Draw bricks, paddle, HUD
        for (size_t b = 0; b < bricks.size(); b++) {
            bricks[b].drawBrick();

        paddle->Draw();
        drawLivesIndicator(lives);

        // Swap and poll 
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    delete paddle;
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
