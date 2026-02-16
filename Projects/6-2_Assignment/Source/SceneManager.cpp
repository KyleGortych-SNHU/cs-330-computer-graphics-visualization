///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#include <glm/gtx/transform.hpp>
#include <iostream>

// stb_image for loading texture files from disk
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// declare the global variables
namespace
{
    const char* g_ModelName      = "model";
    const char* g_ColorValueName = "objectColor";
}

/***********************************************************
 *  SceneManager()
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
    m_pShaderManager = pShaderManager;
    m_basicMeshes    = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 ***********************************************************/
SceneManager::~SceneManager()
{
    m_pShaderManager = NULL;
    if (NULL != m_basicMeshes)
    {
        delete m_basicMeshes;
        m_basicMeshes = NULL;
    }

    // Free GPU textures
    for (auto& pair : m_pbrTextures)
    {
        PBR_TEXTURE_SET& s = pair.second;
        GLuint ids[] = { s.albedoID, s.normalID, s.metallicID,
                         s.roughnessID, s.aoID, s.heightID };
        for (GLuint id : ids)
        {
            if (id != 0)
                glDeleteTextures(1, &id);
        }
    }
    m_pbrTextures.clear();
}

// ================================================================
//  Texture Loading
// ================================================================

/***********************************************************
 *  LoadSingleTexture()
 *
 *  Loads one image file into an OpenGL texture and returns
 *  the GL handle.  Returns 0 on failure.
 ***********************************************************/
GLuint SceneManager::LoadSingleTexture(const char* filepath)
{
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filepath, &width, &height, &channels, 0);
    if (!data)
    {
        std::cerr << "WARNING: Could not load texture: " << filepath << std::endl;
        return 0;
    }

    GLenum format = GL_RGB;
    if (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
                 format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return texID;
}

/***********************************************************
 *  CreateDefaultTexture()
 *
 *  Creates a tiny 1×1 texture filled with the given value.
 *  Useful as a fallback (e.g. white albedo, 0.5 roughness).
 ***********************************************************/
GLuint SceneManager::CreateDefaultTexture(unsigned char value)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    unsigned char pixel[] = { value, value, value, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixel);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return texID;
}

/***********************************************************
 *  LoadPBRTextureSet()
 *
 *  Loads a full set of PBR textures for one material and
 *  stores them in m_pbrTextures under the given tag.
 *  Any path that is nullptr gets a sensible 1×1 default.
 ***********************************************************/
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

    // Albedo — default white
    set.albedoID = albedoPath ? LoadSingleTexture(albedoPath) : 0;
    if (set.albedoID == 0) set.albedoID = CreateDefaultTexture(255);

    // Normal — default flat (128,128,255 → 0,0,1 in tangent space)
    set.normalID = normalPath ? LoadSingleTexture(normalPath) : 0;
    if (set.normalID == 0)
    {
        GLuint texID;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);
        unsigned char flat[] = { 128, 128, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, flat);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        set.normalID = texID;
    }

    // Metallic — default 0 (non-metal)
    set.metallicID = metallicPath ? LoadSingleTexture(metallicPath) : 0;
    if (set.metallicID == 0) set.metallicID = CreateDefaultTexture(0);

    // Roughness — default 128 (~0.5)
    set.roughnessID = roughnessPath ? LoadSingleTexture(roughnessPath) : 0;
    if (set.roughnessID == 0) set.roughnessID = CreateDefaultTexture(128);

    // AO — default 255 (fully lit)
    set.aoID = aoPath ? LoadSingleTexture(aoPath) : 0;
    if (set.aoID == 0) set.aoID = CreateDefaultTexture(255);

    // Height — default 128 (mid-level, no displacement)
    set.heightID = heightPath ? LoadSingleTexture(heightPath) : 0;
    if (set.heightID == 0) set.heightID = CreateDefaultTexture(128);

    m_pbrTextures[tag] = set;

    std::cout << "Loaded PBR set: " << tag << std::endl;
    return true;
}

