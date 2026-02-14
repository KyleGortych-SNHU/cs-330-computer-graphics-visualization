///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../../../Utilities/ShaderManager.h"
#include "../../../Utilities/camera.h"
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

    // prepare the conversion from 3D object display to 2D scene display
    void PrepareSceneView();

    // static callbacks for GLFW
    static void Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos);
    static void Mouse_Scroll_Wheel_Callback(GLFWwindow* window, double xOffset, double yOffset);
    static void Window_Resize_Callback(GLFWwindow* window, int width, int height);

private:
    // pointer to shader manager object
    ShaderManager* m_pShaderManager;
    // active OpenGL display window
    GLFWwindow* m_pWindow;

    // camera for 3D viewing
    Camera* m_pCamera;

    // timing
    float mDeltaTime;
    float mLastFrame;

    // mouse tracking
    float mLastX;
    float mLastY;
    bool mFirstMouse;

    // process keyboard events for interaction with the 3D scene
    void ProcessKeyboardEvents();

    // constants
    static constexpr int WINDOW_WIDTH = 1000;
    static constexpr int WINDOW_HEIGHT = 800;
    static constexpr const char* VIEW_NAME = "view";
    static constexpr const char* PROJECTION_NAME = "projection";

    // helper to get the ViewManager instance from GLFW window
    static ViewManager* GetInstance(GLFWwindow* window);
};
