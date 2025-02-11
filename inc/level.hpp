#pragma once

#include "entity.hpp"
#include "random_generator.hpp"
#include "shader.hpp"
#include "texture_2D.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>

struct Light
{
    glm::vec3 position;
    glm::vec3 color;
};

enum TileKey
{
    COLOR_FLOOR  = 255,
    COLOR_PLAYER = 149,
    COLOR_WALL   = 128,
    COLOR_ENEMY  = 76,
    COLOR_LIGHT  = 28,
    COLOR_EMPTY  = 0
};

struct AABB
{
    glm::vec3 min;
    glm::vec3 max;
};

struct Tile
{
    int key;
    AABB aabb;
};

constexpr float DEFAULT_TILE_FRACTION = 128.0f / 512.0f; // tile size / tilemap size
constexpr float DEFAULT_TILE_SIZE = 3.0f;
constexpr glm::vec3 DEFAULT_LIGHT_COLOR = glm::vec3(0.0f, 0.1f, 0.7f);
constexpr size_t MAX_LIGHTS = 32;

class Level : public Entity
{
public:
    glm::vec3 StartingPosition;

    Level(const std::string& mapPath, Texture2D texture)
        : texture(texture)
    {
        loadLevel(mapPath);
        setupBuffers();
    }

    ~Level()
    {
        stbi_image_free(levelData);
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
        if (VBO != 0) glDeleteBuffers(1, &VBO);
    }

    void Draw(const Shader& shader) const override
    {
        shader.Use();
        shader.SetMat4("model", glm::mat4(1.0f));

        glActiveTexture(GL_TEXTURE0);
        texture.Bind();

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 8));
        glBindVertexArray(0);
    }

    Tile& GetTile(const glm::vec3& position)
    {
        if (position.x < 0 || position.x >= levelWidth * quadSize ||
            position.z < 0 || position.z >= levelDepth * quadSize)
            return nullTile; // Return invalid tile for out-of-bounds
        int ix = static_cast<int>(position.x / quadSize);
        int iz = static_cast<int>(position.z / quadSize);
        return tiles[iz * levelWidth + ix];
    }

    std::vector<Tile> GetNeighboringTiles(const glm::vec3& position) const
    {
        std::vector<Tile> neighbors;
        int ix = static_cast<int>(position.x / quadSize);
        int iz = static_cast<int>(position.z / quadSize);
        for (int dx = -1; dx <= 1; ++dx)
        {
            for (int dz = -1; dz <= 1; ++dz)
            {
                if (dx == 0 && dz == 0)
                    continue; // skip the center tile
                if (ix + dx < 0 || ix + dx >= levelWidth ||
                    iz + dz < 0 || iz + dz >= levelDepth)
                    continue; // skip out-of-bounds tiles
                neighbors.emplace_back(tiles[(iz + dz) * levelWidth + (ix + dx)]);
            }
        }
        return neighbors;
    }

    void AddLight(const glm::vec3& position, const glm::vec3& color)
    {
        if (lights.size() < MAX_LIGHTS)
            lights.push_back({ position, color });
        else
            std::cerr << "Warning: Maximum number of lights exceeded!" << std::endl;
    }

    int NumLights() const
    {
        return static_cast<int>(lights.size());
    }

    void SetLights(const Shader& shader)
    {
        shader.Use();
        for (size_t i = 0; i < NumLights(); ++i)
        {
            shader.SetVec3("lights[" + std::to_string(i) + "].position", lights[i].position);
            shader.SetVec3("lights[" + std::to_string(i) + "].color", lights[i].color);
        }
        shader.SetInt("numLights", NumLights());
    }

