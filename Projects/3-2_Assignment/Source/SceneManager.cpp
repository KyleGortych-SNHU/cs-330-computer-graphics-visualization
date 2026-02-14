///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//  Updated for dynamic framebuffer size
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"
#include <GL/gl.h>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>
#include <GLFW/glfw3.h>

namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  SetTransformations()
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	glm::mat4 scale = glm::scale(scaleXYZ);
	glm::mat4 rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 translation = glm::translate(positionXYZ);

	glm::mat4 modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 ***********************************************************/
void SceneManager::SetShaderColor(float red, float green, float blue, float alpha)
{
	glm::vec4 color(red, green, blue, alpha);

	if (m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, color);
	}
}

/***********************************************************
 *  SetProjection()
 ***********************************************************/
void SceneManager::SetProjection(int width, int height)
{
	if (height == 0) height = 1; // prevent divide-by-zero
	float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	glm::mat4 projection = glm::perspective(
		glm::radians(45.0f),
		aspectRatio,
		0.1f,
		1000.0f
	);

	if (m_pShaderManager)
		m_pShaderManager->setMat4Value("projection", projection);
}

/***********************************************************
 *  UpdateProjection()
 ***********************************************************/
void SceneManager::UpdateProjection(GLFWwindow* window)
{
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	SetProjection(width, height);
}

/***********************************************************
 *  PrepareScene()
 ***********************************************************/
void SceneManager::PrepareScene(GLFWwindow* window)
{
	// Load meshes only once
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadBoxMesh();

	// Set the projection matrix using actual framebuffer size
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	SetProjection(width, height);
}

/***********************************************************
 *  RenderScene()
 ***********************************************************/
void SceneManager::RenderScene()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees, YrotationDegrees, ZrotationDegrees;
	glm::vec3 positionXYZ;

	// setting blue background color
	glClearColor(0.15f, 0.35f, 0.65f, 1.0f);

	// --- Left Platform (Cylinder) ---
	scaleXYZ = glm::vec3(2.0f, 1.0f, 2.0f); // shortest cylinder
	positionXYZ = glm::vec3(-4.0f, 0.5f, 0.0f); // center Y

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.2f, 0.3f, 0.8f, 1.0f);  // blue
	m_basicMeshes->DrawCylinderMesh();

	// --- Purple Sphere on Left Platform ---
	scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);
	positionXYZ = glm::vec3(-4.0f, 2.5f, 0.0f); // cylinder top 1.0 + sphere half-height 1.0
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.6f, 0.3f, 0.9f, 1.0f);  // purple
	m_basicMeshes->DrawSphereMesh();

	// --- Middle Platform (Cylinder) ---
	scaleXYZ = glm::vec3(2.5f, 1.5f, 2.5f); // tallest cylinder
	positionXYZ = glm::vec3(0.0f, 1.5f, 0.0f); // center Y

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.2f, 0.3f, 0.8f, 1.0f);  // blue
	m_basicMeshes->DrawCylinderMesh();

	// --- Yellow Cone on Middle Platform ---
	scaleXYZ = glm::vec3(2.0f, 3.5f, 2.0f);
	positionXYZ = glm::vec3(0.0f, 2.5f, 0.0f); // cylinder top 2.25 + cone half-height 0.75
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 0.0f, 1.0f);  // yellow
	m_basicMeshes->DrawConeMesh();

	// --- Right Platform (Cylinder) ---
	scaleXYZ = glm::vec3(2.0f, 1.0f, 2.0f); // medium cylinder
	positionXYZ = glm::vec3(4.0f, 0.5f, 0.0f); // center Y

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.2f, 0.3f, 0.8f, 1.0f);  // blue
	m_basicMeshes->DrawCylinderMesh();

	// --- Red Cube on Right Platform ---
	scaleXYZ = glm::vec3(2.0f, 2.0f, 2.0f);
	positionXYZ = glm::vec3(4.0f, 2.5f, 0.0f); // cylinder top 1.0 + cube half-height 0.5
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(1.0f, 0.3f, 0.3f, 1.0f);  // red
	m_basicMeshes->DrawBoxMesh();
}
