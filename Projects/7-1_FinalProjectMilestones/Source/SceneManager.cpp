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
    m_pShaderManager->setBoolValue("bIsEmissive", false);
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
    m_pShaderManager->setBoolValue("bIsEmissive", false);
    m_pShaderManager->setVec2Value("UVscale", glm::vec2(1.0f, 1.0f));
    m_pShaderManager->setVec4Value(g_ColorValueName, glm::vec4(red, green, blue, alpha));
}

/***********************************************************
 *  SetShaderEmissive()
 *
 *  Sets the fragment shader to emissive mode — the object
 *  glows with the given color and ignores scene lighting.
 *  Used for lightbulbs and neon strips.
 ***********************************************************/
void SceneManager::SetShaderEmissive(float r, float g, float b, float strength, float alpha)
{
    m_pShaderManager->setBoolValue(g_UsePBRName, false);
    m_pShaderManager->setBoolValue(g_UseCheckerName, false);
    m_pShaderManager->setBoolValue(g_UseTextureName, false);
    m_pShaderManager->setBoolValue("bUseParallax", false);
    m_pShaderManager->setBoolValue("bIsEmissive", true);
    m_pShaderManager->setVec3Value("emissiveColor", glm::vec3(r, g, b));
    m_pShaderManager->setFloatValue("emissiveStrength", strength);
    m_pShaderManager->setFloatValue("emissiveAlpha", alpha);
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
    m_pShaderManager->setBoolValue("bIsEmissive", false);
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

/***********************************************************
 *  SetShaderPBRTinted()
 *
 *  Convenience: activates PBR textures then applies a color
 *  tint. Useful for reusing a single PBR set (e.g. plastic)
 *  with different object colors (ketchup red, mustard yellow).
 ***********************************************************/
void SceneManager::SetShaderPBRTinted(const std::string& tag, const glm::vec3& tint)
{
    SetShaderPBR(tag);
    m_pShaderManager->setVec3Value("pbrTint", tint);
}

void SceneManager::SetShaderCheckerboard(
    float tileCountU, float tileCountV,
    glm::vec3 color1, glm::vec3 color2)
{
    m_pShaderManager->setBoolValue(g_UsePBRName, false);
    m_pShaderManager->setBoolValue(g_UseTextureName, false);
    m_pShaderManager->setBoolValue(g_UseCheckerName, true);
    m_pShaderManager->setBoolValue("bUseParallax", false);
    m_pShaderManager->setBoolValue("bIsEmissive", false);
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
    // ---------------------------------------------------------------
    //  10 lights total (MAX_LIGHTS = 10):
    //    Lights 0–4 : pendant lamp bulbs (warm, per-booth)
    //    Light  5   : overhead fill (cool, dimmed)
    //    Light  6   : wall-bounce (warm, subtle)
    //    Light  7   : front camera fill (neutral, subtle)
    //    Lights 8–9 : neon ceiling strips (red wash)
    // ---------------------------------------------------------------

    const int   boothCount   = 5;
    const float boothSpacing = 4.0f;
    const float tableX       = 4.75f;

    // Bulb Y position (matches RenderScene bulb geometry)
    const float shadeCenterY = 6.5f - 0.4f;
    const float bulbY        = shadeCenterY - 0.15f;

    const int totalLights = 10;
    m_pShaderManager->setIntValue("numLights", totalLights);

    glm::vec3 positions[10];
    glm::vec3 colors[10];
    float     intensities[10];

    // --- Lights 0-4: one per pendant lamp (warm tungsten, reduced) ---
    for (int i = 0; i < boothCount; ++i)
    {
        float zPos = -((boothCount - 1) * boothSpacing) / 2.0f + i * boothSpacing;
        positions[i]   = glm::vec3(tableX, bulbY, zPos);
        colors[i]      = glm::vec3(1.0f, 0.90f, 0.68f);  // warm tungsten
        intensities[i] = 12.0f;                            // reduced from 18
    }

    // --- Light 5: overhead fill (cool, dimmer for moodiness) ---
    positions[5]   = glm::vec3(2.0f, 7.5f, 0.0f);
    colors[5]      = glm::vec3(0.6f, 0.7f, 0.9f);
    intensities[5] = 12.0f;   // was 30

    // --- Light 6: wall-bounce (warm reflected light, subtle) ---
    positions[6]   = glm::vec3(7.0f, 4.0f, 0.0f);
    colors[6]      = glm::vec3(1.0f, 0.85f, 0.7f);
    intensities[6] = 8.0f;    // was 15

    // --- Light 7: front fill for camera side (very subtle) ---
    positions[7]   = glm::vec3(0.0f, 3.0f, 15.0f);
    colors[7]      = glm::vec3(0.85f, 0.85f, 0.9f);
    intensities[7] = 5.0f;    // was 10

    // --- Lights 8-9: neon red ceiling strips (red color bleed) ---
    // Two red lights spread along the neon strip run at ceiling height
    positions[8]   = glm::vec3(7.4f, 7.8f, -5.0f);
    colors[8]      = glm::vec3(1.0f, 0.12f, 0.08f);      // neon red
    intensities[8] = 20.0f;

    positions[9]   = glm::vec3(7.4f, 7.8f, 5.0f);
    colors[9]      = glm::vec3(1.0f, 0.12f, 0.08f);      // neon red
    intensities[9] = 20.0f;

    for (int i = 0; i < totalLights; ++i)
    {
        std::string idx = std::to_string(i);
        m_pShaderManager->setVec3Value("lightPositions[" + idx + "]", positions[i]);
        m_pShaderManager->setVec3Value("lightColors[" + idx + "]",    colors[i]);
        m_pShaderManager->setFloatValue("lightIntensities[" + idx + "]", intensities[i]);
    }

    // Legacy single-light uniforms (kept for fallback)
    m_pShaderManager->setVec3Value("lightPos",   positions[0]);
    m_pShaderManager->setVec3Value("lightColor",  colors[0]);
    m_pShaderManager->setVec3Value("viewPos",    m_cameraPos);

    // Hemisphere environment (dimmer for moodier diner ambiance)
    m_pShaderManager->setVec3Value("envColorTop",    glm::vec3(0.55f, 0.55f, 0.65f));
    m_pShaderManager->setVec3Value("envColorBottom", glm::vec3(0.10f, 0.08f, 0.07f));
    m_pShaderManager->setFloatValue("envIntensity",  0.15f);   // was 0.25
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
    // Simple textures (kept for any objects that still use them)
    CreateGLTexture("../../Utilities/textures/tilesf2.jpg", "floor");
    CreateGLTexture("../../Utilities/textures/stainless.jpg", "stainless");
    BindGLTextures();

    // ---- PBR texture sets ----

    // Leather — diner booth seats
    LoadPBRTextureSet("pbr_leather",
        "../../Utilities/textures/Leather036D_2K-PNG/Leather036D_2K-PNG_Color.png",
        "../../Utilities/textures/Leather036D_2K-PNG/Leather036D_2K-PNG_NormalGL.png",
        nullptr,
        "../../Utilities/textures/Leather036D_2K-PNG/Leather036D_2K-PNG_Roughness.png",
        "../../Utilities/textures/Leather036D_2K-PNG/Leather036D_2K-PNG_AmbientOcclusion.png",
        "../../Utilities/textures/Leather036D_2K-PNG/Leather036D_2K-PNG_Displacement.png");

    // Metal009 — brushed chrome for tables & napkin holder
    LoadPBRTextureSet("pbr_metal009",
        "../../Utilities/textures/Metal009_2K-PNG/Metal009_2K-PNG_Color.png",
        "../../Utilities/textures/Metal009_2K-PNG/Metal009_2K-PNG_NormalGL.png",
        "../../Utilities/textures/Metal009_2K-PNG/Metal009_2K-PNG_Metalness.png",
        "../../Utilities/textures/Metal009_2K-PNG/Metal009_2K-PNG_Roughness.png",
        nullptr,
        "../../Utilities/textures/Metal009_2K-PNG/Metal009_2K-PNG_Displacement.png");

    // Metal052A — darker metal for lamp shades & cables
    LoadPBRTextureSet("pbr_metal052",
        "../../Utilities/textures/Metal052A_2K-PNG/Metal052A_2K-PNG_Color.png",
        "../../Utilities/textures/Metal052A_2K-PNG/Metal052A_2K-PNG_NormalGL.png",
        "../../Utilities/textures/Metal052A_2K-PNG/Metal052A_2K-PNG_Metalness.png",
        "../../Utilities/textures/Metal052A_2K-PNG/Metal052A_2K-PNG_Roughness.png",
        nullptr,
        "../../Utilities/textures/Metal052A_2K-PNG/Metal052A_2K-PNG_Displacement.png");

    // Rubber — center aisle floor
    LoadPBRTextureSet("pbr_rubber",
        "../../Utilities/textures/Rubber004_2K-PNG/Rubber004_2K-PNG_Color.png",
        "../../Utilities/textures/Rubber004_2K-PNG/Rubber004_2K-PNG_NormalGL.png",
        nullptr,
        "../../Utilities/textures/Rubber004_2K-PNG/Rubber004_2K-PNG_Roughness.png",
        nullptr,
        "../../Utilities/textures/Rubber004_2K-PNG/Rubber004_2K-PNG_Displacement.png");

    // Plaster — diner wall
    LoadPBRTextureSet("pbr_plaster",
        "../../Utilities/textures/Plaster001_2K-PNG/Plaster001_2K-PNG_Color.png",
        "../../Utilities/textures/Plaster001_2K-PNG/Plaster001_2K-PNG_NormalGL.png",
        nullptr,
        "../../Utilities/textures/Plaster001_2K-PNG/Plaster001_2K-PNG_Roughness.png",
        nullptr,
        "../../Utilities/textures/Plaster001_2K-PNG/Plaster001_2K-PNG_Displacement.png");

    // Plastic016A — condiment bottles (ketchup, mustard)
    // The albedo is a neutral/yellowish plastic; we tint per-object.
    LoadPBRTextureSet("pbr_plastic",
        "../../Utilities/textures/Plastic016A_2K-PNG/Plastic016A_2K-PNG_Color.png",
        "../../Utilities/textures/Plastic016A_2K-PNG/Plastic016A_2K-PNG_NormalGL.png",
        nullptr,
        "../../Utilities/textures/Plastic016A_2K-PNG/Plastic016A_2K-PNG_Roughness.png",
        nullptr,
        "../../Utilities/textures/Plastic016A_2K-PNG/Plastic016A_2K-PNG_Displacement.png");
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
    m_basicMeshes->LoadTaperedCylinderMesh();

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
    glClearColor(0.04f, 0.04f, 0.06f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    SetupLighting();

    // =================================================================
    // FLOOR — black & white checkerboard
    // =================================================================
    SetTransformations(
        glm::vec3(20.0f, 1.0f, 30.0f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(0.0f, 0.0f, 0.0f));
    SetShaderCheckerboard(
        20.0f, 30.0f,
        glm::vec3(0.92f, 0.92f, 0.92f),
        glm::vec3(0.06f, 0.06f, 0.06f));
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
    SetUVScale(6.0f, 4.0f);  // tile properly across 30-unit wall
    m_pShaderManager->setVec3Value("pbrTint", glm::vec3(0.62f, 0.78f, 0.88f));
    m_basicMeshes->DrawBoxMesh();

    // Checkerboard border strip on wall (slightly proud of wall face)
    SetTransformations(
        glm::vec3(0.03f, 0.5f, 30.0f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(7.47f, 2.9f, 0.0f));
    SetShaderCheckerboard(
        60.0f, 2.0f,
        glm::vec3(0.95f), glm::vec3(0.05f));
    m_basicMeshes->DrawBoxMesh();

    // Checkerboard wall frame — top metal strip (in front of checkerboard)
    SetTransformations(
        glm::vec3(0.035f, 0.06f, 30.0f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(7.46f, 3.18f, 0.0f));
    SetShaderTexture("stainless");
    SetUVScale(1.0f, 10.0f);
    m_basicMeshes->DrawBoxMesh();

    // Bottom metal strip
    SetTransformations(
        glm::vec3(0.035f, 0.06f, 30.0f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(7.46f, 2.62f, 0.0f));
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
    SetShaderColor(0.06f, 0.05f, 0.05f, 1.0f);
    m_basicMeshes->DrawBoxMesh();

    // =================================================================
    // BOOTHS — 5 booth units along the wall
    // =================================================================
    const int   boothCount   = 5;
    const float boothSpacing = 4.0f;
    const float boothWidth   = 3.4f;

    const float wallBackX     = 7.1f;
    const float wallSeatX     = 6.1f;
    const float tableX        = 4.75f;
    const float aisleSeatX    = 3.4f;
    const float aisleBackX    = 2.5f;

    const float seatHeight    = 0.50f;
    const float seatY         = seatHeight / 2.0f;
    const float backHeight    = 2.8f;
    const float backY         = backHeight / 2.0f;
    const float backThickness = 0.20f;
    const float seatDepth     = 1.6f;
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

        // --- Table top (Metal009 PBR) ---
        SetTransformations(
            glm::vec3(2.0f, 0.06f, 2.6f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX, tableTopY, zPos));
        SetShaderPBR("pbr_metal009");
        SetUVScale(2.0f, 2.0f);
        m_basicMeshes->DrawBoxMesh();

        // --- Chrome edge strip around table top ---
        // Front edge (overlap table by 0.01 to prevent Z-fighting)
        SetTransformations(
            glm::vec3(2.08f, 0.08f, 0.04f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX, tableTopY, zPos + 1.31f));
        SetShaderPBR("pbr_metal009");
        SetUVScale(4.0f, 1.0f);
        m_basicMeshes->DrawBoxMesh();
        // Back edge
        SetTransformations(
            glm::vec3(2.08f, 0.08f, 0.04f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX, tableTopY, zPos - 1.31f));
        SetShaderPBR("pbr_metal009");
        SetUVScale(4.0f, 1.0f);
        m_basicMeshes->DrawBoxMesh();
        // Left edge (aisle side)
        SetTransformations(
            glm::vec3(0.04f, 0.08f, 2.68f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX - 1.01f, tableTopY, zPos));
        SetShaderPBR("pbr_metal009");
        SetUVScale(1.0f, 4.0f);
        m_basicMeshes->DrawBoxMesh();
        // Right edge (wall side)
        SetTransformations(
            glm::vec3(0.04f, 0.08f, 2.68f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX + 1.01f, tableTopY, zPos));
        SetShaderPBR("pbr_metal009");
        SetUVScale(1.0f, 4.0f);
        m_basicMeshes->DrawBoxMesh();

        // --- Table pedestal (Metal009 cylinder) ---
        SetTransformations(
            glm::vec3(0.12f, tableTopY - 0.03f, 0.12f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX, 0.0f, zPos));
        SetShaderPBR("pbr_metal009");
        SetUVScale(1.0f, 2.0f);
        m_basicMeshes->DrawCylinderMesh();

        // --- Table base plate (flat Metal009 disc) ---
        SetTransformations(
            glm::vec3(0.5f, 0.04f, 0.5f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX, 0.0f, zPos));
        SetShaderPBR("pbr_metal009");
        SetUVScale(1.0f, 1.0f);
        m_basicMeshes->DrawCylinderMesh();

        // =============================================================
        // PENDANT LAMP (Metal052A shade & cable, emissive bulb)
        // =============================================================

        float ceilingY = 8.0f;

        // ---- SHADE (Metal052A) ----
        float shadeHalfHeight = 0.4f;
        float shadeTopY       = 6.5f;
        float shadeCenterY    = shadeTopY - shadeHalfHeight;

        SetTransformations(
            glm::vec3(1.3f, shadeHalfHeight, 1.3f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX, shadeCenterY, zPos));
        SetShaderPBR("pbr_metal052");
        SetUVScale(2.0f, 2.0f);
        m_basicMeshes->DrawHalfSphereMesh();

        // ---- CAP (Metal052A) ----
        float capHeight  = 0.12f;
        float capCenterY = shadeTopY;

        SetTransformations(
            glm::vec3(0.18f, capHeight, 0.18f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX, capCenterY, zPos));
        SetShaderPBR("pbr_metal052");
        SetUVScale(1.0f, 1.0f);
        m_basicMeshes->DrawHalfSphereMesh();

        // ---- CABLE (Metal052A thin cylinder) ----
        float capTopY    = capCenterY + capHeight;
        float cableLen   = ceilingY - capTopY;

        SetTransformations(
            glm::vec3(0.03f, cableLen, 0.03f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX, capTopY, zPos));
        SetShaderPBR("pbr_metal052");
        SetUVScale(1.0f, 4.0f);
        m_basicMeshes->DrawCylinderMesh();

        // ---- BULB (emissive — warm glow) ----
        SetTransformations(
            glm::vec3(0.20f, 0.20f, 0.20f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(tableX, shadeCenterY - 0.15f, zPos));
        SetShaderEmissive(1.0f, 0.92f, 0.65f, 3.0f);  // warm tungsten glow
        m_basicMeshes->DrawSphereMesh();
    }

    // =================================================================
    // CONDIMENT / NAPKIN HOLDER on each table
    // =================================================================
    for (int i = 0; i < boothCount; ++i)
    {
        float zPos = -((boothCount - 1) * boothSpacing) / 2.0f + i * boothSpacing;

        // ---- Napkin holder body (Metal009) ----
        float napkinHolderX = tableX + 0.5f;
        float napkinHolderY = tableTopY + 0.18f;

        SetTransformations(
            glm::vec3(0.25f, 0.3f, 0.15f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(napkinHolderX, napkinHolderY, zPos));
        SetShaderPBR("pbr_metal009");
        SetUVScale(1.0f, 1.0f);
        m_basicMeshes->DrawBoxMesh();

        // ---- Napkins (stack of thin white slabs poking up) ----
        // Several thin boxes inside the holder, slightly fanned
        for (int n = 0; n < 5; ++n)
        {
            float fan = (n - 2) * 0.012f;  // slight Z spread
            float yOffset = 0.02f * n;     // stagger height slightly
            SetTransformations(
                glm::vec3(0.22f, 0.26f, 0.006f),  // very thin
                0.0f, 0.0f, (n - 2) * 2.0f,       // slight tilt
                glm::vec3(napkinHolderX, napkinHolderY + yOffset, zPos + fan));
            SetShaderColor(0.96f, 0.94f, 0.90f, 1.0f);  // off-white napkin
            m_basicMeshes->DrawBoxMesh();
        }

        // ---- Ketchup bottle (Plastic PBR, red tint) ----
        float ketchupX = tableX + 0.3f;
        float ketchupZ = zPos + 0.3f;
        float ketchupBodyH = 0.25f;
        float ketchupBaseY = tableTopY + 0.03f;

        SetTransformations(
            glm::vec3(0.06f, ketchupBodyH, 0.06f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(ketchupX, ketchupBaseY, ketchupZ));
        SetShaderPBRTinted("pbr_plastic", glm::vec3(0.85f, 0.08f, 0.05f));
        SetUVScale(1.0f, 2.0f);
        m_basicMeshes->DrawCylinderMesh();

        // Ketchup nozzle (tapered cylinder — open top for squeeze opening)
        SetTransformations(
            glm::vec3(0.05f, 0.10f, 0.05f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(ketchupX, ketchupBaseY + ketchupBodyH, ketchupZ));
        SetShaderPBRTinted("pbr_plastic", glm::vec3(0.85f, 0.08f, 0.05f));
        SetUVScale(1.0f, 1.0f);
        m_basicMeshes->DrawTaperedCylinderMesh(false, false, true);  // sides only, open top

        // ---- Mustard bottle (Plastic PBR, yellow tint) ----
        float mustardX = tableX + 0.3f;
        float mustardZ = zPos + 0.15f;
        float mustardBodyH = 0.22f;
        float mustardBaseY = tableTopY + 0.03f;

        SetTransformations(
            glm::vec3(0.06f, mustardBodyH, 0.06f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(mustardX, mustardBaseY, mustardZ));
        SetShaderPBRTinted("pbr_plastic", glm::vec3(0.9f, 0.75f, 0.05f));
        SetUVScale(1.0f, 2.0f);
        m_basicMeshes->DrawCylinderMesh();

        // Mustard nozzle (tapered cylinder — open top for squeeze opening)
        SetTransformations(
            glm::vec3(0.05f, 0.10f, 0.05f),
            0.0f, 0.0f, 0.0f,
            glm::vec3(mustardX, mustardBaseY + mustardBodyH, mustardZ));
        SetShaderPBRTinted("pbr_plastic", glm::vec3(0.9f, 0.75f, 0.05f));
        SetUVScale(1.0f, 1.0f);
        m_basicMeshes->DrawTaperedCylinderMesh(false, false, true);  // sides only, open top
    }

    // =================================================================
    // WALL DECORATIONS — hubcap circles on the wall
    // =================================================================
    float hubcapX = 7.44f;

    for (int h = 0; h < 4; ++h)
    {
        float hz = -4.0f + h * 2.8f;
        float hy = 4.0f + (h % 2) * 0.6f;

        SetTransformations(
            glm::vec3(0.4f, 0.4f, 0.12f),
            0.0f, 90.0f, 0.0f,
            glm::vec3(hubcapX, hy, hz));
        SetShaderPBR("pbr_metal009");
        SetUVScale(3.0f, 3.0f);
        m_basicMeshes->DrawTorusMesh();
    }

    // =================================================================
    // NEON LIGHT TUBES along the ceiling edge (glass cylinders)
    // Rendered last for proper alpha blending.
    // =================================================================
    glDepthMask(GL_FALSE);  // don't write depth for transparent objects

    for (int n = -2; n <= 2; ++n)
    {
        float zOffset = n * 6.0f;

        // Neon tube runs along Z, so rotate cylinder 90° on X to lay flat.
        // Cylinder extends from Y=0→Y=1 locally; after 90° X rotation it
        // goes from Z=0→Z=+5 (scaled), so position at zOffset-2.5 to center.
        SetTransformations(
            glm::vec3(0.06f, 5.0f, 0.06f),   // thin cylinder, 5 units long
            90.0f, 0.0f, 0.0f,               // rotate to lay along Z
            glm::vec3(7.45f, 7.9f, zOffset - 2.5f));  // centered on zOffset
        SetShaderEmissive(1.0f, 0.12f, 0.08f, 4.0f, 0.55f);  // semi-transparent glass
        m_basicMeshes->DrawCylinderMesh(false, false, true);   // sides only
    }

    glDepthMask(GL_TRUE);  // restore depth writes
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