private:
    const float tileFraction = DEFAULT_TILE_FRACTION;
    const float quadSize = DEFAULT_TILE_SIZE;

    int levelWidth, levelDepth;
    unsigned char* levelData;
    GLuint VAO, VBO;
    Texture2D texture;
    std::vector<Tile> tiles;
    std::vector<GLfloat> vertices;
    std::vector<Light> lights;
    Tile nullTile;

    RandomGenerator& random = RandomGenerator::GetInstance();

    void setupBuffers()
    {
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

    void addBlock(int x, int z)
    {
        // Floor
        pushQuad({ x * quadSize, 0.0f, (z + 1) * quadSize },       // 0, 0, 1 E
                 { (x + 1) * quadSize, 0.0f, (z + 1) * quadSize }, // 1, 0, 1 F
                 { (x + 1) * quadSize, 0.0f, z * quadSize },       // 1, 0, 0 B
                 { x * quadSize, 0.0f, z * quadSize },             // 0, 0, 0 A
                 { 0.0f, 1.0f, 0.0f }, random.GetWeightedRandomInRange(0, 3)); // Upward normal
        // Ceiling
        pushQuad({ x * quadSize, quadSize, z * quadSize },             // 0, 1, 0 D
                 { (x + 1) * quadSize, quadSize, z * quadSize },       // 1, 1, 0 C
                 { (x + 1) * quadSize, quadSize, (z + 1) * quadSize }, // 1, 1, 1 G
                 { x * quadSize, quadSize, (z + 1) * quadSize },       // 0, 1, 1 H
                 { 0.0f, -1.0f, 0.0f }, random.GetWeightedRandomInRange(4, 7)); // Downward normal
    }

    void addWall(int x, int z)
    {
        // Positions
        glm::vec3 pA = { x * quadSize, 0.0f, z * quadSize };                 // 0, 0, 0 A
        glm::vec3 pB = { (x + 1) * quadSize, 0.0f, z * quadSize };           // 1, 0, 0 B
        glm::vec3 pC = { (x + 1) * quadSize, quadSize, z * quadSize };       // 1, 1, 0 C
        glm::vec3 pD = { x * quadSize, quadSize, z * quadSize };             // 0, 1, 0 D
        glm::vec3 pE = { x * quadSize, 0.0f, (z + 1) * quadSize };           // 0, 0, 1 E
        glm::vec3 pF = { (x + 1) * quadSize, 0.0f, (z + 1) * quadSize };     // 1, 0, 1 F
        glm::vec3 pG = { (x + 1) * quadSize, quadSize, (z + 1) * quadSize }; // 1, 1, 1 G
        glm::vec3 pH = { x * quadSize, quadSize, (z + 1) * quadSize };       // 0, 1, 1 H

        // Normals
        glm::vec3 nF = {  0.0f,  0.0f, -1.0f }; // Front
        glm::vec3 nB = {  0.0f,  0.0f,  1.0f }; // Back
        glm::vec3 nL = { -1.0f,  0.0f,  0.0f }; // Left
        glm::vec3 nR = {  1.0f,  0.0f,  0.0f }; // Right
        glm::vec3 nU = {  0.0f,  1.0f,  0.0f }; // Up
        glm::vec3 nD = {  0.0f, -1.0f,  0.0f }; // Down

        // Determine if the neighboring tiles should be considered for wall generation
        bool hasFloorFront = (z - 1 >= 0) && (levelData[(z - 1) * levelWidth + x] == COLOR_FLOOR);
        bool hasFloorBack  = (z + 1 < levelDepth) && (levelData[(z + 1) * levelWidth + x] == COLOR_FLOOR);
        bool hasFloorLeft  = (x - 1 >= 0) && (levelData[z  * levelWidth + (x - 1)] == COLOR_FLOOR);
        bool hasFloorRight = (x + 1 < levelWidth) && (levelData[z  * levelWidth + (x + 1)] == COLOR_FLOOR);

        // Backward wall
        if (hasFloorFront)
            pushQuad(pB, pA, pD, pC, nF, random.GetWeightedRandomInRange(8, 11));
        // Forward wall
        if (hasFloorBack)
            pushQuad(pE, pF, pG, pH, nB, random.GetWeightedRandomInRange(8, 11));
        // Right wall
        if (hasFloorLeft)
            pushQuad(pA, pE, pH, pD, nL, random.GetWeightedRandomInRange(8, 11));
        // Left wall
        if (hasFloorRight)
            pushQuad(pF, pB, pC, pG, nR, random.GetWeightedRandomInRange(8, 11));
    }

    void loadLevel(const std::string& path)
    {
        // Load level data from the image
        int channels;
        levelData = stbi_load(path.c_str(), &levelWidth, &levelDepth, &channels, 1);
        if (!levelData)
        {
            throw std::runtime_error("Failed to load level: " + path);
        }

        tiles.resize(levelWidth * levelDepth);

        // Process each tile
        for (int z = 0; z < levelDepth; ++z)
        {
            for (int x = 0; x < levelWidth; ++x)
            {
                Tile tile;
                tile.key = levelData[z * levelWidth + x];
                tile.aabb = { { x * quadSize, 0.0f, z * quadSize },                   // min
                              { (x + 1) * quadSize, quadSize, (z + 1) * quadSize } }; // max
                tiles[z * levelWidth + x] = tile;
                handleTile(tile.key, x, z);
            }
        }
    }

    void handleTile(int tileKey, int x, int z)
    {
        glm::vec3 position = glm::vec3(x * quadSize, 0.0f, z * quadSize);

        switch (tileKey)
        {
        case COLOR_FLOOR:
            addBlock(x, z);
            break;
        case COLOR_PLAYER:
            StartingPosition = position + (quadSize / 2.0f);
            addBlock(x, z);
            break;
        case COLOR_WALL:
            addWall(x, z);
            break;
        case COLOR_ENEMY:
            addBlock(x, z);
            break;
        case COLOR_LIGHT:
            AddLight(position + (quadSize / 2.0f), DEFAULT_LIGHT_COLOR);
            addBlock(x, z);
            break;
        default:
            break;
        }
    }

    void pushQuad(const glm::vec3& ver0, const glm::vec3& ver1,
                  const glm::vec3& ver2, const glm::vec3& ver3,
                  const glm::vec3& normal, const int tile)
    {
        int row = tile / 4;    // Compute the row (0-3)
        int column = tile % 4; // Compute the column (0-3)

        float u0 = column * tileFraction; // Left edge of the tile
        float v0 = row * tileFraction;    // Bottom edge of the tile
        float u1 = u0 + tileFraction;     // Right edge of the tile
        float v1 = v0 + tileFraction;     // Top edge of the tile

        std::vector<GLfloat> newVertices = {
            // Vertex positions, normals, and texture coordinates
            ver0.x, ver0.y, ver0.z, normal.x, normal.y, normal.z, u0, v1, // 0, 1
            ver1.x, ver1.y, ver1.z, normal.x, normal.y, normal.z, u1, v1, // 1, 1
            ver2.x, ver2.y, ver2.z, normal.x, normal.y, normal.z, u1, v0, // 1, 0
            ver2.x, ver2.y, ver2.z, normal.x, normal.y, normal.z, u1, v0, // 1, 0
            ver3.x, ver3.y, ver3.z, normal.x, normal.y, normal.z, u0, v0, // 0, 0
            ver0.x, ver0.y, ver0.z, normal.x, normal.y, normal.z, u0, v1  // 0, 1
        };

        vertices.insert(vertices.end(), newVertices.begin(), newVertices.end());
    }
};
