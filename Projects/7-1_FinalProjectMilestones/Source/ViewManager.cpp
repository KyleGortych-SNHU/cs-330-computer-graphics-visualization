///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//=================================================================
// Constructor
//=================================================================
ViewManager::ViewManager(ShaderManager* pShaderManager)
{
    m_pShaderManager = pShaderManager;
    m_pWindow = nullptr;

    m_pCamera = new Camera();
    m_pCamera->Position = glm::vec3(0.0f, 4.5f, 12.0f);
    m_pCamera->Front = glm::vec3(0.0f, -0.3f, -1.0f);
    m_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
    m_pCamera->Zoom = 60.0f;

    mDeltaTime = 0.0f;
    mLastFrame = 0.0f;
    mLastX = 1000 / 2.0f;
    mLastY = 800 / 2.0f;
    mFirstMouse = true;
    mOrthographic = false;
}

//=================================================================
// Destructor
//=================================================================
ViewManager::~ViewManager()
{
    if (m_pCamera) delete m_pCamera;
    m_pCamera = nullptr;
    m_pWindow = nullptr;
    m_pShaderManager = nullptr;
}

//=================================================================
// CreateDisplayWindow
//=================================================================
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
    GLFWwindow* window = glfwCreateWindow(1000, 800, windowTitle, nullptr, nullptr);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_pWindow = window;

    // Store this instance for static callbacks
    glfwSetWindowUserPointer(window, this);

    // Correct static callback names
    glfwSetCursorPosCallback(window, MousePositionCallback);
    glfwSetScrollCallback(window, MouseScrollCallback);
    glfwSetFramebufferSizeCallback(window, WindowResizeCallback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    return window;
}

//=================================================================
// PrepareSceneView
//=================================================================
void ViewManager::PrepareSceneView()
{
    float currentFrame = static_cast<float>(glfwGetTime());
    mDeltaTime = currentFrame - mLastFrame;
    mLastFrame = currentFrame;

    ProcessKeyboardEvents();
    UpdateShaderMatrices();
}

//=================================================================
// ProcessKeyboardEvents
//=================================================================
void ViewManager::ProcessKeyboardEvents()
{
    if (!m_pWindow) return;

    if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
        m_pCamera->ProcessKeyboard(FORWARD, mDeltaTime);
    if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
        m_pCamera->ProcessKeyboard(BACKWARD, mDeltaTime);
    if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
        m_pCamera->ProcessKeyboard(LEFT, mDeltaTime);
    if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
        m_pCamera->ProcessKeyboard(RIGHT, mDeltaTime);
    if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
        m_pCamera->ProcessKeyboard(DOWN, mDeltaTime);
    if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
        m_pCamera->ProcessKeyboard(UP, mDeltaTime);

    if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
        mOrthographic = false;
    if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS)
        mOrthographic = true;

    if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_pWindow, true);
}

//=================================================================
// UpdateShaderMatrices
//=================================================================
void ViewManager::UpdateShaderMatrices()
{
    if (!m_pShaderManager || !m_pCamera || !m_pWindow) return;

    glm::mat4 view = m_pCamera->GetViewMatrix();

    int width, height;
    glfwGetFramebufferSize(m_pWindow, &width, &height);
    if (height == 0) height = 1;
    float aspect = static_cast<float>(width) / static_cast<float>(height);

    glm::mat4 projection;
    if (mOrthographic)
    {
        float size = 20.0f;
        projection = glm::ortho(-size * aspect, size * aspect, -size, size, 0.1f, 1000.0f);
    }
    else
    {
        projection = glm::perspective(glm::radians(m_pCamera->Zoom), aspect, 0.1f, 1000.0f);
    }

    m_pShaderManager->setMat4Value("view", view);
    m_pShaderManager->setMat4Value("projection", projection);
    m_pShaderManager->setVec3Value("viewPosition", m_pCamera->Position);
    m_pShaderManager->setVec3Value("viewPos", m_pCamera->Position);  // PBR lighting needs this
}

//=================================================================
// GLFW Callbacks
//=================================================================
void ViewManager::MousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    ViewManager* vm = reinterpret_cast<ViewManager*>(glfwGetWindowUserPointer(window));
    if (!vm || !vm->m_pCamera) return;

    if (vm->mFirstMouse)
    {
        vm->mLastX = static_cast<float>(xpos);
        vm->mLastY = static_cast<float>(ypos);
        vm->mFirstMouse = false;
    }

    float xoffset = static_cast<float>(xpos) - vm->mLastX;
    float yoffset = vm->mLastY - static_cast<float>(ypos); // reversed
    vm->mLastX = static_cast<float>(xpos);
    vm->mLastY = static_cast<float>(ypos);

    vm->m_pCamera->ProcessMouseMovement(xoffset, yoffset);
}

void ViewManager::MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    ViewManager* vm = reinterpret_cast<ViewManager*>(glfwGetWindowUserPointer(window));
    if (!vm || !vm->m_pCamera) return;

    vm->m_pCamera->ProcessMouseScroll(static_cast<float>(yoffset));
}

void ViewManager::WindowResizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
