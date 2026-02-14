///////////////////////////////////////////////////////////////////////////////
// SceneManager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"
#include <GL/gl.h>
#include <iostream>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>
#include <GLFW/glfw3.h>

namespace
{
    const char* g_ModelName        = "model";
    const char* g_ColorValueName   = "objectColor";
    const char* g_TextureValueName = "objectTexture";
    const char* g_UseTextureName   = "bUseTexture";
    const char* g_UseLightingName  = "bUseLighting";
    const char* g_UsePBRName       = "bUsePBR";
    const char* g_UseCheckerName   = "bUseCheckerboard";
}

// =====================================================================
//  Constructor / Destructor
// =====================================================================

SceneManager::SceneManager(ShaderManager *pShaderManager)
{
    m_pShaderManager = pShaderManager;
    m_basicMeshes = new ShapeMeshes();

    m_cameraPos   = glm::vec3(0.0f, 5.0f, 20.0f);
    m_cameraFront = glm::vec3(0.0f, -0.2f, -1.0f);
    m_cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);
    m_yaw   = -90.0f;
    m_pitch = -10.0f;
    m_movementSpeed = 5.0f;
    m_orthographic  = false;

    for (int i = 0; i < 16; i++)
    {
        m_textureIDs[i].tag = "/0";
        m_textureIDs[i].ID  = 0;
    }
    m_loadedTextures = 0;
}

SceneManager::~SceneManager()
{
    DestroyGLTextures();

    for (auto& pair : m_pbrTextures)
    {
        PBR_TEXTURE_SET& s = pair.second;
        if (s.albedoID)    glDeleteTextures(1, &s.albedoID);
        if (s.normalID)    glDeleteTextures(1, &s.normalID);
        if (s.metallicID)  glDeleteTextures(1, &s.metallicID);
        if (s.roughnessID) glDeleteTextures(1, &s.roughnessID);
        if (s.aoID)        glDeleteTextures(1, &s.aoID);
        if (s.heightID)    glDeleteTextures(1, &s.heightID);
    }
    m_pbrTextures.clear();

    delete m_basicMeshes;
    m_basicMeshes    = nullptr;
    m_pShaderManager = nullptr;
}

// =====================================================================
//  Single-Image Texture Management
// =====================================================================

bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
    int width = 0, height = 0, colorChannels = 0;
    GLuint textureID = 0;

    stbi_set_flip_vertically_on_load(true);
    unsigned char* image = stbi_load(filename, &width, &height, &colorChannels, 0);
    if (!image)
    {
        std::cout << "Could not load image: " << filename << std::endl;
        return false;
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (colorChannels == 3)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    else if (colorChannels == 4)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    else
    {
        stbi_image_free(image);
        return false;
    }

    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(image);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (m_loadedTextures < 16)
    {
        m_textureIDs[m_loadedTextures].ID  = textureID;
        m_textureIDs[m_loadedTextures].tag = tag;
        m_loadedTextures++;
        return true;
    }

    glDeleteTextures(1, &textureID);
    return false;
}

void SceneManager::BindGLTextures()
{
    for (int i = 0; i < m_loadedTextures; i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
    }
}

void SceneManager::DestroyGLTextures()
{
    for (int i = 0; i < m_loadedTextures; i++)
    {
        glDeleteTextures(1, &m_textureIDs[i].ID);
        m_textureIDs[i].ID = 0;
    }
    m_loadedTextures = 0;
}

int SceneManager::FindTextureSlot(std::string tag)
{
    for (int i = 0; i < m_loadedTextures; i++)
        if (m_textureIDs[i].tag == tag) return i;
    return -1;
}

// =====================================================================
//  PBR Texture Management
// =====================================================================

GLuint SceneManager::LoadSingleTexture(const char* filepath)
{
    int width = 0, height = 0, channels = 0;
    GLuint texID = 0;

    stbi_set_flip_vertically_on_load(true);
    unsigned char* image = stbi_load(filepath, &width, &height, &channels, 0);
    if (!image)
    {
        std::cout << "PBR: Could not load: " << filepath << std::endl;
        return 0;
    }

    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format         = GL_RGB;
    GLenum internalFormat = GL_RGB8;
    if (channels == 4) { format = GL_RGBA; internalFormat = GL_RGBA8; }
    if (channels == 1) { format = GL_RED;  internalFormat = GL_R8;   }

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(image);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "PBR: Loaded " << filepath << " (" << width << "x" << height
              << ", " << channels << "ch)" << std::endl;
    return texID;
}

