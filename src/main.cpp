#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <irrKlang.h>

#include "file_system.hpp"
#include "footsteps_system.hpp"
#include "fps_camera.hpp"
#include "level.hpp"
#include "random_generator.hpp"
#include "shader.hpp"
#include "text_renderer.hpp"
#include "texture2D.hpp"
#include "weapon.hpp"

void ProcessInput(GLFWwindow* window, float deltaTime);
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void MouseCallback(GLFWwindow* window, double xposIn, double yposIn);

// settings
std::string WindowTitle = "Dunkelheit";
int WindowWidth  = 800;
int WindowHeight = 600;
int WindowPositionX = 0;
int WindowPositionY = 0;
bool FullScreen = true;

FPSCamera camera(glm::vec3(0.0f, 0.0f, 0.0f));
float LastX = WindowWidth / 2.0f;
float LastY = WindowHeight / 2.0f;
bool FirstMouse = true;

irrklang::ISoundEngine* SoundEngine;

int main()
{
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
    initRandom();

    // load TexRenderer
    TextRenderer textRenderer(FileSystem::GetPath("assets/font.ttf"), 16);
    Shader textShader(FileSystem::GetPath("shaders/text.vs"), FileSystem::GetPath("shaders/text.fs"));
    glm::mat4 orthoProjection = glm::ortho(0.0f, static_cast<float>(WindowWidth), 0.0f, static_cast<float>(WindowHeight));
    textShader.Use();
    textShader.SetMat4("projection", orthoProjection);

    GLfloat aspectRatio = static_cast<GLfloat>(WindowWidth) / static_cast<GLfloat>(WindowHeight);
    glm::mat4 perspectiveProjection = glm::perspective(glm::radians(80.0f), aspectRatio, 0.1f, 100.0f);

    // load Level
    Texture2D levelTexture(FileSystem::GetPath("assets/texture_05.png"), false);
    Level level(FileSystem::GetPath("assets/level1.png"), levelTexture);
    camera.Position = level.StartingPosition;

    Shader defaultShader(FileSystem::GetPath("shaders/default.vs"), FileSystem::GetPath("shaders/default.fs"));
    defaultShader.Use();
    level.SetLights(defaultShader);
    defaultShader.SetMat4("projection", perspectiveProjection);
    defaultShader.SetVec3("lightColor", glm::vec3(1.0f, 1.0f, 0.8f));
    defaultShader.SetFloat("lightRadius", 12.0f);

    // load Weapon
    Weapon weapon;

    // Initialize player state and footstep system
    PlayerState player = { camera.Position, camera.Position, false };
    FootstepSystem footsteps(SoundEngine);

    // setup OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // play ambient music
    SoundEngine->play2D(FileSystem::GetPath("assets/music.mp3").c_str(), true);

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
        // store current position for collision detection
        glm::vec3 previousPosition = camera.Position;
        ProcessInput(window, deltaTime);

        // update
        // ------
        // simple collision detection
        glm::vec3 checkPoint = camera.Position + camera.Front * 0.75f;
        int tile = level.TileAt(checkPoint.x, checkPoint.z);
        if (tile == 0 || tile == 128)
            camera.Position = previousPosition;

        weapon.Update(camera);

        player.position = camera.Position;
        footsteps.Update(currentTime, player);

        // render
        // ------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // render the level
        defaultShader.Use();
        defaultShader.SetVec3("cameraPos", camera.Position);
        defaultShader.SetFloat("time", currentTime);
        defaultShader.SetMat4("view", camera.GetViewMatrix());
        level.Draw(defaultShader);

        // render the weapon
        glClear(GL_DEPTH_BUFFER_BIT);
        weapon.Draw(defaultShader);

        // render Debug Information
        // Save current blending state
        GLboolean blendEnabled = glIsEnabled(GL_BLEND);
        GLint srcAlphaFunc, dstAlphaFunc;
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlphaFunc);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlphaFunc);

        // Enable alpha blending and set blend function
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        std::stringstream pos;
        pos << "x: " << (int)camera.Position.x << ", z: " << (int)camera.Position.z << ", tile: " << level.TileAt(camera.Position.x, camera.Position.z);
        textRenderer.RenderText(textShader, fps.str(), 4.0f, WindowHeight - 20.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
        textRenderer.RenderText(textShader, pos.str(), 4.0f, WindowHeight - 40.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));

        // // Restore previous blending state
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

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessInputMovement(CAMERA_FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessInputMovement(CAMERA_BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessInputMovement(CAMERA_LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessInputMovement(CAMERA_RIGHT, deltaTime);
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

    camera.ProcessMouseMovement(xoffset, yoffset);
}