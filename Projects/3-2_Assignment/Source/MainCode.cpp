#include <iostream>         // error handling and output
#include <cstdlib>          // EXIT_FAILURE

#include <GL/glew.h>        // GLEW library
#include "GLFW/glfw3.h"     // GLFW library

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "SceneManager.h"
#include "ViewManager.h"
#include "ShapeMeshes.h"
#include "ShaderManager.h"

// Namespace for declaring global variables
namespace
{
	// Macro for window title
	const char* const WINDOW_TITLE = "3-2 Assignment"; 

	// Main GLFW window
	GLFWwindow* g_Window = nullptr;

	// scene manager object for managing the 3D scene prepare and render
	SceneManager* g_SceneManager = nullptr;
	// shader manager object for dynamic interaction with the shader code
	ShaderManager* g_ShaderManager = nullptr;
	// view manager object for managing the 3D view setup and projection to 2D
	ViewManager* g_ViewManager = nullptr;
}

// Function declarations - all functions that are called manually
// need to be pre-declared at the beginning of the source code.
bool InitializeGLFW();
bool InitializeGLEW();
void FramebufferSizeCallback(GLFWwindow* window, int width, int height); // callback declaration

/***********************************************************
 *  main(int, char*)
 *
 *  This function gets called after the application has been
 *  launched.
 ***********************************************************/
int main(int argc, char* argv[])
{
	// if GLFW fails initialization, then terminate the application
	if (InitializeGLFW() == false)
	{
		return(EXIT_FAILURE);
	}

	// try to create a new shader manager object
	g_ShaderManager = new ShaderManager();
	// try to create a new view manager object
	g_ViewManager = new ViewManager(
		g_ShaderManager);

	// try to create the main display window
	g_Window = g_ViewManager->CreateDisplayWindow(WINDOW_TITLE);
	if (!g_Window)
	{
		std::cerr << "ERROR: Failed to create GLFW window\n";
		glfwTerminate();
		return(EXIT_FAILURE);
	}

	// Register framebuffer size callback for dynamic resizing
	glfwSetFramebufferSizeCallback(g_Window, FramebufferSizeCallback);

	// if GLEW fails initialization, then terminate the application
	if (InitializeGLEW() == false)
	{
		glfwDestroyWindow(g_Window);
		glfwTerminate();
		return(EXIT_FAILURE);
	}

	// load the shader code from the external GLSL files
	g_ShaderManager->LoadShaders(
		"../../Utilities/shaders/vertexShader.glsl",
		"../../Utilities/shaders/fragmentShader.glsl");
	g_ShaderManager->use();

	// try to create a new scene manager object and prepare the 3D scene
	g_SceneManager = new SceneManager(g_ShaderManager);
	g_SceneManager->PrepareScene(g_Window); // pass the window to set initial projection

	// Enable depth testing once
	glEnable(GL_DEPTH_TEST);

	// loop will keep running until the application is closed 
	// or until an error has occurred
	while (!glfwWindowShouldClose(g_Window))
	{
		// Clear the frame and z buffers
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// convert from 3D object space to 2D view
		g_ViewManager->PrepareSceneView();

		// refresh the 3D scene
		g_SceneManager->RenderScene();

		// Flips the the back buffer with the front buffer every frame.
		glfwSwapBuffers(g_Window);

		// query the latest GLFW events
		glfwPollEvents();
	}

	// clear the allocated manager objects from memory
	if (NULL != g_SceneManager)
	{
		delete g_SceneManager;
		g_SceneManager = NULL;
	}
	if (NULL != g_ViewManager)
	{
		delete g_ViewManager;
		g_ViewManager = NULL;
	}
	if (NULL != g_ShaderManager)
	{
		delete g_ShaderManager;
		g_ShaderManager = NULL;
	}

	// destroy the GLFW window and terminate GLFW
	if (g_Window)
	{
		glfwDestroyWindow(g_Window);
	}
	glfwTerminate();

	// Terminates the program successfully
	exit(EXIT_SUCCESS); 
}

/***********************************************************
 *	FramebufferSizeCallback()
 *
 *  This function is called automatically by GLFW whenever
 *  the window is resized. It updates the OpenGL viewport
 *  and notifies the SceneManager to adjust the projection.
 ***********************************************************/
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	// prevent divide-by-zero
	if (height == 0) height = 1;

	// update OpenGL viewport
	glViewport(0, 0, width, height);

	// update SceneManager projection using public method
	if (g_SceneManager)
	{
		g_SceneManager->UpdateProjection(window);
	}
}

/***********************************************************
 *	InitializeGLFW()
 * 
 *  This function is used to initialize the GLFW library.   
 ***********************************************************/
bool InitializeGLFW()
{
	// GLFW: initialize and configure library
	// --------------------------------------
	glfwInit();

#ifdef __APPLE__
	// set the version of OpenGL and profile to use
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
	// set the version of OpenGL and profile to use
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
	// GLFW: end -------------------------------

	return(true);
}

/***********************************************************
 *	InitializeGLEW()
 *
 *  This function is used to initialize the GLEW library.
 ***********************************************************/
bool InitializeGLEW()
{
	// GLEW: initialize
	// -----------------------------------------
	GLenum GLEWInitResult = GLEW_OK;

	// allow experimental features
	glewExperimental = GL_TRUE;

	// try to initialize the GLEW library
	GLEWInitResult = glewInit();
	if (GLEW_OK != GLEWInitResult)
	{
		std::cerr << glewGetErrorString(GLEWInitResult) << std::endl;
		return false;
	}
	// GLEW: end -------------------------------

	// Displays a successful OpenGL initialization message
	std::cout << "INFO: OpenGL Successfully Initialized\n";
	std::cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << "\n" << std::endl;

	return(true);
}
