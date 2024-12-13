#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "filesystem.hpp"
#include "level.hpp"
#include "shader.hpp"
#include "text_renderer.hpp"

void ProcessInput(GLFWwindow* window);
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void DisplayFPS(TextRenderer &textRenderer, Shader &textShader, std::string fps);

// settings
const unsigned int WindowWidth  = 800;
const unsigned int WindowHeight = 600;

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
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(WindowWidth), 0.0f, static_cast<float>(WindowHeight));
    textShader.Use();
    textShader.SetMat4("projection", projection);

    Shader defaultShader(FileSystem::GetPath("src/shaders/default.vs"), FileSystem::GetPath("src/shaders/default.fs"));
    Level level(FileSystem::GetPath("assets/level1.png"));
    glm::mat4 perspective = glm::perspective(glm::radians(80.0f), static_cast<GLfloat>(WindowWidth) / static_cast<GLfloat>(WindowHeight), 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(level.StartingPosition, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    defaultShader.Use();
    defaultShader.SetMat4("view", view);
    defaultShader.SetMat4("projection", perspective);
    
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
        ProcessInput(window);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // render FPS counter
        DisplayFPS(textRenderer, textShader, fps.str());

        // render the level
        level.Draw(defaultShader);

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
void ProcessInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void FramebufferSizeCallback(GLFWwindow* /* window */, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void DisplayFPS(TextRenderer &textRenderer, Shader &textShader, std::string fps)
{
    textRenderer.RenderText(textShader, fps, 2.0f, WindowHeight - 10.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
}