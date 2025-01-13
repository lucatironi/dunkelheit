#include "config.hpp"
#include "footsteps_system.hpp"
#include "fps_camera.hpp"
#include "level.hpp"
#include "player_entity.hpp"
#include "quad.hpp"
#include "random_generator.hpp"
#include "shader.hpp"
#include "text_renderer.hpp"
#include "texture2D.hpp"
#include "weapon.hpp"
#include "working_directory.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <irrKlang.h>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

struct GBuffer
{
    GLuint FBO;
    GLuint gPosition;
    GLuint gNormal;
    GLuint gAlbedo;
};

void ProcessInput(GLFWwindow* window, float deltaTime);
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void MouseCallback(GLFWwindow* window, double xposIn, double yposIn);

GBuffer SetupGBuffer(int width, int height);
void SetupDeferredShaders(Shader& shaderGeometryPass, Shader& shaderLightingPass, glm::mat4 projection);
void SetupForwardShaders(Shader& shaderSinglePass, glm::mat4 projection);

// settings
std::string WindowTitle;
int WindowWidth, WindowHeight;
int WindowPositionX, WindowPositionY;
bool FullScreen, UseDeferredShading, ShowDebugInfo;

std::string ForwardShadingVertexShaderFile, ForwardShadingFragmentShaderFile;
std::string DeferredShadingFirstPassVertexShaderFile, DeferredShadingFirstPassFragmentShaderFile;
std::string DeferredShadingSecondPassVertexShaderFile, DeferredShadingSecondPassFragmentShaderFile;

FPSCamera Camera(glm::vec3(0.0f));
PlayerEntity Player(glm::vec3(0.0f), &Camera);
bool FirstMouse = true;
float LastX, LastY;

std::string FontFile;
int FontSize;
std::string TextVertexShaderFile, TextFragmentShaderFile;

bool TorchActivated = true;
glm::vec3 TorchColor;
float TorchInnerCutoff, TorchOuterCutoff;
float TorchAttenuationConstant, TorchAttenuationLinear, TorchAttenuationQuadratic;
glm::vec3 AmbientColor;
float AmbientIntensity, SpecularShininess, SpecularIntensity;
float AttenuationConstant, AttenuationLinear, AttenuationQuadratic;

std::string LevelMapFile, LevelTextureFile;

std::string LeftWeaponModelFile, LeftWeaponTextureFile;
glm::vec3 LeftWeaponPositionOffset, LeftWeaponRotationOffset, LeftWeaponScale;
std::string RightWeaponModelFile, RightWeaponTextureFile;
glm::vec3 RightWeaponPositionOffset, RightWeaponRotationOffset, RightWeaponScale;

irrklang::ISoundEngine* SoundEngine;
std::string AmbientMusicFile;
std::vector<std::string> FootstepsSoundFiles;

