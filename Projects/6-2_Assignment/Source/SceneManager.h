///////////////////////////////////////////////////////////////////////////////
// shadermanager.h
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ShaderManager.h"
#include "ShapeMeshes.h"

#include <string>
#include <vector>
#include <map>
#include <glm/glm.hpp>

/***********************************************************
 *  SceneManager
 ***********************************************************/
class SceneManager
{
public:
    SceneManager(ShaderManager *pShaderManager);
    ~SceneManager();

    // A set of GPU texture handles for one PBR material
    struct PBR_TEXTURE_SET
    {
        GLuint albedoID    = 0;
        GLuint normalID    = 0;
        GLuint metallicID  = 0;
        GLuint roughnessID = 0;
        GLuint aoID        = 0;
        GLuint heightID    = 0;
    };

    void PrepareScene();
    void RenderScene();

private:
    ShaderManager* m_pShaderManager;
    ShapeMeshes*   m_basicMeshes;

    // Maps a tag name (e.g. "ground") to its loaded GPU textures
    std::map<std::string, PBR_TEXTURE_SET> m_pbrTextures;

    // --- Texture helpers ---
    GLuint LoadSingleTexture(const char* filepath);
    GLuint CreateDefaultTexture(unsigned char value);
    bool   LoadPBRTextureSet(
               const std::string& tag,
               const char* albedoPath,
               const char* normalPath,
               const char* metallicPath,
               const char* roughnessPath,
               const char* aoPath,
               const char* heightPath = nullptr);

    // --- Shader helpers ---
    void SetTransformations(
             glm::vec3 scaleXYZ,
             float XrotationDegrees,
             float YrotationDegrees,
             float ZrotationDegrees,
             glm::vec3 positionXYZ);

    void SetShaderColor(float r, float g, float b, float a);
    void BindPBRMaterial(const std::string& tag);

    // --- Scene setup ---
    void LoadSceneTextures();
    void SetupSceneLights();
};
