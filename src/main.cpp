#include "entity.hpp"
#include "fps_camera.hpp"
#include "item.hpp"
#include "json_file.hpp"
#include "level.hpp"
#include "pixelator.hpp"
#include "player_audio_system.hpp"
#include "random_generator.hpp"
#include "settings.hpp"
#include "shader.hpp"
#include "text_renderer.hpp"
#include "texture_2D.hpp"
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
void CursorPosCallback(GLFWwindow* window, double xposIn, double yposIn);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

void SetupShaders(const Shader& shader);
void CalculateFPS(float& lastTime, float& lastFPSTime, float& deltaTime, int& frames, int& fps);
void HandleCollisions(FPSCamera& camera, const Level& level);
void Render(const Shader& shader);
void RenderDebugInfo(TextRenderer& textRenderer, Shader& textShader, const int fps);

void Shoot();

SettingsData Settings;
FPSCamera Camera;
PlayerState Player;
std::vector<Entity*> Entities;
PlayerAudioSystem* PlayerAudio;

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

        Settings = LoadSettingsFile("config/settings.json");
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
    if (Settings.FullScreen) {
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        window = glfwCreateWindow(mode->width, mode->height, Settings.WindowTitle.c_str(), monitor, nullptr);
        Settings.WindowWidth = mode->width;
        Settings.WindowHeight = mode->height;
    }
    else
    {
        window = glfwCreateWindow(Settings.WindowWidth, Settings.WindowHeight, Settings.WindowTitle.c_str(), nullptr, nullptr);
        glfwGetWindowSize(window, &Settings.WindowWidth, &Settings.WindowHeight);
        glfwGetWindowPos(window, &Settings.WindowPositionX, &Settings.WindowPositionY);
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
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);

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
    TextRenderer textRenderer(Settings.FontFile, Settings.FontSize);
    Shader textShader(Settings.TextVertexShaderFile, Settings.TextFragmentShaderFile);
    glm::mat4 orthoProjection = glm::ortho(0.0f, static_cast<float>(Settings.WindowWidth), 0.0f, static_cast<float>(Settings.WindowHeight));
    textShader.Use();
    textShader.SetMat4("projection", orthoProjection);

    // load Level
    Texture2D levelTexture(Settings.LevelTextureFile, true);
    Level level(Settings.LevelMapFile, levelTexture);
    Entities.push_back(&level);

    // load camera
    Camera.Constrained = true;
    Camera.FOV = Settings.FOV;
    Camera.AspectRatio = static_cast<GLfloat>(Settings.WindowWidth) / static_cast<GLfloat>(Settings.WindowHeight);
    Camera.Position = level.StartingPosition;
    Camera.MovementSpeed = Settings.PlayerSpeed;
    Camera.HeadHeight = Settings.PlayerHeadHeight;

    Pixelator pixelator(Settings.WindowWidth, Settings.WindowHeight, Settings.WindowWidth / 4.0f, Settings.WindowHeight / 4.0f);

    // load Weapons
    Item leftWeapon(Settings.LeftWeaponModelFile, Settings.LeftWeaponTextureFile,
        Settings.LeftWeaponPositionOffset, Settings.LeftWeaponRotationOffset, Settings.LeftWeaponScale);
    Item rightWeapon(Settings.RightWeaponModelFile, Settings.RightWeaponTextureFile,
        Settings.RightWeaponPositionOffset, Settings.RightWeaponRotationOffset, Settings.RightWeaponScale);
    Entities.push_back(&leftWeapon);
    Entities.push_back(&rightWeapon);

    // initialize player state and footstep system
    Player.Position = Camera.Position;
    Player.PreviousPosition = Camera.Position;
    Player.IsMoving = false;
    Player.IsTorchOn = true;
    PlayerAudio = new PlayerAudioSystem(SoundEngine, Settings.FootstepsSoundFiles, Settings.TorchOnSoundFile, Settings.TorchOffSoundFile);

    Shader defaultShader(Settings.ForwardShadingVertexShaderFile, Settings.ForwardShadingFragmentShaderFile);
    SetupShaders(defaultShader);
    level.SetLights(defaultShader);

    // setup OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // play ambient music
    SoundEngine->play2D(Settings.AmbientMusicFile.c_str(), true);

    // game loop
    // -----------
    float lastTime    = 0.0f;
    float lastFPSTime = 0.0f;
    float deltaTime   = 0.0f;
    int frames = 0;
    int fps    = 0;

    while (!glfwWindowShouldClose(window))
    {
        // calculate deltaTime and FPS
        // ---------------------------
        CalculateFPS(lastTime, lastFPSTime, deltaTime, frames, fps);

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
        PlayerAudio->Update(Player, CurrentTime);


        // render
        // ------
        if (Settings.Pixelate)
            pixelator.BeginRender();

        Render(defaultShader);

        if (Settings.Pixelate)
            pixelator.EndRender();

        if (Settings.ShowDebugInfo)
            RenderDebugInfo(textRenderer, textShader, fps);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
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

// glfw: whenever a keyboard key is pressed, this callback is called
// -----------------------------------------------------------------
void KeyCallback(GLFWwindow* window, int key, int /* scancode */, int action, int /* mods */)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        Player.IsTorchOn = !Player.IsTorchOn;
        PlayerAudio->ToggleTorch(Player);
    }
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
        Settings.ShowDebugInfo = !Settings.ShowDebugInfo;
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
        Settings.Pixelate = !Settings.Pixelate;
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void CursorPosCallback(GLFWwindow* /* window */, double xposIn, double yposIn)
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