/***********************************************************
 *  LoadSceneTextures()
 *
 *  Loads all PBR texture sets needed by the scene.
 *  Paths are relative to the working directory (project root).
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
    // Base path to the shared textures folder
    const std::string base = "../../Utilities/textures/";

    // --- Ground plane: Plaster ---
    {
        std::string dir = base + "Plaster001_2K-PNG/";
        LoadPBRTextureSet("ground",
            (dir + "Plaster001_2K-PNG_Color.png").c_str(),
            (dir + "Plaster001_2K-PNG_NormalGL.png").c_str(),
            nullptr,   // no metalness map for plaster
            (dir + "Plaster001_2K-PNG_Roughness.png").c_str(),
            nullptr,   // no AO map
            (dir + "Plaster001_2K-PNG_Displacement.png").c_str());
    }

    // --- Cylinder: Metal009 ---
    {
        std::string dir = base + "Metal009_2K-PNG/";
        LoadPBRTextureSet("cylinder",
            (dir + "Metal009_2K-PNG_Color.png").c_str(),
            (dir + "Metal009_2K-PNG_NormalGL.png").c_str(),
            (dir + "Metal009_2K-PNG_Metalness.png").c_str(),
            (dir + "Metal009_2K-PNG_Roughness.png").c_str(),
            nullptr,
            (dir + "Metal009_2K-PNG_Displacement.png").c_str());
    }

    // --- Box1: Leather ---
    {
        std::string dir = base + "Leather036D_2K-PNG/";
        LoadPBRTextureSet("box1",
            (dir + "Leather036D_2K-PNG_Color.png").c_str(),
            (dir + "Leather036D_2K-PNG_NormalGL.png").c_str(),
            nullptr,   // leather is non-metallic
            (dir + "Leather036D_2K-PNG_Roughness.png").c_str(),
            (dir + "Leather036D_2K-PNG_AmbientOcclusion.png").c_str(),
            (dir + "Leather036D_2K-PNG_Displacement.png").c_str());
    }

    // --- Box2: Rubber ---
    {
        std::string dir = base + "Rubber004_2K-PNG/";
        LoadPBRTextureSet("box2",
            (dir + "Rubber004_2K-PNG_Color.png").c_str(),
            (dir + "Rubber004_2K-PNG_NormalGL.png").c_str(),
            nullptr,
            (dir + "Rubber004_2K-PNG_Roughness.png").c_str(),
            nullptr,
            (dir + "Rubber004_2K-PNG_Displacement.png").c_str());
    }

    // --- Sphere: Metal052A ---
    {
        std::string dir = base + "Metal052A_2K-PNG/";
        LoadPBRTextureSet("sphere",
            (dir + "Metal052A_2K-PNG_Color.png").c_str(),
            (dir + "Metal052A_2K-PNG_NormalGL.png").c_str(),
            (dir + "Metal052A_2K-PNG_Metalness.png").c_str(),
            (dir + "Metal052A_2K-PNG_Roughness.png").c_str(),
            nullptr,
            (dir + "Metal052A_2K-PNG_Displacement.png").c_str());
    }

    // --- Cone: Plastic ---
    {
        std::string dir = base + "Plastic016A_2K-PNG/";
        LoadPBRTextureSet("cone",
            (dir + "Plastic016A_2K-PNG_Color.png").c_str(),
            (dir + "Plastic016A_2K-PNG_NormalGL.png").c_str(),
            nullptr,
            (dir + "Plastic016A_2K-PNG_Roughness.png").c_str(),
            nullptr,
            (dir + "Plastic016A_2K-PNG_Displacement.png").c_str());
    }
}

// ================================================================
//  Shader Helpers
// ================================================================

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
    glm::mat4 scale     = glm::scale(scaleXYZ);
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
void SceneManager::SetShaderColor(float r, float g, float b, float a)
{
    if (m_pShaderManager)
    {
        m_pShaderManager->setVec4Value(g_ColorValueName, glm::vec4(r, g, b, a));
    }
}

/***********************************************************
 *  BindPBRMaterial()
 *
 *  Activates PBR mode in the shader and binds the texture
 *  set associated with the given tag to texture units 0–5.
 ***********************************************************/
void SceneManager::BindPBRMaterial(const std::string& tag)
{
    if (!m_pShaderManager) return;

    auto it = m_pbrTextures.find(tag);
    if (it == m_pbrTextures.end())
    {
        // Fallback: use flat color, disable PBR
        m_pShaderManager->setBoolValue("bUsePBR", false);
        m_pShaderManager->setBoolValue("bUseTexture", false);
        SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);
        return;
    }

    const PBR_TEXTURE_SET& s = it->second;

    // Enable PBR path in the fragment shader
    m_pShaderManager->setBoolValue("bUsePBR", true);
    m_pShaderManager->setBoolValue("bUseTexture", false);
    m_pShaderManager->setBoolValue("bIsEmissive", false);
    m_pShaderManager->setBoolValue("bUseCheckerboard", false);

    // Bind each texture to the expected texture unit
    // (matches fragment shader: albedoMap=0, normalMap=1, metallicMap=2,
    //  roughnessMap=3, aoMap=4, heightMap=5)
    m_pShaderManager->setPBRTextures(
        s.albedoID,
        s.normalID,
        s.metallicID,
        s.roughnessID,
        s.aoID,
        s.heightID);

    // Default UV scale and tint
    m_pShaderManager->setVec2Value("UVscale", glm::vec2(1.0f, 1.0f));
    m_pShaderManager->setVec3Value("pbrTint", glm::vec3(1.0f));
}

