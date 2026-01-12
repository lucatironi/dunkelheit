#include "audio_engine.hpp"
#include "entity.hpp"
#include "fps_camera.hpp"
#include "game_scene.hpp"
#include "item.hpp"
#include "level.hpp"
#include "main_menu.hpp"
#include "pixelator.hpp"
#include "player_audio_system.hpp"
#include "random_generator.hpp"
#include "settings.hpp"
#include "shader.hpp"
#include "text_renderer.hpp"
#include "texture_2D.hpp"
#include "torch.hpp"
#include "working_directory.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

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
void Render(const Shader& shader);
void RenderDebugInfo(TextRenderer& textRenderer, Shader& textShader, const int fps);
void SetupMenu(GLFWwindow* window);
void Restart();

void Shoot();

SettingsData Settings;
FPSCamera Camera;
PlayerState Player;
Torch TorchLight;
PlayerAudioSystem* PlayerAudio;
GameScene* Scene;
MainMenu* Menu;

float CurrentTime = 0.0f;
bool FirstMouse = true;
float LastX, LastY;

bool GameStarted = false;

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
#endif

    // glfw: window creation
    // --------------------
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    GLFWwindow* window = nullptr;
    if (Settings.FullScreen)
    {
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        window = glfwCreateWindow(mode->width, mode->height, Settings.WindowTitle.c_str(), monitor, nullptr);
        Settings.WindowWidth = mode->width;
        Settings.WindowHeight = mode->height;
    }
    else
    {
        window = glfwCreateWindow(Settings.WindowWidth, Settings.WindowHeight, Settings.WindowTitle.c_str(), nullptr, nullptr);
        glfwGetWindowPos(window, &Settings.WindowPositionX, &Settings.WindowPositionY);
    }

    int framebufferW, framebufferH;
    glfwGetFramebufferSize(window, &framebufferW, &framebufferH);

    Settings.FrameBufferWidth = framebufferW;
    Settings.FrameBufferHeight = framebufferH;

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
    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0)
    {
        std::cerr << "ERROR::GLAD: Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Successfully loaded OpenGL
    std::cout << "Loaded OpenGL " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << std::endl;

    // seed random generator
    RandomGenerator& random = RandomGenerator::GetInstance();
    random.SetSeed(1337);

    // load TexRenderer
    TextRenderer textRenderer(Settings.FontFile, Settings.FontSize);
    Shader textShader(Settings.TextVertexShaderFile, Settings.TextFragmentShaderFile);
    glm::mat4 orthoProjection = glm::ortho(0.0f, static_cast<float>(Settings.WindowWidth), 0.0f, static_cast<float>(Settings.WindowHeight));
    textShader.Use();
    textShader.SetMat4("projectionMatrix", orthoProjection);

    // main menu
    Menu = new MainMenu(Settings.MenuItemClickSoundFile);
    Menu->Active = true;
    SetupMenu(window);

    // load GameScene
    Scene = new GameScene(Settings);
    // load items
    Scene->AddItem(Settings.LeftWeaponModelFile, Settings.LeftWeaponTextureFile,
        Settings.LeftWeaponPositionOffset, Settings.LeftWeaponRotationOffset, Settings.LeftWeaponScale);
    Scene->AddItem(Settings.RightWeaponModelFile, Settings.RightWeaponTextureFile,
        Settings.RightWeaponPositionOffset, Settings.RightWeaponRotationOffset, Settings.RightWeaponScale);

    // load camera
    Camera.Constrained = true;
    Camera.FOV = Settings.FOV;
    Camera.AspectRatio = static_cast<GLfloat>(Settings.WindowWidth) / static_cast<GLfloat>(Settings.WindowHeight);
    Camera.Position = Scene->GetStartingPosition();
    Camera.MovementSpeed = Settings.PlayerSpeed;
    Camera.HeadHeight = Settings.PlayerHeadHeight;

    // load post processing
    Pixelator pixelator(
        (GLuint)(Settings.FrameBufferWidth / Settings.PixelScale),
        (GLuint)(Settings.FrameBufferHeight / Settings.PixelScale),
        Settings.FrameBufferWidth,
        Settings.FrameBufferHeight
    );

    // initialize player state and audio system
    Player.Init(Camera);
    PlayerAudio = new PlayerAudioSystem(Settings.FootstepsSoundFiles, Settings.TorchToggleSoundFile);

    TorchLight.PositionOffset = Settings.TorchPos;

    Shader defaultShader(Settings.ForwardShadingVertexShaderFile, Settings.ForwardShadingFragmentShaderFile);
    SetupShaders(defaultShader);
    Scene->SetLights(defaultShader);

    // setup OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // play ambient music
    AudioEngine::GetInstance().LoopSound(Settings.AmbientMusicFile, 0.5f);
    AudioEngine::GetInstance().AddEmitter(Settings.GizmoSoundFile, glm::vec3(23.0f, 1.5f, 139.0f));

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

        // update
        // ------
        // Only process movement and game world if menu is closed
        if (!Menu->Active)
        {
            ProcessInput(window, deltaTime);

            Scene->Update(deltaTime, Camera);
            Player.Position = Camera.Position;
            Player.Front = Camera.Front;
            PlayerAudio->Update(Player, CurrentTime);
            TorchLight.Update(Camera);
        }

        // render
        // ------
        if (Settings.Pixelate)
            pixelator.BeginRender();

        Render(defaultShader);

        if (Settings.Pixelate)
            pixelator.EndRender();

        if (Settings.ShowDebugInfo)
            RenderDebugInfo(textRenderer, textShader, fps);

        // Render Menu last (so it's on top of everything)
        if (Menu->Active)
            Menu->Render(textRenderer, textShader, Settings.WindowWidth, Settings.WindowHeight);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    delete Scene;
    delete PlayerAudio;
    delete Menu;

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
    if (action != GLFW_PRESS) return;

    // 1. Global Toggle
    if (key == GLFW_KEY_ESCAPE)
    {
        Menu->Active = !Menu->Active;
        if (Menu->Active)
        {
            AudioEngine::GetInstance().PlayOneShotSound(Settings.MenuOpenSoundFile);
            Scene->ToggleSounds(true);
            SetupMenu(window); // Refresh items (Switch Start -> Resume)
        }
        else
        {
            AudioEngine::GetInstance().PlayOneShotSound(Settings.MenuCloseSoundFile);
            Scene->ToggleSounds(false);
        }
        return;
    }

    // 2. Menu Input (If active)
    if (Menu->Active)
    {
        if (key == GLFW_KEY_UP)    Menu->NavigateUp();
        if (key == GLFW_KEY_DOWN)  Menu->NavigateDown();
        if (key == GLFW_KEY_ENTER) Menu->Confirm();
        return; // BLOCK game input
    }

    // 3. Game Input (Only if menu is NOT active)
    if (key == GLFW_KEY_F)
    {
        PlayerAudio->ToggleTorch(Player);
        Player.IsTorchOn = !Player.IsTorchOn;
    }
    if (key == GLFW_KEY_O)
        Settings.ShowDebugInfo = !Settings.ShowDebugInfo;
    if (key == GLFW_KEY_P)
        Settings.Pixelate = !Settings.Pixelate;
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void CursorPosCallback(GLFWwindow* /* window */, double xposIn, double yposIn)
{
    if (!Menu->Active)
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
    shader.SetMat4("projectionMatrix", Camera.GetProjectionMatrix());
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

void Render(const Shader& shader)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader.Use();
    shader.SetMat4("viewMatrix", Camera.GetViewMatrix());
    shader.SetVec3("cameraPos", Camera.Position);
    shader.SetVec3("torchPos", TorchLight.Position);
    shader.SetVec3("torchDir", TorchLight.Direction);
    shader.SetFloat("time", CurrentTime);
    shader.SetBool("torchActivated", Player.IsTorchOn);
    shader.SetBool("menuActive", Menu->Active);

    Scene->Draw(shader);
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

    textRenderer.BeginBatch();

    std::string fpsStr = "FPS: " + std::to_string(fps);
    textRenderer.AddText(fpsStr, 4.0f, Settings.WindowHeight - 20.0f, 1.0f);
    std::string winStr = std::to_string(Settings.WindowWidth) + "x" + std::to_string(Settings.WindowHeight);
    textRenderer.AddText(winStr, 4.0f, Settings.WindowHeight - 40.0f, 1.0f);
    std::string posStr = "pos x: " + std::to_string((int)Camera.Position.x) + ", z: " + std::to_string((int)Camera.Position.z);
    textRenderer.AddText(posStr, 4.0f, Settings.WindowHeight - 60.0f, 1.0f);

    textRenderer.FlushBatch(textShader, Settings.FontColor);

    // restore previous blending state
    glBlendFunc(srcAlphaFunc, dstAlphaFunc);
    if (!blendEnabled)
        glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void SetupMenu(GLFWwindow* window)
{
    Menu->Clear();

    if (!GameStarted)
    {
        // Initial State
        Menu->AddItem("START", [=]() {
            AudioEngine::GetInstance().PlayOneShotSound(Settings.GameStartSoundFile);
            GameStarted = true;
            Menu->Active = false;
            Scene->ToggleSounds(false);
            SetupMenu(window); // Re-setup immediately so next time it shows Resume
        });
        Menu->AddItem("QUIT", [=]() {
            glfwSetWindowShouldClose(window, true);
        });
    }
    else
    {
        // Game is paused
        Menu->AddItem("RESUME", [=]() {
            AudioEngine::GetInstance().PlayOneShotSound(Settings.MenuCloseSoundFile);
            Menu->Active = false;
            Scene->ToggleSounds(false);
        });
        Menu->AddItem("RESTART", [=]() {
            AudioEngine::GetInstance().PlayOneShotSound(Settings.GameStartSoundFile);
            Restart();
        });
        Menu->AddItem("QUIT", [=]() {
            glfwSetWindowShouldClose(window, true);
        });
    }
}

void Restart()
{
    std::cout << "Restart" << std::endl;
    Scene->Reset();
    Camera.Reset(Scene->GetStartingPosition());
    Player.Init(Camera);
    TorchLight.Direction = Camera.Front;

    Menu->Active = false;
    Menu->Reset();
    Scene->ToggleSounds(false);
}

void Shoot()
{
    std::cout << "Pew!" << std::endl;
}