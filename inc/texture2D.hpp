#pragma once

#include <string>

#include <glad/glad.h>
#include <stb_image.h>

class Texture2D
{
public:
    GLuint ID;
    GLuint Width, Height;
    GLuint InternalFormat;
    GLuint ImageFormat;
    GLuint WrapS;
    GLuint WrapT;
    GLuint FilterMin;
    GLuint FilterMax;

    Texture2D(const std::string imagePath, GLboolean alpha, GLuint wrap, GLuint filterMin, GLuint filterMax)
    {
        glGenTextures(1, &ID);
        if (alpha)
        {
            InternalFormat = GL_RGBA;
            ImageFormat = GL_RGBA;
        }
        WrapS = wrap;
        WrapT = wrap;
        FilterMin = filterMin;
        FilterMax = filterMax;
        // Load image
        int width, height, channels;
        unsigned char* image = stbi_load(imagePath.c_str(), &width, &height, &channels, 0);
        // Now generate texture
        Generate(width, height, image);
        stbi_image_free(image);
    }

    void Generate(GLuint width, GLuint height, unsigned char* data)
    {
        Width = width;
        Height = height;
        // Create Texture
        glBindTexture(GL_TEXTURE_2D, ID);
        glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, width, height, 0, ImageFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        // Set Texture wrap and filter modes
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, WrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, WrapT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, FilterMin);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, FilterMax);
        // Unbind texture
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Bind() const
    {
        glBindTexture(GL_TEXTURE_2D, ID);
    }
};