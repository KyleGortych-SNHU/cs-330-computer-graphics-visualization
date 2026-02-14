///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declare the global variables
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
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// clear the allocated memory
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	// destroy the created OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename
			<< ", width:" << width
			<< ", height:" << height
			<< ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
				width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
				width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with "
				<< colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureSlot()
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (!bFound))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}
	return textureSlot;
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
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	scale = glm::scale(scaleXYZ);
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1, 0, 0));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0, 1, 0));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0, 0, 1));
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	m_pShaderManager->setMat4Value(g_ModelName, modelView);
}

/***********************************************************
 *  SetShaderTexture()
 ***********************************************************/
void SceneManager::SetShaderTexture(std::string textureTag)
{
	m_pShaderManager->setIntValue(g_UseTextureName, true);
	int textureSlot = FindTextureSlot(textureTag);
	m_pShaderManager->setSampler2DValue(g_TextureValueName, textureSlot);
}

/***********************************************************
 *  LoadSceneTextures()
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	bReturn = CreateGLTexture("../../Utilities/textures/pavers.jpg", "floor");
	bReturn = CreateGLTexture("../../Utilities/textures/circular-brushed-gold-texture.jpg", "cylinder");
	bReturn = CreateGLTexture("../../Utilities/textures/rusticwood.jpg", "plank");
	bReturn = CreateGLTexture("../../Utilities/textures/tilesf2.jpg", "box");
	bReturn = CreateGLTexture("../../Utilities/textures/stainedglass.jpg", "ball");
	bReturn = CreateGLTexture("../../Utilities/textures/abstract.jpg", "cone");

	BindGLTextures();
}

/***********************************************************
 *  PrepareScene()
 ***********************************************************/
void SceneManager::PrepareScene()
{
	LoadSceneTextures();

	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadSphereMesh();
}

/***********************************************************
 *  RenderScene()
 ***********************************************************/
void SceneManager::RenderScene()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// FLOOR
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("floor");
	m_basicMeshes->DrawPlaneMesh();

	// CYLINDER
	scaleXYZ = glm::vec3(0.9f, 2.8f, 0.9f);
	positionXYZ = glm::vec3(0.0f, 0.9f, 0.4f);
	SetTransformations(scaleXYZ, 90.0f, 0.0f, -15.0f, positionXYZ);
	SetShaderTexture("cylinder");
	m_basicMeshes->DrawCylinderMesh();

	// LONG BOX
	scaleXYZ = glm::vec3(1.0f, 9.0f, 1.3f);
	positionXYZ = glm::vec3(0.2f, 2.27f, 2.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 95.0f, positionXYZ);
	SetShaderTexture("plank");
	m_basicMeshes->DrawBoxMesh();

	// SQUARE BOX
	scaleXYZ = glm::vec3(1.7f, 1.5f, 1.5f);
	positionXYZ = glm::vec3(3.3f, 3.85f, 2.19f);
	SetTransformations(scaleXYZ, 0.0f, 40.0f, 8.0f, positionXYZ);
	SetShaderTexture("box");
	m_basicMeshes->DrawBoxMesh();

	// SPHERE
	scaleXYZ = glm::vec3(1.0f);
	positionXYZ = glm::vec3(3.2f, 5.6f, 2.5f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("ball");
	m_basicMeshes->DrawSphereMesh();

	// CONE
	scaleXYZ = glm::vec3(1.2f, 4.0f, 1.2f);
	positionXYZ = glm::vec3(-3.3f, 2.5f, 2.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 5.0f, positionXYZ);
	SetShaderTexture("cone");
	m_basicMeshes->DrawConeMesh();
}