GLuint SceneManager::CreateDefaultTexture(unsigned char value)
{
    GLuint texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned char pixel = value;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, &pixel);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

bool SceneManager::LoadPBRTextureSet(
    const std::string& tag,
    const char* albedoPath,
    const char* normalPath,
    const char* metallicPath,
    const char* roughnessPath,
    const char* aoPath,
    const char* heightPath)
{
    PBR_TEXTURE_SET set;
    set.albedoID    = LoadSingleTexture(albedoPath);
    set.normalID    = LoadSingleTexture(normalPath);
    set.metallicID  = metallicPath  ? LoadSingleTexture(metallicPath)  : CreateDefaultTexture(0);
    set.roughnessID = LoadSingleTexture(roughnessPath);
    set.aoID        = aoPath        ? LoadSingleTexture(aoPath)        : CreateDefaultTexture(255);

    if (heightPath)
    {
        set.heightID  = LoadSingleTexture(heightPath);
        set.hasHeight = (set.heightID != 0);
    }
    else
    {
        set.heightID  = CreateDefaultTexture(0);
        set.hasHeight = false;
    }

    if (set.albedoID == 0)
    {
        std::cout << "PBR: Failed to load albedo for '" << tag << "'" << std::endl;
        return false;
    }

    m_pbrTextures[tag] = set;
    std::cout << "PBR: Registered texture set '" << tag << "'"
              << (set.hasHeight ? " (with parallax)" : "") << std::endl;
    return true;
}

// =====================================================================
//  Shader State Helpers
//  Each helper disables all other modes to prevent state leaking.
// =====================================================================

void SceneManager::SetShaderTexture(std::string textureTag)
{
    m_pShaderManager->setBoolValue(g_UsePBRName, false);
    m_pShaderManager->setBoolValue(g_UseCheckerName, false);
    m_pShaderManager->setBoolValue(g_UseTextureName, true);
    m_pShaderManager->setBoolValue("bUseParallax", false);
    m_pShaderManager->setVec2Value("UVscale", glm::vec2(1.0f, 1.0f));

    int slot = FindTextureSlot(textureTag);
    if (slot >= 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_textureIDs[slot].ID);
        m_pShaderManager->setSampler2DValue(g_TextureValueName, 0);
    }
}

void SceneManager::SetShaderColor(float red, float green, float blue, float alpha)
{
    m_pShaderManager->setBoolValue(g_UsePBRName, false);
    m_pShaderManager->setBoolValue(g_UseCheckerName, false);
    m_pShaderManager->setBoolValue(g_UseTextureName, false);
    m_pShaderManager->setBoolValue("bUseParallax", false);
    m_pShaderManager->setVec2Value("UVscale", glm::vec2(1.0f, 1.0f));
    m_pShaderManager->setVec4Value(g_ColorValueName, glm::vec4(red, green, blue, alpha));
}

void SceneManager::SetShaderPBR(const std::string& tag)
{
    auto it = m_pbrTextures.find(tag);
    if (it == m_pbrTextures.end())
    {
        std::cout << "PBR: '" << tag << "' not found, falling back" << std::endl;
        SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
        return;
    }

    const PBR_TEXTURE_SET& set = it->second;

    m_pShaderManager->setBoolValue(g_UseTextureName, false);
    m_pShaderManager->setBoolValue(g_UseCheckerName, false);
    m_pShaderManager->setBoolValue(g_UsePBRName, true);
    m_pShaderManager->setVec2Value("UVscale", glm::vec2(1.0f, 1.0f));
    m_pShaderManager->setVec3Value("pbrTint", glm::vec3(1.0f));  // no tint by default

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, set.albedoID);
    m_pShaderManager->setIntValue("albedoMap", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, set.normalID);
    m_pShaderManager->setIntValue("normalMap", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, set.metallicID);
    m_pShaderManager->setIntValue("metallicMap", 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, set.roughnessID);
    m_pShaderManager->setIntValue("roughnessMap", 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, set.aoID);
    m_pShaderManager->setIntValue("aoMap", 4);

    // Height map for parallax
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, set.heightID);
    m_pShaderManager->setIntValue("heightMap", 5);

    m_pShaderManager->setBoolValue("bUseParallax", set.hasHeight);
    m_pShaderManager->setFloatValue("parallaxScale", 0.06f);
}

