#pragma once

#include "mesh.hpp"
#include "shader.hpp"

#include <string>
#include <vector>

class CubeModel
{
public:
    CubeModel(const std::string& texturePath)
    {
        creatMesh(texturePath);
    }

    void Draw(const Shader& shader) const
    {
        mesh->Draw(shader);
    }

private:
    std::unique_ptr<Mesh> mesh;

    void creatMesh(const std::string& texturePath)
    {
        // Shared data for cube geometry
        const glm::vec3 positions[] = {
            { -0.5f, -0.5f, -0.5f }, {  0.5f, -0.5f, -0.5f },
            {  0.5f,  0.5f, -0.5f }, { -0.5f,  0.5f, -0.5f },
            { -0.5f, -0.5f,  0.5f }, {  0.5f, -0.5f,  0.5f },
            {  0.5f,  0.5f,  0.5f }, { -0.5f,  0.5f,  0.5f }
        };

        const glm::vec3 normals[] = {
            {  0.0f,  0.0f, -1.0f }, {  0.0f,  0.0f,  1.0f },
            { -1.0f,  0.0f,  0.0f }, {  1.0f,  0.0f,  0.0f },
            {  0.0f,  1.0f,  0.0f }, {  0.0f, -1.0f,  0.0f }
        };

        const glm::vec2 texCoords[] = {
            { 0.0f, 0.0f }, { 1.0f, 0.0f },
            { 1.0f, 1.0f }, { 0.0f, 1.0f }
        };

        // Vertex data for cube faces
        const int faceIndices[6][4] = {
            { 1, 0, 3, 2 }, // Front
            { 4, 5, 6, 7 }, // Back
            { 0, 4, 7, 3 }, // Left
            { 5, 1, 2, 6 }, // Right
            { 7, 6, 2, 3 }, // Top
            { 0, 1, 5, 4 }  // Bottom
        };
        std::vector<Vertex> vertices;
        vertices.reserve(24);
        for (int i = 0; i < 6; ++i)
        {
            const auto& face = faceIndices[i];
            vertices.push_back({ positions[face[0]], normals[i], texCoords[0] });
            vertices.push_back({ positions[face[1]], normals[i], texCoords[1] });
            vertices.push_back({ positions[face[2]], normals[i], texCoords[2] });
            vertices.push_back({ positions[face[3]], normals[i], texCoords[3] });
        }

        // Indices for cube faces
        std::vector<GLuint> indices;
        indices.reserve(36);
        for (GLuint i = 0; i < 6; ++i)
        {
            GLuint base = i * 4;
            indices.insert(indices.end(), { base, base + 1, base + 2, base + 2, base + 3, base });
        }

        // Texture setup
        std::vector<Texture> textures = {
            { Texture2D(texturePath), "texture_diffuse" }
        };

        // Create the mesh
        mesh = std::make_unique<Mesh>(vertices, indices, textures);
    }
};