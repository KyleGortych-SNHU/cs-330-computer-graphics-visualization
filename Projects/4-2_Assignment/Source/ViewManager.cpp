///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

ViewManager::ViewManager(ShaderManager* pShaderManager)
    : m_pShaderManager(pShaderManager),
      m_pWindow(nullptr),
      m_pCamera(new Camera()),
      mLastX(WINDOW_WIDTH / 2.0f),
      mLastY(WINDOW_HEIGHT / 2.0f),
      mFirstMouse(true),
      mDeltaTime(0.0f),
      mLastFrame(0.0f)
{
    // default camera parameters
    m_pCamera->Position = glm::vec3(0.5f, 5.5f, 10.0f);
    m_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
    m_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
    m_pCamera->Zoom = 80;
    m_pCamera->MovementSpeed = 5.0f;
}

ViewManager::~ViewManager()
{
    if (m_pCamera)
        delete m_pCamera;
    m_pCamera = nullptr;
    m_pShaderManager = nullptr;
    m_pWindow = nullptr;
}

GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, windowTitle, nullptr, nullptr);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);

    // Store pointer to this instance in GLFW window
    glfwSetWindowUserPointer(window, this);

    glfwSetCursorPosCallback(window, Mouse_Position_Callback);
    glfwSetScrollCallback(window, Mouse_Scroll_Wheel_Callback);
    glfwSetFramebufferSizeCallback(window, Window_Resize_Callback);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_pWindow = window;
    return window;
}

void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xPos, double yPos)
{
    ViewManager* instance = GetInstance(window);
    if (!instance || !instance->m_pCamera) return;

    if (instance->mFirstMouse)
    {
        instance->mLastX = xPos;
        instance->mLastY = yPos;
        instance->mFirstMouse = false;
    }

    float xOffset = xPos - instance->mLastX;
    float yOffset = instance->mLastY - yPos;

    instance->mLastX = xPos;
    instance->mLastY = yPos;

    instance->m_pCamera->ProcessMouseMovement(xOffset, yOffset);
}

void ViewManager::Mouse_Scroll_Wheel_Callback(GLFWwindow* window, double xOffset, double yOffset)
{
    ViewManager* instance = GetInstance(window);
    if (instance && instance->m_pCamera)
    {
        instance->m_pCamera->ProcessMouseScroll(static_cast<float>(yOffset));
    }
}

void ViewManager::Window_Resize_Callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void ViewManager::ProcessKeyboardEvents()
{
    if (!m_pCamera) return;

    if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_pWindow, true);

    if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
        m_pCamera->ProcessKeyboard(FORWARD, mDeltaTime);
    if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
        m_pCamera->ProcessKeyboard(BACKWARD, mDeltaTime);
    if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
        m_pCamera->ProcessKeyboard(LEFT, mDeltaTime);
    if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
        m_pCamera->ProcessKeyboard(RIGHT, mDeltaTime);
    if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
        m_pCamera->ProcessKeyboard(UP, mDeltaTime);
    if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
        m_pCamera->ProcessKeyboard(DOWN, mDeltaTime);
}

void ViewManager::PrepareSceneView()
{
    float currentFrame = glfwGetTime();
    mDeltaTime = currentFrame - mLastFrame;
    mLastFrame = currentFrame;

    ProcessKeyboardEvents();

    glm::mat4 view = m_pCamera->GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(m_pCamera->Zoom), 
                                            (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
                                            0.1f, 100.0f);

    if (m_pShaderManager)
    {
        m_pShaderManager->setMat4Value(VIEW_NAME, view);
        m_pShaderManager->setMat4Value(PROJECTION_NAME, projection);
        m_pShaderManager->setVec3Value("viewPosition", m_pCamera->Position);
    }
}

ViewManager* ViewManager::GetInstance(GLFWwindow* window)
{
    return static_cast<ViewManager*>(glfwGetWindowUserPointer(window));
}