// glfw: whenever a mouse button is clicked, this callback is called
// -----------------------------------------------------------------
void MouseButtonCallback(GLFWwindow* window, int button, int action, int /* mods */)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        Shoot();
}

void SetupShaders(const Shader& shader)
{
    shader.Use();
    shader.SetMat4("projection", Camera.GetProjectionMatrix());
    shader.SetInt("texture_diffuse0", 0);
    shader.SetInt("texture_specular0", 1);
    shader.SetVec3("torchColor", Settings.TorchColor);
    shader.SetFloat("torchInnerCutoff", glm::cos(glm::radians(Settings.TorchInnerCutoff)));
    shader.SetFloat("torchOuterCutoff", glm::cos(glm::radians(Settings.TorchOuterCutoff)));
    shader.SetFloat("torchAttenuationConstant", Settings.TorchAttenuationConstant);
    shader.SetFloat("torchAttenuationLinear", Settings.TorchAttenuationLinear);
    shader.SetFloat("torchAttenuationQuadratic", Settings.TorchAttenuationQuadratic);
    shader.SetVec3("ambientColor", Settings.AmbientColor);
    shader.SetFloat("ambientIntensity", Settings.AmbientIntensity);
    shader.SetFloat("specularShininess", Settings.SpecularShininess);
    shader.SetFloat("specularIntensity", Settings.SpecularIntensity);
    shader.SetFloat("attenuationConstant", Settings.AttenuationConstant);
    shader.SetFloat("attenuationLinear", Settings.AttenuationLinear);
    shader.SetFloat("attenuationQuadratic", Settings.AttenuationQuadratic);
}

void CalculateFPS(float& lastTime, float& lastFPSTime, float& deltaTime, int& frames, int& fps)
{
    CurrentTime = glfwGetTime();
    deltaTime = CurrentTime - lastTime;
    frames++;
    if ((CurrentTime - lastFPSTime) >= 1.0f)
    {
        fps = frames;
        frames = 0;
        lastFPSTime = CurrentTime;
    }
    lastTime = CurrentTime;
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
            float overlap = Settings.PlayerCollisionRadius - glm::length(rayToNearest);
            if (std::isnan(overlap))
                overlap = 0.0f;
            if (overlap > 0.0f)
                Camera.Position -= glm::normalize(rayToNearest) * overlap;
        }
    }
}

void Render(const Shader& shader)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader.Use();
    shader.SetMat4("view", Camera.GetViewMatrix());
    shader.SetVec3("cameraPos", Camera.Position);
    shader.SetVec3("cameraDir", Camera.Front);
    shader.SetFloat("time", CurrentTime);
    shader.SetBool("torchActivated", Player.IsTorchOn);

    for (const auto& entity : Entities)
    {
        if (entity->AlwaysOnTop)
            glClear(GL_DEPTH_BUFFER_BIT);
        entity->Draw(shader);
    }
}

void RenderDebugInfo(TextRenderer& textRenderer, Shader& textShader, const int fps)
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
    textRenderer.RenderText(fpsText.str(), textShader, 4.0f, Settings.WindowHeight - 20.0f, 1.0f, Settings.FontColor);
    std::stringstream windowSize;
    windowSize << Settings.WindowWidth << "x" << Settings.WindowHeight;
    textRenderer.RenderText(windowSize.str(), textShader, 4.0f, Settings.WindowHeight - 40.0f, 1.0f, Settings.FontColor);
    std::stringstream pos;
    pos << "pos x: " << (int)Camera.Position.x << ", z: " << (int)Camera.Position.z;
    textRenderer.RenderText(pos.str(), textShader, 4.0f, Settings.WindowHeight - 60.0f, 1.0f, Settings.FontColor);

    // restore previous blending state
    glBlendFunc(srcAlphaFunc, dstAlphaFunc);
    if (!blendEnabled)
        glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void Shoot()
{
    std::cout << "Pew!" << std::endl;
}