int main()
{
    // config: load from file
    // ----------------------
    try
    {
        std::string workingDirPath = WorkingDirectory::getPath();
        std::filesystem::current_path(workingDirPath);
        std::cout << "Current working directory set to: " << std::filesystem::current_path() << std::endl;

        // Load the configuration file
        Config& config = Config::GetInstance();
        config.LoadFromFile("config/settings.json");

        // Access individual configuration values
        WindowTitle = config.GetNested<std::string>("window.title");
        WindowWidth = config.GetNested<int>("window.width");
        WindowHeight = config.GetNested<int>("window.height");
        FullScreen = config.GetNested<bool>("window.fullScreen");
        ShowDebugInfo = config.GetNested<bool>("window.showDebugInfo");
        LastX = WindowWidth / 2.0f;
        LastY = WindowHeight / 2.0f;

        UseDeferredShading = config.GetNested<bool>("renderer.useDeferredShading");
        ForwardShadingVertexShaderFile = config.GetNested<std::string>("renderer.forwardSinglePass.shaders.vertex");
        ForwardShadingFragmentShaderFile = config.GetNested<std::string>("renderer.forwardSinglePass.shaders.fragment");
        DeferredShadingFirstPassVertexShaderFile = config.GetNested<std::string>("renderer.deferredFirstPass.shaders.vertex");
        DeferredShadingFirstPassFragmentShaderFile = config.GetNested<std::string>("renderer.deferredFirstPass.shaders.fragment");
        DeferredShadingSecondPassVertexShaderFile = config.GetNested<std::string>("renderer.deferredSecondPass.shaders.vertex");
        DeferredShadingSecondPassFragmentShaderFile = config.GetNested<std::string>("renderer.deferredSecondPass.shaders.fragment");

        FontFile = config.GetNested<std::string>("textRenderer.fontFile");
        FontSize = config.GetNested<int>("textRenderer.fontSize");
        TextVertexShaderFile = config.GetNested<std::string>("textRenderer.shaders.vertex");
        TextFragmentShaderFile = config.GetNested<std::string>("textRenderer.shaders.fragment");

        LevelMapFile = config.GetNested<std::string>("level.mapFile");
        LevelTextureFile = config.GetNested<std::string>("level.textureFile");

        LeftWeaponModelFile = config.GetNested<std::string>("weapons.left.modelFile");
        LeftWeaponTextureFile = config.GetNested<std::string>("weapons.left.textureFile");
        LeftWeaponPositionOffset = config.GetNested<glm::vec3>("weapons.left.positionOffset");
        LeftWeaponRotationOffset = config.GetNested<glm::vec3>("weapons.left.rotationOffset");
        LeftWeaponScale = config.GetNested<glm::vec3>("weapons.left.scale");
        RightWeaponModelFile = config.GetNested<std::string>("weapons.right.modelFile");
        RightWeaponTextureFile = config.GetNested<std::string>("weapons.right.textureFile");
        RightWeaponPositionOffset = config.GetNested<glm::vec3>("weapons.right.positionOffset");
        RightWeaponRotationOffset = config.GetNested<glm::vec3>("weapons.right.rotationOffset");
        RightWeaponScale = config.GetNested<glm::vec3>("weapons.right.scale");

        TorchColor = config.GetNested<glm::vec3>("lighting.torch.color");
        TorchInnerCutoff = config.GetNested<float>("lighting.torch.innerCutoff");
        TorchOuterCutoff = config.GetNested<float>("lighting.torch.outerCutoff");
        TorchAttenuationConstant = config.GetNested<float>("lighting.torch.attenuation.constant");
        TorchAttenuationLinear = config.GetNested<float>("lighting.torch.attenuation.linear");
        TorchAttenuationQuadratic = config.GetNested<float>("lighting.torch.attenuation.quadratic");
        AmbientColor = config.GetNested<glm::vec3>("lighting.ambient.color");
        AmbientIntensity = config.GetNested<float>("lighting.ambient.intensity");
        SpecularShininess = config.GetNested<float>("lighting.specular.shininess");
        SpecularIntensity = config.GetNested<float>("lighting.specular.intensity");
        AttenuationConstant = config.GetNested<float>("lighting.attenuation.constant");
        AttenuationLinear = config.GetNested<float>("lighting.attenuation.linear");
        AttenuationQuadratic = config.GetNested<float>("lighting.attenuation.quadratic");

        AmbientMusicFile = config.GetNested<std::string>("audio.ambientMusicFile");
        FootstepsSoundFiles = config.GetNested<std::vector<std::string>>("audio.footstepsSoundFiles");
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GL_FALSE);
#endif

    // glfw window creation
    // --------------------
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    GLFWwindow* window = nullptr;
    if (FullScreen) {
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        window = glfwCreateWindow(mode->width, mode->height, WindowTitle.c_str(), monitor, nullptr);
        WindowWidth = mode->width;
        WindowHeight = mode->height;
    }
    else
    {
        window = glfwCreateWindow(WindowWidth, WindowHeight, WindowTitle.c_str(), nullptr, nullptr);
        glfwGetWindowSize(window, &WindowWidth, &WindowHeight);
        glfwGetWindowPos(window, &WindowPositionX, &WindowPositionY);
    }

    if (window == nullptr)
    {
        std::cerr << "ERROR::GLFW: Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // disable vsync
    glfwSwapInterval(0);

    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetCursorPosCallback(window, MouseCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "ERROR::GLAD: Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // irrklang: initalize sound engine
    // ---------------------------------------
    SoundEngine = irrklang::createIrrKlangDevice();
    if (!SoundEngine)
    {
        std::cerr << "ERROR::IRRKLANG: Could not initialize irrklang sound engine" << std::endl;
        return -1;
    }

    // seed random generator
    RandomGenerator& random = RandomGenerator::GetInstance();
    random.SetSeed(1337);

    // load TexRenderer
    TextRenderer textRenderer(FontFile, FontSize);
    Shader textShader(TextVertexShaderFile, TextFragmentShaderFile);
    glm::mat4 orthoProjection = glm::ortho(0.0f, static_cast<float>(WindowWidth), 0.0f, static_cast<float>(WindowHeight));
    textShader.Use();
    textShader.SetMat4("projection", orthoProjection);

    // load Level
    Texture2D levelTexture(LevelTextureFile, true);
    Level level(LevelMapFile, levelTexture);
    Player.SetPosition(level.StartingPosition);

    // load Weapons
    Weapon leftWeapon(LeftWeaponModelFile, LeftWeaponTextureFile,
        LeftWeaponPositionOffset, LeftWeaponRotationOffset, LeftWeaponScale);
    Weapon rightWeapon(RightWeaponModelFile, RightWeaponTextureFile,
        RightWeaponPositionOffset, RightWeaponRotationOffset, RightWeaponScale);

    // Initialize player state and footstep system
    PlayerState playerState = { Player.Position, Player.Position, false };
    FootstepSystem footsteps(SoundEngine, FootstepsSoundFiles);

    GLfloat aspectRatio = static_cast<GLfloat>(WindowWidth) / static_cast<GLfloat>(WindowHeight);
    glm::mat4 perspectiveProjection = glm::perspective(glm::radians(80.0f), aspectRatio, 0.1f, 100.0f);

    // deferred shading setup
    Quad quad;
    GBuffer gBuffer = SetupGBuffer(WindowWidth, WindowHeight);
    Shader shaderGeometryPass(DeferredShadingFirstPassVertexShaderFile, DeferredShadingFirstPassFragmentShaderFile);
    Shader shaderLightingPass(DeferredShadingSecondPassVertexShaderFile, DeferredShadingSecondPassFragmentShaderFile);
    SetupDeferredShaders(shaderGeometryPass, shaderLightingPass, perspectiveProjection);
    level.SetLights(shaderLightingPass);

    // forward shading setup
    Shader defaultShader(ForwardShadingVertexShaderFile, ForwardShadingFragmentShaderFile);
    SetupForwardShaders(defaultShader, perspectiveProjection);
    level.SetLights(defaultShader);

    // setup OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // play ambient music
    SoundEngine->play2D(AmbientMusicFile.c_str(), true);

    // game loop
    // -----------
    float currentTime = 0.0f;
    float lastTime    = 0.0f;
    float lastFPSTime = 0.0f;
    float deltaTime   = 0.0f;
    int fpsCount = 0;
    std::stringstream fps;

    while (!glfwWindowShouldClose(window))
    {
        // calculate deltaTime and FPS
        currentTime = glfwGetTime();
        deltaTime = currentTime - lastTime;
        fpsCount++;
        if ((currentTime - lastFPSTime) >= 1.0f)
        {
            fps.str(std::string());
            fps << fpsCount;
            fpsCount = 0;
            lastFPSTime = currentTime;
        }

        // input
        // -----
        ProcessInput(window, deltaTime);

        // collision detection
        // -------------------
        for (const auto& tile : level.GetNeighboringTiles(Player.Position))
        {
            if (tile.key == TileKey::COLOR_EMPTY || tile.key == TileKey::COLOR_WALL)
            {
                glm::vec3 nearestPoint;
                nearestPoint.x = glm::clamp(Player.Position.x, tile.aabb.min.x, tile.aabb.max.x);
                nearestPoint.z = glm::clamp(Player.Position.z, tile.aabb.min.z, tile.aabb.max.z);
                glm::vec3 rayToNearest = nearestPoint - Player.Position;
                rayToNearest.y = 0.0f; // y component is irrelevant
                float overlap = Player.CollisionRadius - glm::length(rayToNearest);
                if (std::isnan(overlap))
                    overlap = 0.0f;
                if (overlap > 0.0f)
                    Player.Position -= glm::normalize(rayToNearest) * overlap;
            }
        }

        // update
        // ------
        leftWeapon.Update(Player);
        rightWeapon.Update(Player);

        playerState.position = Player.Position;
        footsteps.Update(currentTime, playerState);

        // render
        // ------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (UseDeferredShading)
        {
            // deferred shading
            // 1. geometry pass: render scene's geometry/color data into gbuffer
            glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.FBO);
                shaderGeometryPass.Use();
                shaderGeometryPass.SetMat4("view", Camera.GetViewMatrix());
                level.Draw(shaderGeometryPass);
                glClear(GL_DEPTH_BUFFER_BIT);
                leftWeapon.Draw(shaderGeometryPass);
                rightWeapon.Draw(shaderGeometryPass);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // 2. lighting pass: traditional deferred Blinn-Phong lighting
            shaderLightingPass.Use();
            shaderLightingPass.SetVec3("cameraPos", Camera.Position);
            shaderLightingPass.SetVec3("cameraDir", Camera.Front);
            shaderLightingPass.SetFloat("time", currentTime);
            shaderLightingPass.SetBool("torchActivated", TorchActivated);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gBuffer.gPosition);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gBuffer.gNormal);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, gBuffer.gAlbedo);
            quad.Draw();
        }
        else
        {
            // forward shading
            defaultShader.Use();
            defaultShader.SetMat4("view", Camera.GetViewMatrix());
            defaultShader.SetVec3("cameraPos", Camera.Position);
            defaultShader.SetVec3("cameraDir", Camera.Front);
            defaultShader.SetFloat("time", currentTime);
            defaultShader.SetBool("torchActivated", TorchActivated);
            level.Draw(defaultShader);
            glClear(GL_DEPTH_BUFFER_BIT);
            leftWeapon.Draw(defaultShader);
            rightWeapon.Draw(defaultShader);
        }

        // render Debug Information
        // save current blending state
        GLboolean blendEnabled = glIsEnabled(GL_BLEND);
        GLint srcAlphaFunc, dstAlphaFunc;
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlphaFunc);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlphaFunc);

        // enable alpha blending and set blend function
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        std::stringstream fpsText;
        fpsText << "FPS: " << fps.str();
        textRenderer.RenderText(fpsText.str(), textShader, 4.0f, WindowHeight - 20.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
        if (ShowDebugInfo)
        {
            std::stringstream windowSize;
            windowSize << WindowWidth << "x" << WindowHeight;
            textRenderer.RenderText(windowSize.str(), textShader, 4.0f, WindowHeight - 40.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
            std::string shadingMode = UseDeferredShading ? "Deferred" : "Forward";
            textRenderer.RenderText(shadingMode, textShader, 4.0f, WindowHeight - 60.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
            std::stringstream pos;
            pos << "pos x: " << (int)Player.Position.x << ", z: " << (int)Player.Position.z << ", tile: " << level.GetTile(Player.Position).key;
            textRenderer.RenderText(pos.str(), textShader, 4.0f, WindowHeight - 80.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
        }

        // restore previous blending state
        glBlendFunc(srcAlphaFunc, dstAlphaFunc);
        if (!blendEnabled)
            glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        lastTime = currentTime;
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    SoundEngine->drop();

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

/// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void ProcessInput(GLFWwindow* window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (TorchActivated && glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
        TorchActivated = false;

    if (!TorchActivated && glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
        TorchActivated = true;

    if (UseDeferredShading && glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        UseDeferredShading = false;

    if (!UseDeferredShading && glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        UseDeferredShading = true;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        Player.Move(MOVE_FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        Player.Move(MOVE_BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        Player.Move(MOVE_LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        Player.Move(MOVE_RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void FramebufferSizeCallback(GLFWwindow* /* window */, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void MouseCallback(GLFWwindow* /* window */, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (FirstMouse)
    {
        LastX = xpos;
        LastY = ypos;
        FirstMouse = false;
    }

    float xoffset = xpos - LastX;
    float yoffset = LastY - ypos; // reversed since y-coordinates go from bottom to top

    LastX = xpos;
    LastY = ypos;

    Camera.ProcessMouseMovement(xoffset, yoffset);
}

GBuffer SetupGBuffer(int width, int height)
{
    GBuffer gBuffer;

    glGenFramebuffers(1, &gBuffer.FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.FBO);

    // position color buffer
    glGenTextures(1, &gBuffer.gPosition);
    glBindTexture(GL_TEXTURE_2D, gBuffer.gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gBuffer.gPosition, 0);

    // normal color buffer
    glGenTextures(1, &gBuffer.gNormal);
    glBindTexture(GL_TEXTURE_2D, gBuffer.gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gBuffer.gNormal, 0);

    // albedo color buffer
    glGenTextures(1, &gBuffer.gAlbedo);
    glBindTexture(GL_TEXTURE_2D, gBuffer.gAlbedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gBuffer.gAlbedo, 0);

    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    // depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR::GBUFFER: Framebuffer not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return gBuffer;
}

void SetupDeferredShaders(Shader& shaderGeometryPass, Shader& shaderLightingPass, glm::mat4 projection)
{
    shaderGeometryPass.Use();
    shaderGeometryPass.SetMat4("projection", projection);

    shaderLightingPass.Use();
    shaderLightingPass.SetInt("gPosition", 0);
    shaderLightingPass.SetInt("gNormal", 1);
    shaderLightingPass.SetInt("gAlbedo", 2);
    shaderLightingPass.SetVec3("torchColor", TorchColor);
    shaderLightingPass.SetFloat("torchInnerCutoff", glm::cos(glm::radians(TorchInnerCutoff)));
    shaderLightingPass.SetFloat("torchOuterCutoff", glm::cos(glm::radians(TorchOuterCutoff)));
    shaderLightingPass.SetFloat("torchAttenuationConstant", TorchAttenuationConstant);
    shaderLightingPass.SetFloat("torchAttenuationLinear", TorchAttenuationLinear);
    shaderLightingPass.SetFloat("torchAttenuationQuadratic", TorchAttenuationQuadratic);
    shaderLightingPass.SetVec3("ambientColor", AmbientColor);
    shaderLightingPass.SetFloat("ambientIntensity", AmbientIntensity);
    shaderLightingPass.SetFloat("specularShininess", SpecularShininess);
    shaderLightingPass.SetFloat("specularIntensity", SpecularIntensity);
    shaderLightingPass.SetFloat("attenuationConstant", AttenuationConstant);
    shaderLightingPass.SetFloat("attenuationLinear", AttenuationLinear);
    shaderLightingPass.SetFloat("attenuationQuadratic", AttenuationQuadratic);
}

void SetupForwardShaders(Shader& shaderSinglePass, glm::mat4 projection)
{
    shaderSinglePass.Use();
    shaderSinglePass.SetMat4("projection", projection);
    shaderSinglePass.SetVec3("torchColor", TorchColor);
    shaderSinglePass.SetFloat("torchInnerCutoff", glm::cos(glm::radians(TorchInnerCutoff)));
    shaderSinglePass.SetFloat("torchOuterCutoff", glm::cos(glm::radians(TorchOuterCutoff)));
    shaderSinglePass.SetFloat("torchAttenuationConstant", TorchAttenuationConstant);
    shaderSinglePass.SetFloat("torchAttenuationLinear", TorchAttenuationLinear);
    shaderSinglePass.SetFloat("torchAttenuationQuadratic", TorchAttenuationQuadratic);
    shaderSinglePass.SetVec3("ambientColor", AmbientColor);
    shaderSinglePass.SetFloat("ambientIntensity", AmbientIntensity);
    shaderSinglePass.SetFloat("specularShininess", SpecularShininess);
    shaderSinglePass.SetFloat("specularIntensity", SpecularIntensity);
    shaderSinglePass.SetFloat("attenuationConstant", AttenuationConstant);
    shaderSinglePass.SetFloat("attenuationLinear", AttenuationLinear);
    shaderSinglePass.SetFloat("attenuationQuadratic", AttenuationQuadratic);
}