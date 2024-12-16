#pragma once

#include <vector>

#include "file_system.hpp"
#include "mesh.hpp"
#include "shader.hpp"

class CubeModel
{
public:
    CubeModel()
    {
        loadMesh();
    }

    void Draw()
    {
        for (auto mesh : meshes)
        {
            std::cout << "Drawing mesh" << std::endl;
            mesh.Draw();
        }
    }

private:
    std::vector<Mesh> meshes;

    void loadMesh()
    {
        std::vector<Vertex> vertices = {
            // Front face
            { { -0.5f, -0.5f, -0.5f }, { 0.0f,  0.0f, -1.0f },{ 0.0f, 0.0f } }, // Bottom-left
            { {  0.5f, -0.5f, -0.5f }, { 0.0f,  0.0f, -1.0f },{ 1.0f, 0.0f } }, // Bottom-right
            { {  0.5f,  0.5f, -0.5f }, { 0.0f,  0.0f, -1.0f },{ 1.0f, 1.0f } }, // Top-right
            { { -0.5f,  0.5f, -0.5f }, { 0.0f,  0.0f, -1.0f },{ 0.0f, 1.0f } }, // Top-left

            // Back face
            { { -0.5f, -0.5f,  0.5f }, { 0.0f,  0.0f,  1.0f },{ 0.0f, 0.0f } }, // Bottom-left
            { {  0.5f, -0.5f,  0.5f }, { 0.0f,  0.0f,  1.0f },{ 1.0f, 0.0f } }, // Bottom-right
            { {  0.5f,  0.5f,  0.5f }, { 0.0f,  0.0f,  1.0f },{ 1.0f, 1.0f } }, // Top-right
            { { -0.5f,  0.5f,  0.5f }, { 0.0f,  0.0f,  1.0f },{ 0.0f, 1.0f } }  // Top-left
        };

        // Cube indices: 36 total, 6 faces × 2 triangles per face × 3 vertices per triangle
        std::vector<GLuint> indices = {
            // Front face
            0, 1, 2, 0, 2, 3,
            // Back face
            4, 5, 6, 4, 6, 7,
            // Left face
            4, 0, 3, 4, 3, 7,
            // Right face
            1, 5, 6, 1, 6, 2,
            // Top face
            3, 2, 6, 3, 6, 7,
            // Bottom face
            4, 5, 1, 4, 1, 0
        };

        std::vector<Texture> textures = {
            { Texture2D(FileSystem::GetPath("assets/texture_05.png"), false), "diffuse", "assets/texture_05.png" }
        };

        meshes.push_back(Mesh(vertices, indices, textures));
    }
};