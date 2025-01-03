#include <iostream>
#include <random>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <irrKlang.h>

#include "file_system.hpp"
#include "footsteps_system.hpp"
#include "fps_camera.hpp"
#include "glm/matrix.hpp"
#include "level.hpp"
#include "object.hpp"
#include "quad.hpp"
#include "random_generator.hpp"
#include "shader.hpp"
#include "text_renderer.hpp"
#include "texture2D.hpp"
#include "weapon.hpp"

struct GBuffer
{
    GLuint FBO;
    GLuint gPosition;
    GLuint gNormal;
    GLuint gAlbedo;
};

struct SSAO
{
    GLuint FBO;
    GLuint BlurFBO;
    GLuint ColorBuffer;
    GLuint ColorBufferBlur;
};

void ProcessInput(GLFWwindow* window, float deltaTime);
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void MouseCallback(GLFWwindow* window, double xposIn, double yposIn);

GBuffer setupGBuffer(int width, int height);
void setupDeferredShaders(Shader& shaderGeometryPass, Shader& shaderLightingPass, glm::mat4 projection);
void setupForwardShaders(Shader& shaderSinglePass, glm::mat4 projection);

// settings
std::string WindowTitle = "Dunkelheit";
int WindowWidth  = 800;
int WindowHeight = 600;
int WindowPositionX = 0;
int WindowPositionY = 0;
bool FullScreen = true;
bool UseDeferredShading = true;

FPSCamera Camera(glm::vec3(0.0f, 0.0f, 0.0f));
float LastX = WindowWidth / 2.0f;
float LastY = WindowHeight / 2.0f;
bool FirstMouse = true;

glm::vec3 LightColor = { 1.0f, 1.0f, 0.8f };
float LightRadius = 5.0;
float AmbientLight = 0.01f;
float SpecularShininess = 4.0f;
float SpecularIntensity = 0.1f;

irrklang::ISoundEngine* SoundEngine;

