#include "entity.hpp"
#include "fps_camera.hpp"
#include "item.hpp"
#include "json_file.hpp"
#include "level.hpp"
#include "player_audio_system.hpp"
#include "random_generator.hpp"
#include "settings.hpp"
#include "shader.hpp"
#include "text_renderer.hpp"
#include "texture2D.hpp"
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

void ProcessInput(GLFWwindow* window, float deltaTime);
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MouseCallback(GLFWwindow* window, double xposIn, double yposIn);

void SetupShaders(const Shader& shader);
void CalculateFPS(float& lastTime, float& lastFPSTime, float& deltaTime, int& fpsCount, std::stringstream& fps);
void HandleCollisions(FPSCamera& camera, const Level& level);
void Render(const std::vector<Entity*>& entities, const Shader& shader);
void RenderDebugInfo(TextRenderer& textRenderer, Shader& textShader, const std::string& fps, const SettingsData& settings);

SettingsData settings;
FPSCamera Camera;
PlayerState Player;
std::vector<Entity*> Entities;

float CurrentTime = 0.0f;
bool FirstMouse = true;
float LastX, LastY;

irrklang::ISoundEngine* SoundEngine;

int main()
{
    // config: load from file
    // ----------------------
    try
    {
        std::filesystem::current_path(WorkingDirectory::getPath());
        std::cout << "Current working directory set to: " << std::filesystem::current_path() << std::endl;

        settings = LoadSettingsFile("config/settings.json");
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
    if (settings.FullScreen) {
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        window = glfwCreateWindow(mode->width, mode->height, settings.WindowTitle.c_str(), monitor, nullptr);
        settings.WindowWidth = mode->width;
        settings.WindowHeight = mode->height;
    }
    else
    {
        window = glfwCreateWindow(settings.WindowWidth, settings.WindowHeight, settings.WindowTitle.c_str(), nullptr, nullptr);
        glfwGetWindowSize(window, &settings.WindowWidth, &settings.WindowHeight);
        glfwGetWindowPos(window, &settings.WindowPositionX, &settings.WindowPositionY);
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
    glfwSetKeyCallback(window, KeyCallback);
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
    TextRenderer textRenderer(settings.FontFile, settings.FontSize);
    Shader textShader(settings.TextVertexShaderFile, settings.TextFragmentShaderFile);
    glm::mat4 orthoProjection = glm::ortho(0.0f, static_cast<float>(settings.WindowWidth), 0.0f, static_cast<float>(settings.WindowHeight));
    textShader.Use();
    textShader.SetMat4("projection", orthoProjection);

    // load Level
    Texture2D levelTexture(settings.LevelTextureFile, true);
    Level level(settings.LevelMapFile, levelTexture);
    Entities.push_back(&level);

    // load camera
    Camera.Constrained = true;
    Camera.FOV = settings.FOV;
    Camera.AspectRatio = static_cast<GLfloat>(settings.WindowWidth) / static_cast<GLfloat>(settings.WindowHeight);
    Camera.Position = level.StartingPosition;
    Camera.MovementSpeed = settings.PlayerSpeed;
    Camera.HeadHeight = settings.PlayerHeadHeight;

    // load Weapons
    Item leftWeapon(settings.LeftWeaponModelFile, settings.LeftWeaponTextureFile,
        settings.LeftWeaponPositionOffset, settings.LeftWeaponRotationOffset, settings.LeftWeaponScale);
    Item rightWeapon(settings.RightWeaponModelFile, settings.RightWeaponTextureFile,
        settings.RightWeaponPositionOffset, settings.RightWeaponRotationOffset, settings.RightWeaponScale);
    Entities.push_back(&leftWeapon);
    Entities.push_back(&rightWeapon);

    // initialize player state and footstep system
    Player.Position = Camera.Position;
    Player.PreviousPosition = Camera.Position;
    Player.IsMoving = false;
    Player.IsTorchOn = true;
    PlayerAudioSystem playerAudioSystem(SoundEngine, settings.FootstepsSoundFiles);

    Shader defaultShader(settings.ForwardShadingVertexShaderFile, settings.ForwardShadingFragmentShaderFile);
    SetupShaders(defaultShader);
    level.SetLights(defaultShader);

    // setup OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // play ambient music
    SoundEngine->play2D(settings.AmbientMusicFile.c_str(), true);

    // game loop
    // -----------
    float lastTime    = 0.0f;
    float lastFPSTime = 0.0f;
    float deltaTime   = 0.0f;
    int fpsCount = 0;
    std::stringstream fps;

    while (!glfwWindowShouldClose(window))
    {
        // calculate deltaTime and FPS
        // ---------------------------
        CurrentTime = glfwGetTime();
        CalculateFPS(lastTime, lastFPSTime, deltaTime, fpsCount, fps);

        // input
        // -----
        ProcessInput(window, deltaTime);

        // collision detection
        // -------------------
        HandleCollisions(Camera, level);

        // update
        // ------
        leftWeapon.Update(Camera);
        rightWeapon.Update(Camera);

        Player.Position = Camera.Position;
        playerAudioSystem.Update(Player, CurrentTime);

        // render
        // ------
        Render(Entities, defaultShader);

        RenderDebugInfo(textRenderer, textShader, fps.str(), settings);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();

        lastTime = CurrentTime;
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
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        Camera.Move(MOVE_FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        Camera.Move(MOVE_BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        Camera.Move(MOVE_LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        Camera.Move(MOVE_RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void FramebufferSizeCallback(GLFWwindow* /* window */, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void KeyCallback(GLFWwindow* window, int key, int /* scancode */, int action, int /* mods */)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (key == GLFW_KEY_F && action == GLFW_PRESS)
        Player.IsTorchOn = !Player.IsTorchOn;
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

void SetupShaders(const Shader& shader)
{
    shader.Use();
    shader.SetMat4("projection", Camera.GetProjectionMatrix());
    shader.SetInt("texture_diffuse0", 0);
    shader.SetInt("texture_specular0", 1);
    shader.SetVec3("torchColor", settings.TorchColor);
    shader.SetFloat("torchInnerCutoff", glm::cos(glm::radians(settings.TorchInnerCutoff)));
    shader.SetFloat("torchOuterCutoff", glm::cos(glm::radians(settings.TorchOuterCutoff)));
    shader.SetFloat("torchAttenuationConstant", settings.TorchAttenuationConstant);
    shader.SetFloat("torchAttenuationLinear", settings.TorchAttenuationLinear);
    shader.SetFloat("torchAttenuationQuadratic", settings.TorchAttenuationQuadratic);
    shader.SetVec3("ambientColor", settings.AmbientColor);
    shader.SetFloat("ambientIntensity", settings.AmbientIntensity);
    shader.SetFloat("specularShininess", settings.SpecularShininess);
    shader.SetFloat("specularIntensity", settings.SpecularIntensity);
    shader.SetFloat("attenuationConstant", settings.AttenuationConstant);
    shader.SetFloat("attenuationLinear", settings.AttenuationLinear);
    shader.SetFloat("attenuationQuadratic", settings.AttenuationQuadratic);
}

void CalculateFPS(float& lastTime, float& lastFPSTime, float& deltaTime, int& fpsCount, std::stringstream& fps)
{
    deltaTime = CurrentTime - lastTime;
    fpsCount++;
    if ((CurrentTime - lastFPSTime) >= 1.0f)
    {
        fps.str(std::string());
        fps << fpsCount;
        fpsCount = 0;
        lastFPSTime = CurrentTime;
    }
}

void HandleCollisions(FPSCamera& camera, const Level& level)
{
    for (const auto& tile : level.GetNeighboringTiles(Camera.Position))
    {
        if (tile.key == TileKey::COLOR_WALL || tile.key == TileKey::COLOR_EMPTY)
        {
            glm::vec3 nearestPoint;
            nearestPoint.x = glm::clamp(Camera.Position.x, tile.aabb.min.x, tile.aabb.max.x);
            nearestPoint.z = glm::clamp(Camera.Position.z, tile.aabb.min.z, tile.aabb.max.z);
            glm::vec3 rayToNearest = nearestPoint - Camera.Position;
            rayToNearest.y = 0.0f; // y component is irrelevant
            float overlap = settings.PlayerCollisionRadius - glm::length(rayToNearest);
            if (std::isnan(overlap))
                overlap = 0.0f;
            if (overlap > 0.0f)
                Camera.Position -= glm::normalize(rayToNearest) * overlap;
        }
    }
}

void Render(const std::vector<Entity*>& entities, const Shader& shader)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader.Use();
    shader.SetMat4("view", Camera.GetViewMatrix());
    shader.SetVec3("cameraPos", Camera.Position);
    shader.SetVec3("cameraDir", Camera.Front);
    shader.SetFloat("time", CurrentTime);
    shader.SetBool("torchActivated", Player.IsTorchOn);

    for (const auto& entity : entities)
    {
        if (entity->AlwaysOnTop)
            glClear(GL_DEPTH_BUFFER_BIT);
        entity->Draw(shader);
    }
}

void RenderDebugInfo(TextRenderer& textRenderer, Shader& textShader, const std::string& fps, const SettingsData& settings)
{
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
    fpsText << "FPS: " << fps;
    textRenderer.RenderText(fpsText.str(), textShader, 4.0f, settings.WindowHeight - 20.0f, 1.0f, settings.FontColor);
    if (settings.ShowDebugInfo)
    {
        std::stringstream windowSize;
        windowSize << settings.WindowWidth << "x" << settings.WindowHeight;
        textRenderer.RenderText(windowSize.str(), textShader, 4.0f, settings.WindowHeight - 40.0f, 1.0f, settings.FontColor);
        std::stringstream pos;
        pos << "pos x: " << (int)Camera.Position.x << ", z: " << (int)Camera.Position.z;
        textRenderer.RenderText(pos.str(), textShader, 4.0f, settings.WindowHeight - 60.0f, 1.0f, settings.FontColor);
    }

    // restore previous blending state
    glBlendFunc(srcAlphaFunc, dstAlphaFunc);
    if (!blendEnabled)
        glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}