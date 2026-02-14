///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//  Updated for aspect ratio fix
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    

// declarations for global variables and defines
namespace
{
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	Camera* g_pCamera = nullptr;

	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	float gDeltaTime = 0.0f; 
	float gLastFrame = 0.0f;

	bool bOrthographicProjection = false;
}

/***********************************************************
 *  ViewManager()
 ***********************************************************/
ViewManager::ViewManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();
	g_pCamera->Position = glm::vec3(0.0f, 4.5f, 12.0f); // lower camera height
	g_pCamera->Front = glm::vec3(0.0f, -0.3f, -1.0f); // face camera head on
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 60; // reduced field of view
}

/***********************************************************
 *  ~ViewManager()
 ***********************************************************/
ViewManager::~ViewManager()
{
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, windowTitle, NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;
	return window;
}

/***********************************************************
 *  ProcessKeyboardEvents()
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}
}

/***********************************************************
 *  PrepareSceneView()
 *
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process keyboard events
	ProcessKeyboardEvents();

	// get the current view matrix from the camera
	view = g_pCamera->GetViewMatrix();

	// --- FIX: dynamic framebuffer size ---
	int width, height;
	glfwGetFramebufferSize(m_pWindow, &width, &height);
	if (height == 0) height = 1; // prevent divide-by-zero
	float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	projection = glm::perspective(glm::radians(g_pCamera->Zoom), aspectRatio, 0.1f, 100.0f);

	// set matrices into the shader
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ViewName, view);
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
	}
}
