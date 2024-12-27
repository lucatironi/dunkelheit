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

    void Draw() const
    {
        mesh->Draw();
    }

private:
    Mesh* mesh;

    void loadMesh()
    {
        // Positions
        glm::vec3 pA = { -0.5f, -0.5f, -0.5f };
        glm::vec3 pB = {  0.5f, -0.5f, -0.5f };
        glm::vec3 pC = {  0.5f,  0.5f, -0.5f };
        glm::vec3 pD = { -0.5f,  0.5f, -0.5f };
        glm::vec3 pE = { -0.5f, -0.5f,  0.5f };
        glm::vec3 pF = {  0.5f, -0.5f,  0.5f };
        glm::vec3 pG = {  0.5f,  0.5f,  0.5f };
        glm::vec3 pH = { -0.5f,  0.5f,  0.5f };

        // Normals
        glm::vec3 nF = {  0.0f,  0.0f, -1.0f }; // Front
        glm::vec3 nB = {  0.0f,  0.0f,  1.0f }; // Back
        glm::vec3 nL = { -1.0f,  0.0f,  0.0f }; // Left
        glm::vec3 nR = {  1.0f,  0.0f,  0.0f }; // Right
        glm::vec3 nU = {  0.0f,  1.0f,  0.0f }; // Up
        glm::vec3 nD = {  0.0f, -1.0f,  0.0f }; // Down

        // Texture Coordinates
        glm::vec2 tTL = { 0.0f, 0.0f }; // Top Left
        glm::vec2 tTR = { 1.0f, 0.0f }; // Top Right
        glm::vec2 tBL = { 0.0f, 1.0f }; // Bottom Left
        glm::vec2 tBR = { 1.0f, 1.0f }; // Bottom Right

        // Cube vertices
        std::vector<Vertex> vertices = {
            // Front face
            { pB, nF, tBL }, //  0 Bottom-left B
            { pA, nF, tBR }, //  1 Bottom-right A
            { pD, nF, tTR }, //  2 Top-right D
            { pC, nF, tTL }, //  3 Top-left C

            // Back face
            { pE, nB, tBL }, //  4 Bottom-left E
            { pF, nB, tBR }, //  5 Bottom-right F
            { pG, nB, tTR }, //  6 Top-right G
            { pH, nB, tTL }, //  7 Top-left H

            // Left face
            { pA, nL, tBL }, //  8 Bottom-left A
            { pE, nL, tBR }, //  9 Bottom-right E
            { pH, nL, tTR }, // 10 Top-right H
            { pD, nL, tTL }, // 11 Top-left D

            // Right face
            { pF, nR, tBL }, // 12 Bottom-left F
            { pB, nR, tBR }, // 13 Bottom-right B
            { pC, nR, tTR }, // 14 Top-right C
            { pG, nR, tTL }, // 15 Top-left G

            // Top face
            { pH, nU, tBL }, // 16 Bottom-left H
            { pG, nU, tBR }, // 17 Bottom-right G
            { pC, nU, tTR }, // 18 Top-right C
            { pD, nU, tTL }, // 19 Top-left D

            // Bottom face
            { pA, nD, tBL }, // 20 Bottom-left A
            { pB, nD, tBR }, // 21 Bottom-right B
            { pF, nD, tTR }, // 22 Top-right F
            { pE, nD, tTL }  // 23 Top-left E
        };

        // Cube indices: 36 total, 6 faces × 2 triangles per face × 3 vertices per triangle
        std::vector<GLuint> indices = {
            // Front face
            0, 1, 2, 2, 3, 0,
            // Back face
            4, 5, 6, 6, 7, 4,
            // Left face
            8, 9, 10, 10, 11, 8,
            // Right face
            12, 13, 14, 14, 15, 12,
            // Top face
            16, 17, 18, 18, 19, 16,
            // Bottom face
            20, 21, 22, 22, 23, 20
        };

        // Cube textures
        std::vector<Texture> textures = {
            { Texture2D(FileSystem::GetPath("assets/crate.png"), false), "texture_diffuse", "assets/crate.png" }
        };

        mesh = new Mesh(vertices, indices, textures);
    }
};