void SceneManager::SetShaderCheckerboard(
    float tileCountU, float tileCountV,
    glm::vec3 color1, glm::vec3 color2)
{
    m_pShaderManager->setBoolValue(g_UsePBRName, false);
    m_pShaderManager->setBoolValue(g_UseTextureName, false);
    m_pShaderManager->setBoolValue(g_UseCheckerName, true);
    m_pShaderManager->setBoolValue("bUseParallax", false);
    m_pShaderManager->setVec2Value("UVscale", glm::vec2(tileCountU, tileCountV));
    m_pShaderManager->setVec3Value("checkerColor1", color1);
    m_pShaderManager->setVec3Value("checkerColor2", color2);
}

void SceneManager::SetUVScale(float u, float v)
{
    m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
}

// =====================================================================
//  Lighting
// =====================================================================

void SceneManager::SetupLighting()
{
    // --- Multi-light setup ---
    // Light 0: Warm overhead (main key light)
    // Light 1: Cool fill from the aisle side
    // Light 2: Warm back-wall bounce
    // Light 3: Subtle front fill for the camera side

    const int numLights = 4;
    m_pShaderManager->setIntValue("numLights", numLights);

    glm::vec3 positions[] = {
        glm::vec3(4.5f,  7.0f,  0.0f),   // overhead center
        glm::vec3(-2.0f, 5.0f,  0.0f),   // aisle-side fill
        glm::vec3(7.0f,  4.0f,  0.0f),   // wall bounce
        glm::vec3(4.0f,  3.0f, 15.0f),   // front fill
    };
    glm::vec3 colors[] = {
        glm::vec3(1.0f, 0.95f, 0.85f),   // warm white
        glm::vec3(0.7f, 0.8f,  1.0f),    // cool blue fill
        glm::vec3(1.0f, 0.9f,  0.8f),    // warm bounce
        glm::vec3(0.9f, 0.9f,  0.95f),   // neutral fill
    };
    float intensities[] = {
         60.0f,   // key
         25.0f,   // fill
         18.0f,   // bounce
         12.0f,   // front
    };

    for (int i = 0; i < numLights; ++i)
    {
        std::string idx = std::to_string(i);
        m_pShaderManager->setVec3Value("lightPositions[" + idx + "]", positions[i]);
        m_pShaderManager->setVec3Value("lightColors[" + idx + "]",    colors[i]);
        m_pShaderManager->setFloatValue("lightIntensities[" + idx + "]", intensities[i]);
    }

    // Legacy single-light uniforms (kept for any non-PBR paths)
    m_pShaderManager->setVec3Value("lightPos",   positions[0]);
    m_pShaderManager->setVec3Value("lightColor",  colors[0]);
    m_pShaderManager->setVec3Value("viewPos",    m_cameraPos);

    // Fake environment hemisphere colors (diner ambiance)
    m_pShaderManager->setVec3Value("envColorTop",    glm::vec3(0.85f, 0.90f, 1.00f));
    m_pShaderManager->setVec3Value("envColorBottom", glm::vec3(0.15f, 0.12f, 0.10f));
    m_pShaderManager->setFloatValue("envIntensity",  0.25f);
}

// =====================================================================
//  Transformations
// =====================================================================

void SceneManager::SetTransformations(
    glm::vec3 scaleXYZ,
    float XrotationDegrees,
    float YrotationDegrees,
    float ZrotationDegrees,
    glm::vec3 positionXYZ)
{
    glm::mat4 scale       = glm::scale(scaleXYZ);
    glm::mat4 rotationX   = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1, 0, 0));
    glm::mat4 rotationY   = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0, 1, 0));
    glm::mat4 rotationZ   = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0, 0, 1));
    glm::mat4 translation = glm::translate(positionXYZ);

    glm::mat4 modelView = translation * rotationY * rotationX * rotationZ * scale;
    m_pShaderManager->setMat4Value(g_ModelName, modelView);
}

// =====================================================================
//  Texture Loading
// =====================================================================

