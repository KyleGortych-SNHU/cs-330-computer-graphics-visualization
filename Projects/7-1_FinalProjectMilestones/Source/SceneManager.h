///////////////////////////////////////////////////////////////////////////////
// SceneManager.h
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//  Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../../../Utilities/ShaderManager.h"
#include "ShapeMeshes.h"

#include <string>
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

/***********************************************************
 *  SceneManager
 ***********************************************************/
class SceneManager
{
public:
    SceneManager(ShaderManager *pShaderManager);
    ~SceneManager();

    struct TEXTURE_INFO
    {
        std::string tag;
        uint32_t ID;
    };

    struct PBR_TEXTURE_SET
    {
        GLuint albedoID    = 0;
        GLuint normalID    = 0;
        GLuint metallicID  = 0;
        GLuint roughnessID = 0;
        GLuint aoID        = 0;
        GLuint heightID    = 0;
        bool   hasHeight   = false;
    };

    struct OBJECT_MATERIAL
    {
        float ambientStrength;
        glm::vec3 ambientColor;
        glm::vec3 diffuseColor;
        glm::vec3 specularColor;
        float shininess;
        std::string tag;
    };

private:
    ShaderManager* m_pShaderManager;
    ShapeMeshes *m_basicMeshes;

    // Single-image textures
    int m_loadedTextures;
    TEXTURE_INFO m_textureIDs[16];

    // PBR texture sets
    std::map<std::string, PBR_TEXTURE_SET> m_pbrTextures;

    // Materials
    std::vector<OBJECT_MATERIAL> m_objectMaterials;

    // --- Shader helpers ---
    void SetTransformations(glm::vec3 scaleXYZ,
                            float XrotationDegrees,
                            float YrotationDegrees,
                            float ZrotationDegrees,
                            glm::vec3 positionXYZ);

    void SetShaderColor(float r, float g, float b, float a);

    void SetShaderCheckerboard(
        float tileCountU, float tileCountV,
        glm::vec3 color1 = glm::vec3(1.0f),
        glm::vec3 color2 = glm::vec3(0.0f));

    void SetUVScale(float u, float v);

    // --- Single-Image Texture Management ---
    bool CreateGLTexture(const char* filename, std::string tag);
    void BindGLTextures();
    void DestroyGLTextures();
    void SetShaderTexture(std::string textureTag);
    int FindTextureSlot(std::string tag);
    void LoadSceneTextures();

    // --- PBR Texture Management ---
    GLuint LoadSingleTexture(const char* filepath);
    GLuint CreateDefaultTexture(unsigned char value);
    bool LoadPBRTextureSet(
        const std::string& tag,
        const char* albedoPath,
        const char* normalPath,
        const char* metallicPath,
        const char* roughnessPath,
        const char* aoPath,
        const char* heightPath = nullptr);
    void SetShaderPBR(const std::string& tag);
    void SetupLighting();

    // Camera state
    glm::vec3 m_cameraPos   = glm::vec3(0.0f, 10.0f, 30.0f);
    glm::vec3 m_cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 m_cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);
    float m_yaw              = -90.0f;
    float m_pitch            = 0.0f;
    float m_movementSpeed    = 5.0f;
    bool m_orthographic      = false;

    void UpdateCameraDirection();

public:
    void PrepareScene(GLFWwindow* window);
    void RenderScene();
    void SetProjection(int width, int height);
    void UpdateProjection(GLFWwindow* window);
    void SetViewMatrix();

    void MoveCamera(const glm::vec3& delta);
    void RotateCamera(float xoffset, float yoffset);
    void AdjustSpeed(float yoffset);
    void ToggleProjection(bool orthographic);
};
