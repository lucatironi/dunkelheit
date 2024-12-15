#pragma once

#include <string>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "fps_camera.hpp"
#include "random_generator.hpp"
#include "shader.hpp"
#include "texture2D.hpp"

struct Light
{
    glm::vec3 position;
    glm::vec3 color;
};

enum ColorKey {
    COLOR_FLOOR  = 255,
    COLOR_PLAYER = 149,
    COLOR_WALL   = 128,
    COLOR_ENEMY  = 76,
    COLOR_LIGHT  = 28
};

constexpr float DEFAULT_TILE_FRACTION = 16.0f / 1024.0f;
constexpr float DEFAULT_QUAD_SIZE = 3.0f;
constexpr size_t MAX_LIGHTS = 16;

class Level
{
public:
    glm::vec3 StartingPosition;

    Level(const std::string levelPath, Texture2D texture)
        : texture(texture)
    {
        initRandom();
        loadLevel(levelPath);
        initRenderData();
    }

    ~Level()
    {
        stbi_image_free(levelData);
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

    void Draw(const Shader& shader)
    {
        shader.Use();
        shader.SetMat4("model", glm::mat4(1.0f));

        // Upload light data
        for (size_t i = 0; i < lights.size(); ++i)
        {
            shader.SetVec3("lights[" + std::to_string(i) + "].position", lights[i].position);
            shader.SetVec3("lights[" + std::to_string(i) + "].color", lights[i].color);
        }

        glActiveTexture(GL_TEXTURE0);
        texture.Bind();

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 8));
        glBindVertexArray(0);
    }

    int TileAt(float x, float z) const
    {
        if (x < 0 || z < 0 || x >= levelWidth * quadSize || z >= levelDepth * quadSize)
            return -1; // Return invalid tile for out-of-bounds
        int i = static_cast<int>(x / quadSize);
        int j = static_cast<int>(z / quadSize);
        return levelData[levelWidth * j + i];
    }

    void AddLight(const glm::vec3& position, const glm::vec3& color)
    {
        if (lights.size() < MAX_LIGHTS)
            lights.push_back({position, color});
        else
            std::cerr << "Warning: Maximum number of lights exceeded!" << std::endl;
    }

    int NumLights() const
    {
        return (int)lights.size();
    }