void SceneManager::LoadSceneTextures()
{
    // Simple textures (kept for any objects that don't use PBR)
    CreateGLTexture("../../Utilities/textures/tilesf2.jpg", "floor");
    CreateGLTexture("../../Utilities/textures/stainless.jpg", "stainless");
    BindGLTextures();

    // PBR texture sets

    // Leather — diner booth seats (has AO, has displacement for parallax)
    LoadPBRTextureSet("pbr_leather",
        "../../Utilities/textures/Leather036D_2K-PNG/Leather036D_2K-PNG_Color.png",
        "../../Utilities/textures/Leather036D_2K-PNG/Leather036D_2K-PNG_NormalGL.png",
        nullptr,
        "../../Utilities/textures/Leather036D_2K-PNG/Leather036D_2K-PNG_Roughness.png",
        "../../Utilities/textures/Leather036D_2K-PNG/Leather036D_2K-PNG_AmbientOcclusion.png",
        "../../Utilities/textures/Leather036D_2K-PNG/Leather036D_2K-PNG_Displacement.png");

    // Metal — chrome table legs, lamp shades (has metalness, no AO)
    //LoadPBRTextureSet("pbr_metal",
    //    "../../Utilities/textures/Metal049A_2K-PNG/Metal049A_2K-PNG_Color.png",
    //    "../../Utilities/textures/Metal049A_2K-PNG/Metal049A_2K-PNG_NormalGL.png",
    //    "../../Utilities/textures/Metal049A_2K-PNG/Metal049A_2K-PNG_Metalness.png",
    //    "../../Utilities/textures/Metal049A_2K-PNG/Metal049A_2K-PNG_Roughness.png",
    //    nullptr,
    //    "../../Utilities/textures/Metal049A_2K-PNG/Metal049A_2K-PNG_Displacement.png");

    // Rubber — center aisle floor (has displacement for parallax)
    LoadPBRTextureSet("pbr_rubber",
        "../../Utilities/textures/Rubber004_2K-PNG/Rubber004_2K-PNG_Color.png",
        "../../Utilities/textures/Rubber004_2K-PNG/Rubber004_2K-PNG_NormalGL.png",
        nullptr,
        "../../Utilities/textures/Rubber004_2K-PNG/Rubber004_2K-PNG_Roughness.png",
        nullptr,
        "../../Utilities/textures/Rubber004_2K-PNG/Rubber004_2K-PNG_Displacement.png");

    // Plaster — diner wall (has displacement for parallax)
    LoadPBRTextureSet("pbr_plaster",
        "../../Utilities/textures/Plaster001_2K-PNG/Plaster001_2K-PNG_Color.png",
        "../../Utilities/textures/Plaster001_2K-PNG/Plaster001_2K-PNG_NormalGL.png",
        nullptr,
        "../../Utilities/textures/Plaster001_2K-PNG/Plaster001_2K-PNG_Roughness.png",
        nullptr,
        "../../Utilities/textures/Plaster001_2K-PNG/Plaster001_2K-PNG_Displacement.png");
}

// =====================================================================
//  Scene Prepare
// =====================================================================

void SceneManager::PrepareScene(GLFWwindow* window)
{
    LoadSceneTextures();

    // Load all mesh primitives we'll use
    m_basicMeshes->LoadPlaneMesh();
    m_basicMeshes->LoadBoxMesh();
    m_basicMeshes->LoadCylinderMesh();
    m_basicMeshes->LoadSphereMesh();
    m_basicMeshes->LoadConeMesh();
    m_basicMeshes->LoadTorusMesh();

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    SetProjection(width, height);
    SetViewMatrix();
}

// =====================================================================
//  RenderScene — 1950s American Diner
//
//  Layout (top-down, camera at +Z looking toward -Z):
//
//       Wall at X=7.5 (runs along Z axis)
//       Booths:  X ~ 2.5 to 7.3
//       Aisle:   X ~ 0 to 2.5 (rubber strip)
//       Floor:   X ~ -10 to 10
//       5 booths at Z = -8, -4, 0, 4, 8
//
// =====================================================================

