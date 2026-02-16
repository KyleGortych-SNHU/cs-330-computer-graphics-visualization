///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ShaderManager.h"
#include "camera.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class ViewManager
{
public:
    // constructor
    ViewManager(ShaderManager* pShaderManager);
    // destructor
    ~ViewManager();

    // create the initial OpenGL display window
    GLFWwindow* CreateDisplayWindow(const char* windowTitle);

    // prepare the per-frame scene view and camera
    void PrepareSceneView();

    // handle GLFW callbacks
    static void MousePositionCallback(GLFWwindow* window, double xpos, double ypos);
    static void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void WindowResizeCallback(GLFWwindow* window, int width, int height);

    // toggle between perspective and orthographic views
    void ToggleProjection(bool orthographic);

private:
    ShaderManager* m_pShaderManager; // pointer to shader manager
    GLFWwindow* m_pWindow;           // active OpenGL window
    Camera* m_pCamera;               // camera for 3D navigation

    // timing
    float mDeltaTime;
    float mLastFrame;

    // mouse tracking
    float mLastX;
    float mLastY;
    bool mFirstMouse;

    // process keyboard input
    void ProcessKeyboardEvents();

    // update shader matrices
    void UpdateShaderMatrices();

    // helper to retrieve ViewManager instance from GLFW window
    static ViewManager* GetInstance(GLFWwindow* window);

    // projection mode
    bool mOrthographic;
};
