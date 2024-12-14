#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "fps_camera.hpp"
#include "filesystem.hpp"
#include "level.hpp"
#include "shader.hpp"
#include "texture2D.hpp"
#include "text_renderer.hpp"

void ProcessInput(GLFWwindow* window, float deltaTime);
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void MouseCallback(GLFWwindow* window, double xposIn, double yposIn);

// settings
const unsigned int WindowWidth  = 800;
const unsigned int WindowHeight = 600;

FPSCamera camera(glm::vec3(0.0f, 0.0f, 0.0f));
float lastX = WindowWidth / 2.0f;
float lastY = WindowHeight / 2.0f;
bool firstMouse = true;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GL_FALSE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(WindowWidth, WindowHeight, "Dunkelheit", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
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
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // TextRenderer: compile and setup the shader
    // ----------------------------
    TextRenderer textRenderer(FileSystem::GetPath("assets/font.ttf"), 16);
    Shader textShader(FileSystem::GetPath("src/shaders/text.vs"), FileSystem::GetPath("src/shaders/text.fs"));
    glm::mat4 orthoProjection = glm::ortho(0.0f, static_cast<float>(WindowWidth), 0.0f, static_cast<float>(WindowHeight));
    textShader.Use();
    textShader.SetMat4("projection", orthoProjection);

    Shader defaultShader(FileSystem::GetPath("src/shaders/default.vs"), FileSystem::GetPath("src/shaders/default.fs"));
    Texture2D levelTexture(FileSystem::GetPath("assets/tiles.png"), GL_TRUE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
    Level level(FileSystem::GetPath("assets/level1.png"), levelTexture);
    camera.Position = level.StartingPosition;
    glm::mat4 perspectiveProjection = glm::perspective(glm::radians(80.0f), static_cast<GLfloat>(WindowWidth) / static_cast<GLfloat>(WindowHeight), 0.1f, 100.0f);
    defaultShader.Use();
    defaultShader.SetMat4("projection", perspectiveProjection);
    defaultShader.SetVec3("lightColor", glm::vec3(0.7f, 0.1f, 0.0f));
    defaultShader.SetFloat("constantAtt", 0.3f);
    defaultShader.SetFloat("linearAtt", 0.13f);
    defaultShader.SetFloat("quadraticAtt", 0.68f);


    // setup OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // render loop
    // -----------
    float currentFrame = 0.0f;
    float lastFrame    = 0.0f;
    float lastFPSFrame = 0.0f;
    float deltaTime    = 0.0f;
    int fpsCount = 0;
    std::stringstream fps;

    while (!glfwWindowShouldClose(window))
    {
        // calculate deltaTime and FPS
        currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        fpsCount++;
        if ((currentFrame - lastFPSFrame) >= 1.0f)
        {
            fps.str(std::string());
            fps << fpsCount;
            fpsCount = 0;
            lastFPSFrame = currentFrame;
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

        // render
        // ------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        defaultShader.Use();
        defaultShader.SetMat4("view", camera.GetViewMatrix());
        glm::vec3 torchPos = camera.Position + glm::normalize(glm::vec3(-0.5f, 1.0f, -0.75f)) * 0.5f;
        defaultShader.SetVec3("cameraPos", torchPos);

        // render the level
        level.Draw(defaultShader);

        // render Debug Information
        std::stringstream pos;
        pos << "x: " << (int)camera.Position.x << ", z: " << (int)camera.Position.z << ", tile: " << level.TileAt(camera.Position.x, camera.Position.z);
        textRenderer.RenderText(textShader, fps.str(), 2.0f, WindowHeight - 10.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
        textRenderer.RenderText(textShader, pos.str(), 2.0f, WindowHeight - 20.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        lastFrame = currentFrame;
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------

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

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}