void SceneManager::RenderScene()
{
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    SetupLighting();

    // =================================================================
    // FLOOR — black & white checkerboard
    // =================================================================
    // The plane mesh sits at Y=0 in XZ. UVscale controls tile count.
    SetTransformations(
        glm::vec3(20.0f, 1.0f, 30.0f),  // scale
        0.0f, 0.0f, 0.0f,                // rotations
        glm::vec3(0.0f, 0.0f, 0.0f));    // position
    SetShaderCheckerboard(
        20.0f, 30.0f,                     // tile counts (1-unit tiles)
        glm::vec3(0.92f, 0.92f, 0.92f),  // white
        glm::vec3(0.06f, 0.06f, 0.06f)); // black
    m_basicMeshes->DrawPlaneMesh();

    // =================================================================
    // RUBBER AISLE STRIP — center walkway (PBR rubber on top of floor)
    // =================================================================
    SetTransformations(
        glm::vec3(3.0f, 0.02f, 30.0f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(1.0f, 0.01f, 0.0f));
    SetShaderPBR("pbr_rubber");
    SetUVScale(2.0f, 10.0f);
    m_basicMeshes->DrawBoxMesh();

    // =================================================================
    // BACK WALL — light blue (upper) with checkerboard border strip
    // =================================================================

    // Main wall — plaster PBR tinted light blue
    SetTransformations(
        glm::vec3(0.3f, 8.0f, 30.0f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(7.65f, 4.0f, 0.0f));
    SetShaderPBR("pbr_plaster");
    SetUVScale(1.0f, 4.0f);
    m_pShaderManager->setVec3Value("pbrTint", glm::vec3(0.62f, 0.78f, 0.88f)); // light blue
    m_basicMeshes->DrawBoxMesh();

    // Checkerboard border strip on wall (at about chair-back height)
    // Using a thin box so each face gets proportional UVs.
    // The -X face (facing the aisle) is 1.0 unit tall × 30 units wide.
    // For ~0.5-unit square tiles: UVscale = (60, 2)  (U→Z, V→Y on -X face)
    SetTransformations(
        glm::vec3(0.02f, 0.5f, 30.0f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(7.48f, 2.9f, 0.0f));
    SetShaderCheckerboard(
        60.0f, 2.0f,
        glm::vec3(0.95f), glm::vec3(0.05f));
    m_basicMeshes->DrawBoxMesh();

    // Checkerboard wall frame
    // Top metal strip
    SetTransformations(
        glm::vec3(0.02f, 0.06f, 30.0f),  // thin along Y, long along Z
        0.0f, 0.0f, 0.0f,
        glm::vec3(7.48f, 3.18f, 0.0f));  // slightly above checkerboard
    SetShaderTexture("stainless");
    SetUVScale(1.0f, 10.0f);            // stretch along Z
    m_basicMeshes->DrawBoxMesh();
    
    // Bottom metal strip
    SetTransformations(
        glm::vec3(0.02f, 0.06f, 30.0f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(7.48f, 2.62f, 0.0f));  // slightly below checkerboard
    SetShaderTexture("stainless");
    SetUVScale(1.0f, 10.0f);
    m_basicMeshes->DrawBoxMesh();

    // =================================================================
    // CEILING
    // =================================================================
    SetTransformations(
        glm::vec3(20.0f, 0.1f, 30.0f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(0.0f, 8.0f, 0.0f));
    SetShaderColor(0.12f, 0.10f, 0.10f, 1.0f);
    m_basicMeshes->DrawBoxMesh();

    // =================================================================
    // BOOTHS — 5 booth units along the wall
    // =================================================================
    const int   boothCount   = 5;
    const float boothSpacing = 4.0f;   // center-to-center Z distance
    const float boothWidth   = 3.4f;   // Z extent of each booth

    // X positions (distance from wall)
    const float wallBackX     = 7.1f;  // wall-side bench back
    const float wallSeatX     = 6.1f;  // wall-side seat cushion center
    const float tableX        = 4.75f; // table center
    const float aisleSeatX    = 3.4f;  // aisle-side seat cushion center
    const float aisleBackX    = 2.5f;  // aisle-side bench back

    // Y dimensions
    const float seatHeight    = 0.50f;
    const float seatY         = seatHeight / 2.0f;
    const float backHeight    = 2.8f;
    const float backY         = backHeight / 2.0f;
    const float backThickness = 0.20f;
    const float seatDepth     = 1.6f;  // X-direction depth of seat cushion
    const float tableTopY     = 1.55f;

    for (int i = 0; i < boothCount; ++i)
    {
        float zPos = -((boothCount - 1) * boothSpacing) / 2.0f + i * boothSpacing;

        // --- Wall-side bench back (tall) ---
        SetTransformations(
            glm::vec3(backThickness, backHeight, boothWidth),
            0.0f, 0.0f, 0.0f,
            glm::vec3(wallBackX, backY, zPos));
        SetShaderPBR("pbr_leather");
        SetUVScale(1.0f, 2.0f);
        m_basicMeshes->DrawBoxMesh();

        // --- Wall-side seat cushion ---
        SetTransformations(
            glm::vec3(seatDepth, seatHeight, boothWidth),
            0.0f, 0.0f, 0.0f,
            glm::vec3(wallSeatX, seatY, zPos));
        SetShaderPBR("pbr_leather");
        SetUVScale(1.0f, 2.0f);
        m_basicMeshes->DrawBoxMesh();

        // --- Aisle-side seat cushion ---
        SetTransformations(
            glm::vec3(seatDepth, seatHeight, boothWidth),
            0.0f, 0.0f, 0.0f,
            glm::vec3(aisleSeatX, seatY, zPos));
        SetShaderPBR("pbr_leather");
        SetUVScale(1.0f, 2.0f);
        m_basicMeshes->DrawBoxMesh();

        // --- Aisle-side bench back (tall, faces the aisle) ---
        SetTransformations(
            glm::vec3(backThickness, backHeight, boothWidth),
            0.0f, 0.0f, 0.0f,
            glm::vec3(aisleBackX, backY, zPos));
        SetShaderPBR("pbr_leather");
        SetUVScale(1.0f, 2.0f);
        m_basicMeshes->DrawBoxMesh();

        // --- Table top (chrome) ---
        SetTransformations(
            glm::vec3(2.0f, 0.06f, 2.6f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX, tableTopY, zPos));
        SetShaderTexture("stainless");
        SetUVScale(2.0f, 2.0f);
        m_basicMeshes->DrawBoxMesh();

        // --- Chrome edge strip around table top ---
        // Front edge
        SetTransformations(
            glm::vec3(2.08f, 0.08f, 0.04f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX, tableTopY, zPos + 1.32f));
        SetShaderTexture("stainless");
        SetUVScale(4.0f, 1.0f);
        m_basicMeshes->DrawBoxMesh();
        // Back edge
        SetTransformations(
            glm::vec3(2.08f, 0.08f, 0.04f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX, tableTopY, zPos - 1.32f));
        SetShaderTexture("stainless");
        SetUVScale(4.0f, 1.0f);
        m_basicMeshes->DrawBoxMesh();

        // --- Table pedestal (chrome cylinder) ---
        // Cylinder base at Y=0, extends up to Y=scale.y
        SetTransformations(
            glm::vec3(0.12f, tableTopY - 0.03f, 0.12f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX, 0.0f, zPos));
        SetShaderTexture("stainless");
        SetUVScale(1.0f, 2.0f);
        m_basicMeshes->DrawCylinderMesh();

        // --- Table base plate (flat chrome disc) ---
        SetTransformations(
            glm::vec3(0.5f, 0.04f, 0.5f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX, 0.0f, zPos));
        SetShaderTexture("stainless");
        SetUVScale(1.0f, 1.0f);
        m_basicMeshes->DrawCylinderMesh();

        // =============================================================
        // PENDANT LAMP (correct physical alignment)
        // =============================================================
        
        float ceilingY = 8.0f;
        
        // ---- SHADE ----
        // Half-sphere extends downward from its center.
        float shadeHalfHeight = 0.4f;
        float shadeTopY       = 6.5f;                 // move lamp up
        float shadeCenterY    = shadeTopY - shadeHalfHeight;
        
        SetTransformations(
            glm::vec3(1.3f, shadeHalfHeight, 1.3f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX, shadeCenterY, zPos));
        SetShaderTexture("stainless");
        SetUVScale(2.0f, 2.0f);
        m_basicMeshes->DrawHalfSphereMesh();
        
        // ---- CAP ----
        // Cap sits flush on top of the shade dome
        float capHeight  = 0.12f;
        float capCenterY = shadeTopY;  // flush with shade top
        
        SetTransformations(
            glm::vec3(0.18f, capHeight, 0.18f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX, capCenterY, zPos));
        SetShaderTexture("stainless");
        SetUVScale(1.0f, 1.0f);
        m_basicMeshes->DrawHalfSphereMesh();
        
        // ---- CABLE ----
        float capTopY    = capCenterY + capHeight;
        float cableLen   = ceilingY - capTopY;
        
        SetTransformations(
            glm::vec3(0.03f, cableLen, 0.03f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX, capTopY, zPos));
        SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f);
        m_basicMeshes->DrawCylinderMesh();
        
        // ---- BULB ----
        SetTransformations(
            glm::vec3(0.20f, 0.20f, 0.20f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX, shadeCenterY - 0.15f, zPos)); // move upward inside shade
        SetShaderColor(1.0f, 0.95f, 0.70f, 1.0f);
        m_basicMeshes->DrawSphereMesh();
    }

    // =================================================================
    // CONDIMENT / NAPKIN HOLDER on each table (small chrome box)
    // =================================================================
    for (int i = 0; i < boothCount; ++i)
    {
        float zPos = -((boothCount - 1) * boothSpacing) / 2.0f + i * boothSpacing;

        // Napkin holder
        SetTransformations(
            glm::vec3(0.25f, 0.3f, 0.15f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX + 0.5f, tableTopY + 0.18f, zPos));
        SetShaderTexture("stainless");
        SetUVScale(1.0f, 1.0f);
        m_basicMeshes->DrawBoxMesh();

        // Condiment bottles (small cylinders)
        SetTransformations(
            glm::vec3(0.06f, 0.25f, 0.06f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX + 0.3f, tableTopY + 0.03f, zPos + 0.3f));
        SetShaderColor(0.8f, 0.1f, 0.1f, 1.0f); // ketchup red
        m_basicMeshes->DrawCylinderMesh();

        SetTransformations(
            glm::vec3(0.06f, 0.22f, 0.06f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX + 0.3f, tableTopY + 0.03f, zPos + 0.15f));
        SetShaderColor(0.6f, 0.5f, 0.05f, 1.0f); // mustard yellow
        m_basicMeshes->DrawCylinderMesh();
    }

    // =================================================================
    // WALL DECORATIONS — hubcap circles on the wall
    // =================================================================
    // A few torus shapes mounted on the wall to simulate hubcaps
    float hubcapX = 7.44f; // just in front of the wall

    for (int h = 0; h < 4; ++h)
    {
        float hz = -4.0f + h * 2.8f;
        float hy = 4.0f + (h % 2) * 0.6f; // staggered heights

        SetTransformations(
            glm::vec3(0.4f, 0.4f, 0.12f),
            0.0f, 90.0f, 0.0f,           // flat on the wall via Y axis
            glm::vec3(hubcapX, hy, hz));
        SetShaderTexture("stainless");
        SetUVScale(3.0f, 3.0f);
        m_basicMeshes->DrawTorusMesh();
    }

    // Neon light strips along the ceiling edge
    
    // old
    //SetTransformations(
    //    glm::vec3(0.08f, 0.08f, 30.0f),
    //    0.0f, 0.0f, 0.0f,
    //    glm::vec3(7.45f, 7.9f, 0.0f));
    //SetShaderColor(1.0f, 0.15f, 0.10f, 1.0f); // neon red
    //m_basicMeshes->DrawBoxMesh();
    
    //new
    for (int n = -2; n <= 2; ++n) {
    float zOffset = n * 6.0f;

    SetTransformations(
        glm::vec3(0.08f, 0.08f, 5.0f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(7.45f, 7.9f, zOffset));
    SetShaderColor(1.0f, 0.15f, 0.10f, 1.0f);
    m_basicMeshes->DrawBoxMesh();
  }
}

// =====================================================================
//  Camera / Projection
// =====================================================================

void SceneManager::SetProjection(int width, int height)
{
    if (!m_pShaderManager) return;
    if (height == 0) height = 1;
    float aspect = static_cast<float>(width) / static_cast<float>(height);

    glm::mat4 projection;
    if (m_orthographic)
    {
        float size = 20.0f;
        projection = glm::ortho(-size * aspect, size * aspect, -size, size, 0.1f, 100.0f);
    }
    else
    {
        projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    }

    m_pShaderManager->setMat4Value("projection", projection);
}

void SceneManager::SetViewMatrix()
{
    if (!m_pShaderManager) return;
    glm::mat4 view = glm::lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_cameraUp);
    m_pShaderManager->setMat4Value("view", view);
}

void SceneManager::UpdateProjection(GLFWwindow* window)
{
    if (!window) return;
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    SetProjection(width, height);
}