float ourLerp(float a, float b, float f)
{
    return a + f * (b - a);
}

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

    // load Level
    Texture2D levelTexture(FileSystem::GetPath("assets/level_textures_02.png"), true);
    Level level(FileSystem::GetPath("assets/level1.png"), levelTexture);
    Camera.Position = level.StartingPosition;

    // load Weapons
    Weapon leftWeapon(FileSystem::GetPath("assets/left_hand.glb"), FileSystem::GetPath("assets/base_texture.png"),
        glm::vec3(-1.0f, -1.2f, 1.2f), glm::vec3(-60.0f, 30.0f, 0.0f), glm::vec3(1.0f));
    Weapon rightWeapon(FileSystem::GetPath("assets/blasterI.glb"), FileSystem::GetPath("assets/base_texture.png"),
        glm::vec3(1.4f, -1.0f, 1.8f), glm::vec3(5.0f, 185.0f, 0.0f), glm::vec3(2.0f));

    // load test cube
    Object testCube(glm::vec3(42.0f, 0.5f, 167.0f));

    // Initialize player state and footstep system
    PlayerState player = { Camera.Position, Camera.Position, false };
    FootstepSystem footsteps(SoundEngine);

    GLfloat aspectRatio = static_cast<GLfloat>(WindowWidth) / static_cast<GLfloat>(WindowHeight);
    glm::mat4 perspectiveProjection = glm::perspective(glm::radians(80.0f), aspectRatio, 0.1f, 100.0f);

    // deferred shading setup
    Quad quad;
    GBuffer gBuffer = setupGBuffer(WindowWidth, WindowHeight);
    Shader shaderGeometryPass(FileSystem::GetPath("shaders/geometry_pass.vs"), FileSystem::GetPath("shaders/geometry_pass.fs"));
    Shader shaderLightingPass(FileSystem::GetPath("shaders/render_to_quad.vs"), FileSystem::GetPath("shaders/lighting_pass.fs"));
    setupDeferredShaders(shaderGeometryPass, shaderLightingPass, perspectiveProjection);
    level.SetLights(shaderLightingPass);

    // forward shading setup
    Shader defaultShader(FileSystem::GetPath("shaders/default.vs"), FileSystem::GetPath("shaders/default.fs"));
    setupForwardShaders(defaultShader, perspectiveProjection);
    level.SetLights(defaultShader);

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
        glm::vec3 previousPosition = Camera.Position;
        ProcessInput(window, deltaTime);

        // update
        // ------
        // simple collision detection
        glm::vec3 checkPoint = Camera.Position + Camera.Front * 0.75f;
        int tile = level.TileAt(checkPoint.x, checkPoint.z);
        if (tile == 0 || tile == 128)
            Camera.Position = previousPosition;

        leftWeapon.Update(Camera);
        rightWeapon.Update(Camera);

        player.position = Camera.Position;
        footsteps.Update(currentTime, player);

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
                testCube.Draw(shaderGeometryPass);
                glClear(GL_DEPTH_BUFFER_BIT);
                leftWeapon.Draw(shaderGeometryPass);
                rightWeapon.Draw(shaderGeometryPass);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // 2. lighting pass: traditional deferred Blinn-Phong lighting
            shaderLightingPass.Use();
            shaderLightingPass.SetVec3("cameraPos", Camera.Position);
            shaderLightingPass.SetFloat("time", currentTime);
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
            defaultShader.SetFloat("time", currentTime);
            level.Draw(defaultShader);
            testCube.Draw(defaultShader);
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
        textRenderer.RenderText(textShader, fpsText.str(), 4.0f, WindowHeight - 20.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
        std::stringstream windowSize;
        windowSize << WindowWidth << "x" << WindowHeight;
        textRenderer.RenderText(textShader, windowSize.str(), 4.0f, WindowHeight - 40.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
        std::string shadingMode = UseDeferredShading ? "Deferred" : "Forward";
        textRenderer.RenderText(textShader, shadingMode, 4.0f, WindowHeight - 60.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
        std::stringstream pos;
        pos << "pos x: " << (int)Camera.Position.x << ", z: " << (int)Camera.Position.z << ", tile: " << level.TileAt(Camera.Position.x, Camera.Position.z);
        textRenderer.RenderText(textShader, pos.str(), 4.0f, WindowHeight - 80.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));

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

    if (!UseDeferredShading && glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        UseDeferredShading = true;

    if (UseDeferredShading && glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        UseDeferredShading = false;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        Camera.ProcessInputMovement(CAMERA_FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        Camera.ProcessInputMovement(CAMERA_BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        Camera.ProcessInputMovement(CAMERA_LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        Camera.ProcessInputMovement(CAMERA_RIGHT, deltaTime);
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

GBuffer setupGBuffer(int width, int height)
{
    GBuffer gBuffer;

    glGenFramebuffers(1, &gBuffer.FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.FBO);

    // position color buffer
    glGenTextures(1, &gBuffer.gPosition);
    glBindTexture(GL_TEXTURE_2D, gBuffer.gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gBuffer.gPosition, 0);

    // normal color buffer
    glGenTextures(1, &gBuffer.gNormal);
    glBindTexture(GL_TEXTURE_2D, gBuffer.gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gBuffer.gNormal, 0);

    // albedo color buffer
    glGenTextures(1, &gBuffer.gAlbedo);
    glBindTexture(GL_TEXTURE_2D, gBuffer.gAlbedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
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

void setupDeferredShaders(Shader& shaderGeometryPass, Shader& shaderLightingPass, glm::mat4 projection)
{
    shaderGeometryPass.Use();
    shaderGeometryPass.SetMat4("projection", projection);

    shaderLightingPass.Use();
    shaderLightingPass.SetInt("gPosition", 0);
    shaderLightingPass.SetInt("gNormal", 1);
    shaderLightingPass.SetInt("gAlbedo", 2);
    shaderLightingPass.SetFloat("ambient", AmbientLight);
    shaderLightingPass.SetFloat("specularShininess", SpecularShininess);
    shaderLightingPass.SetFloat("specularIntensity", SpecularIntensity);
    shaderLightingPass.SetFloat("lightRadius", LightRadius);
    shaderLightingPass.SetVec3("lightColor", LightColor);
}

void setupForwardShaders(Shader& shaderSinglePass, glm::mat4 projection)
{
    shaderSinglePass.Use();
    shaderSinglePass.SetMat4("projection", projection);
    shaderSinglePass.SetFloat("ambient", AmbientLight);
    shaderSinglePass.SetFloat("specularShininess", SpecularShininess);
    shaderSinglePass.SetFloat("specularIntensity", SpecularIntensity);
    shaderSinglePass.SetFloat("lightRadius", LightRadius);
    shaderSinglePass.SetVec3("lightColor", LightColor);
}