private:
    const float tileFraction = DEFAULT_TILE_FRACTION;
    const float quadSize = DEFAULT_QUAD_SIZE;

    int levelWidth, levelDepth;
    unsigned char *levelData;
    GLuint VAO, VBO;
    Texture2D texture;
    std::vector<GLfloat> vertices;
    std::vector<Light> lights;

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

    void addFloor(int x, int z)
    {
        pushQuad({x * quadSize, 0.0f, z * quadSize},
                 {(x + 1) * quadSize, 0.0f, z * quadSize},
                 {x * quadSize, 0.0f, (z + 1) * quadSize},
                 {(x + 1) * quadSize, 0.0f, (z + 1) * quadSize},
                 {0.0f, 1.0f, 0.0f}, getRandomInRange(0, 2)); // Upward normal
    }

    void addCeiling(int x, int z)
    {
         pushQuad(
                  {(x + 1) * quadSize, quadSize, z * quadSize},
                  {x * quadSize, quadSize, z * quadSize},
                  {(x + 1) * quadSize, quadSize, (z + 1) * quadSize},
                  {x * quadSize, quadSize, (z + 1) * quadSize},
                  {0.0f, -1.0f, 0.0f}, getRandomInRange(4, 6)); // Downward normal
    }

    void addWall(int x, int z)
    {
       // Determine if the neighboring tiles should be considered for wall generation
        bool hasFloorRight = (x + 1 < levelWidth) && (levelData[levelWidth * z + (x + 1)] == COLOR_FLOOR);
        bool hasFloorLeft  = (x - 1 >= 0) && (levelData[levelWidth * z + (x - 1)] == COLOR_FLOOR);
        bool hasFloorUp    = (z - 1 >= 0) && (levelData[levelWidth * (z - 1) + x] == COLOR_FLOOR);
        bool hasFloorDown  = (z + 1 < levelDepth) && (levelData[levelWidth * (z + 1) + x] == COLOR_FLOOR);

        // Left wall
        if (hasFloorRight)
        {
            pushQuad({(x + 1) * quadSize, quadSize, (z + 1) * quadSize},
                     {(x + 1) * quadSize, quadSize, z * quadSize},
                     {(x + 1) * quadSize, 0.0f, (z + 1) * quadSize},
                     {(x + 1) * quadSize, 0.0f, z * quadSize},
                     {1.0f, 0.0f, 0.0f}, getRandomInRange(7, 16)); // Rightward normal
        }
        // Right wall
        if (hasFloorLeft)
        {
            pushQuad({x * quadSize, quadSize, z * quadSize},
                     {x * quadSize, quadSize, (z + 1) * quadSize},
                     {x * quadSize, 0.0f, z * quadSize},
                     {x * quadSize, 0.0f, (z + 1) * quadSize},
                     {-1.0f, 0.0f, 0.0f}, getRandomInRange(7, 16));  // Leftward normal
        }
        // Forward wall
        if (hasFloorDown)
        {
            pushQuad({x * quadSize, quadSize, (z + 1) * quadSize},
                     {(x + 1) * quadSize, quadSize, (z + 1) * quadSize},
                     {x * quadSize, 0.0f, (z + 1) * quadSize},
                     {(x + 1) * quadSize, 0.0f, (z + 1) * quadSize},
                     {0.0f, 0.0f, 1.0f}, getRandomInRange(7, 16)); // Backward normal
        }
        // Backward wall
        if (hasFloorUp)
        {
            pushQuad({(x + 1) * quadSize, quadSize, z * quadSize},
                     {x * quadSize, quadSize, z * quadSize},
                     {(x + 1) * quadSize, 0.0f, z * quadSize},
                     {x * quadSize, 0.0f, z * quadSize},
                     {0.0f, 0.0f, -1.0f}, getRandomInRange(7, 16)); // Forward normal
        }
    }

    void loadLevel(const std::string& levelPath)
    {
        // Load level data from the image
        int channels;
        levelData = stbi_load(levelPath.c_str(), &levelWidth, &levelDepth, &channels, 1);
        if (!levelData)
        {
            throw std::runtime_error("Failed to load level: " + levelPath);
        }

        // Process each tile
        for (int z = 0; z < levelDepth; ++z)
        {
            for (int x = 0; x < levelWidth; ++x)
            {
                int colorKey = levelData[levelWidth * z + x];
                handleTile(colorKey, x, z);
            }
        }
    }

    void handleTile(int colorKey, int x, int z)
    {
        glm::vec3 position = glm::vec3(x * quadSize, 0.0f, z * quadSize);

        switch (colorKey)
        {
        case COLOR_FLOOR:
            addFloor(x, z);
            addCeiling(x, z);
            break;
        case COLOR_PLAYER:
            StartingPosition = glm::vec3(position.x, FPSCamera::DEFAULT_HEAD_HEIGHT, position.z);
            addFloor(x, z);
            addCeiling(x, z);
            break;
        case COLOR_WALL:
            addWall(x, z);
            break;
        case COLOR_ENEMY:
            addFloor(x, z);
            addCeiling(x, z);
            break;
        case COLOR_LIGHT:
            AddLight(position + glm::vec3(0.0f, quadSize / 2.0f, 0.0f), glm::vec3(0.0f, 0.1f, 0.7f));
            addFloor(x, z);
            addCeiling(x, z);
            break;
        default:
            break;
        }
    }

    void pushQuad(const glm::vec3& v1, const glm::vec3& v2,
                  const glm::vec3& v3, const glm::vec3& v4,
                  const glm::vec3& normal, GLuint tile)
    {
        float u = tile * tileFraction;
        float u1 = u + tileFraction;

        std::vector<GLfloat> newVertices = {
            // Vertex positions, normals, and texture coordinates
            v3.x, v3.y, v3.z, normal.x, normal.y, normal.z, u,  1.0f,
            v2.x, v2.y, v2.z, normal.x, normal.y, normal.z, u1, 0.0f,
            v1.x, v1.y, v1.z, normal.x, normal.y, normal.z, u,  0.0f,
            v2.x, v2.y, v2.z, normal.x, normal.y, normal.z, u1, 0.0f,
            v3.x, v3.y, v3.z, normal.x, normal.y, normal.z, u,  1.0f,
            v4.x, v4.y, v4.z, normal.x, normal.y, normal.z, u1, 1.0f
        };

        vertices.insert(vertices.end(), newVertices.begin(), newVertices.end());
    }
};
