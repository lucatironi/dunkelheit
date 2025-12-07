#pragma once

#include <glad/gl.h>

#include <iostream>
#include <vector>

class Pixelator
{
public:
    Pixelator(const GLuint lowResWidth, const GLuint lowResHeight,
              const GLuint screenWidth, const GLuint screenHeight)
        : lowResWidth(lowResWidth), lowResHeight(lowResHeight),
          screenWidth(screenWidth), screenHeight(screenHeight)
    {
        setupBuffers();
    }

    ~Pixelator()
    {
        glDeleteFramebuffers(1, &FBO);
        glDeleteRenderbuffers(1, &colorRBO);
        glDeleteRenderbuffers(1, &depthRBO);
    }

    void BeginRender()
    {
        // 1. Bind and clear the OFF-SCREEN, LOW-RESOLUTION FBO
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 2. Set viewport to the LOW-RESOLUTION size for rendering
        glViewport(0, 0, lowResWidth, lowResHeight);
    }

    void EndRender()
    {
        // 1. Set the blit source/destination buffers
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);   // Destination: default screen FBO
        glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO); // Source: off-screen low-res FBO
        glReadBuffer(GL_COLOR_ATTACHMENT0);

        // 2. Blit (Copy and Stretch)
        glBlitFramebuffer(
            // Source (Low-Res FBO)
            0, 0, lowResWidth, lowResHeight,

            // Destination (High-Res Screen)
            0, 0, screenWidth, screenHeight,

            GL_COLOR_BUFFER_BIT,
            GL_NEAREST); // GL_NEAREST ensures the blocky, pixelated look

        // 3. Unbind FBO and restore viewport
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Binds both READ and WRITE to default
        glViewport(0, 0, screenWidth, screenHeight);
    }

private:
    GLuint lowResWidth, lowResHeight;  // The size of the internal FBO (e.g., 512x288)
    GLuint screenWidth, screenHeight;  // The size of the screen's Framebuffer (e.g., 2048x1152)
    GLuint FBO, colorRBO, depthRBO;

    void setupBuffers()
    {
        // Initialize framebuffer/renderbuffers objects
        glGenFramebuffers(1, &FBO);
        glGenRenderbuffers(1, &colorRBO);
        glGenRenderbuffers(1, &depthRBO);

        glBindRenderbuffer(GL_RENDERBUFFER, colorRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, lowResWidth, lowResHeight);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::PIXELATOR: Failed to initialize color colorRBO" << std::endl;
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, lowResWidth, lowResHeight);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::PIXELATOR: Failed to initialize depth depthRBO" << std::endl;
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        // attach renderbuffer to framebuffer (at location = GL_COLOR_ATTACHMENT0)
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRBO);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);
        // specify the attachments in which to draw:
        std::vector<GLenum> drawbuffers = {
            GL_COLOR_ATTACHMENT0,
            GL_DEPTH_ATTACHMENT
        };
        glDrawBuffers(drawbuffers.size(), drawbuffers.data());
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::PIXELATOR: Failed to initialize FBO" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
};