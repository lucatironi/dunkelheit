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
            { { -0.5f, -0.5f, -0.5f }, { 0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f } }, // Bottom-left
            { {  0.5f, -0.5f, -0.5f }, { 0.0f,  0.0f, -1.0f }, { 1.0f, 0.0f } }, // Bottom-right
            { {  0.5f,  0.5f, -0.5f }, { 0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f } }, // Top-right
            { { -0.5f,  0.5f, -0.5f }, { 0.0f,  0.0f, -1.0f }, { 0.0f, 1.0f } }, // Top-left

            // Back face
            { { -0.5f, -0.5f,  0.5f }, { 0.0f,  0.0f,  1.0f }, { 1.0f, 0.0f } }, // Bottom-left
            { {  0.5f, -0.5f,  0.5f }, { 0.0f,  0.0f,  1.0f }, { 0.0f, 0.0f } }, // Bottom-right
            { {  0.5f,  0.5f,  0.5f }, { 0.0f,  0.0f,  1.0f }, { 0.0f, 1.0f } }, // Top-right
            { { -0.5f,  0.5f,  0.5f }, { 0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f } }  // Top-left
        };

        // Cube indices: 36 total, 6 faces × 2 triangles per face × 3 vertices per triangle
        std::vector<GLuint> indices = {
            // Front face
            1, 0, 3, 1, 3, 2,
            // Back face
            4, 5, 6, 4, 6, 7,
            // Left face
            0, 4, 7, 0, 7, 3,
            // Right face
            5, 1, 2, 5, 2, 6,
            // Top face
            7, 6, 2, 7, 2, 3,
            // Bottom face
            5, 4, 0, 5, 0, 1
        };

        std::vector<Texture> textures = {
            { Texture2D(FileSystem::GetPath("assets/texture_05.png"), false), "diffuse", "assets/texture_05.png" }
        };

        meshes.push_back(Mesh(vertices, indices, textures));
    }
};