// ================================================================
//  Scene Setup
// ================================================================

/***********************************************************
 *  SetupSceneLights()
 *
 *  Configures point lights using the uniform arrays that
 *  the fragment shader actually expects.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
    if (!m_pShaderManager) return;

    // Two point lights with distinct colors — the shader accumulates
    // both contributions per fragment, so surfaces between the lights
    // receive a natural blend of both colors.
    m_pShaderManager->setIntValue("numLights", 2);

    // Light 0 — warm orange/amber, upper-right, close to the scene
    m_pShaderManager->setVec3Value("lightPositions[0]", glm::vec3(4.0f, 5.0f, 4.0f));
    m_pShaderManager->setVec3Value("lightColors[0]",    glm::vec3(1.0f, 0.6f, 0.2f));
    m_pShaderManager->setFloatValue("lightIntensities[0]", 200.0f);

    // Light 1 — cool blue/cyan, upper-left, close to the scene
    m_pShaderManager->setVec3Value("lightPositions[1]", glm::vec3(-4.0f, 5.0f, 4.0f));
    m_pShaderManager->setVec3Value("lightColors[1]",    glm::vec3(0.05f, 0.15f, 1.0f));
    m_pShaderManager->setFloatValue("lightIntensities[1]", 200.0f);
}

/***********************************************************
 *  PrepareScene()
 ***********************************************************/
void SceneManager::PrepareScene()
{
    // Load all PBR texture sets from disk into GPU memory
    LoadSceneTextures();

    // Configure the lights
    SetupSceneLights();

    // Load the mesh geometry into GPU buffers
    m_basicMeshes->LoadBoxMesh();
    m_basicMeshes->LoadPlaneMesh();
    m_basicMeshes->LoadCylinderMesh();
    m_basicMeshes->LoadConeMesh();
    m_basicMeshes->LoadSphereMesh();
}

// ================================================================
//  Rendering
// ================================================================

/***********************************************************
 *  RenderScene()
 *
 *  Renders each object with its correct mesh, material, and
 *  transformation.
 ***********************************************************/
void SceneManager::RenderScene()
{
    // --- Ground plane ---
    SetTransformations(
        glm::vec3(20.0f, 1.0f, 10.0f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(0.0f, 0.0f, 0.0f));
    BindPBRMaterial("ground");
    m_pShaderManager->setVec2Value("UVscale", glm::vec2(4.0f, 4.0f));
    m_basicMeshes->DrawPlaneMesh();

    // --- Cylinder ---
    SetTransformations(
        glm::vec3(0.9f, 2.8f, 0.9f),
        90.0f, 0.0f, -15.0f,
        glm::vec3(0.0f, 0.9f, 0.4f));
    BindPBRMaterial("cylinder");
    m_basicMeshes->DrawCylinderMesh();

    // --- Box 1 ---
    SetTransformations(
        glm::vec3(1.0f, 9.0f, 1.3f),
        0.0f, 0.0f, 95.0f,
        glm::vec3(0.2f, 2.27f, 2.0f));
    BindPBRMaterial("box1");
    m_basicMeshes->DrawBoxMesh();

    // --- Box 2 ---
    SetTransformations(
        glm::vec3(1.7f, 1.5f, 1.5f),
        0.0f, 40.0f, 8.0f,
        glm::vec3(3.3f, 3.85f, 2.19f));
    BindPBRMaterial("box2");
    m_basicMeshes->DrawBoxMesh();

    // --- Sphere ---
    SetTransformations(
        glm::vec3(1.0f, 1.0f, 1.0f),
        0.0f, 0.0f, 0.0f,
        glm::vec3(3.2f, 5.6f, 2.5f));
    BindPBRMaterial("sphere");
    m_basicMeshes->DrawSphereMesh();

    // --- Cone ---
    SetTransformations(
        glm::vec3(1.2f, 4.0f, 1.2f),
        0.0f, 0.0f, 5.0f,
        glm::vec3(-3.3f, 2.50f, 2.0f));
    BindPBRMaterial("cone");
    m_basicMeshes->DrawConeMesh();
}
