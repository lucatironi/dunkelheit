#pragma once

#include <string>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "shader.hpp"
#include "texture2D.hpp"

class Level
{
public:
    glm::vec3 StartingPosition;

    Level(const std::string levelPath, Texture2D texture)
        : texture(texture)
    {
        loadLevel(levelPath);
        initRenderData();
    }
    ~Level()
    {
        stbi_image_free(levelData);
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

    void Draw(Shader shader)
    {
        shader.Use();
        shader.SetMat4("model", glm::mat4(1.0f));
        
        glActiveTexture(GL_TEXTURE0);
        texture.Bind();

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size());
        glBindVertexArray(0);
    }

private:
    const float tileFraction = 16.0f / 1024.0f;
    const float quadSize = 1.0f;

    int levelWidth, levelDepth;
    unsigned char *levelData;
    GLuint VAO, VBO;
    Texture2D texture;
    std::vector<GLfloat> vertices;

    void loadLevel(const std::string levelPath)
    {
        // Load level data from image
        int channels;
        levelData = stbi_load(levelPath.c_str(), &levelWidth, &levelDepth, &channels, 1);
        for (int z = 0; z < levelDepth; z++)
        {
            for (int x = 0; x < levelWidth; x++)
            {
                int colorKey = levelData[levelWidth * z + x];
                switch (colorKey)
                {
                case 255: // white, floor
                    addFloor(x * quadSize, z * quadSize);
                    break;
                case 149: // green, player
                    StartingPosition = glm::vec3(x, 1.0f, z);
                    addFloor(x * quadSize, z * quadSize);
                    break;
                case 128: // grey, wall
                    addWall(x * quadSize, z * quadSize);
                    break;
                case 76: // red, enemy spawner
                    addFloor(x * quadSize, z * quadSize);
                    break;
                case 28: // blue, light
                    addFloor(x * quadSize, z * quadSize);
                    break;
                default:
                    break;
                }
            }
        }
    }

    void initRenderData()
    {
        // Configure VAO/VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void *)0);
        glEnableVertexAttribArray(0);
        // normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        // texture coord attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void *)(6 * sizeof(GLfloat)));
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void addFloor(GLfloat x, GLfloat z)
    {
        pushQuad(x, 0.0f, z,
                 x + quadSize, 0.0f, z,
                 x, 0.0f, z + quadSize,
                 x + quadSize, 0.0f, z + quadSize,
                 0.0f, 1.0f, 0.0f,
                 0);
    }

    void addWall(GLfloat x, GLfloat z)
    {

    }

    void pushQuad(GLfloat x1, GLfloat y1, GLfloat z1,
                  GLfloat x2, GLfloat y2, GLfloat z2,
                  GLfloat x3, GLfloat y3, GLfloat z3,
                  GLfloat x4, GLfloat y4, GLfloat z4,
                  GLfloat nx, GLfloat ny, GLfloat nz,
                  GLuint tile)
    {
        GLfloat u = tile * tileFraction;

        std::vector<GLfloat> newVertices = {
            x3, y3, z3, nx, ny, nz, u, 1.0f,
            x2, y2, z2, nx, ny, nz, u + tileFraction, 0.0f,
            x1, y1, z1, nx, ny, nz, u, 0.0f,
            x2, y2, z2, nx, ny, nz, u + tileFraction, 0.0f,
            x3, y3, z3, nx, ny, nz, u, 1.0f,
            x4, y4, z4, nx, ny, nz, u + tileFraction, 1.0f};
        vertices.insert(end(vertices), begin(newVertices), end(newVertices));
    }
};
