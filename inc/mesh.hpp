#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

class Mesh {
public:

    // Constructor
    Mesh() : VAO(0), VBO(0), EBO(0) {
        setupBuffers();
    }

    // Destructor: Clean up the buffers
    ~Mesh() {
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
        if (VBO != 0) glDeleteBuffers(1, &VBO);
        if (EBO != 0) glDeleteBuffers(1, &EBO);
    }

    // Draw the mesh
    void Draw() {
        glBindVertexArray(VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

private:
    GLuint VAO, VBO, EBO;  // Vertex Array Object, Vertex Buffer Object, Element Buffer Object
    std::vector<glm::vec3> vertices;
    std::vector<GLuint> indices;

    void setupBuffers() {
        // Define the vertices for a cube (6 faces, 2 triangles per face, 3 vertices per triangle)
        vertices = {
            // Positions
            glm::vec3(-0.5f, -0.5f, -0.5f),  // Front-bottom-left
            glm::vec3( 0.5f, -0.5f, -0.5f),  // Front-bottom-right
            glm::vec3( 0.5f,  0.5f, -0.5f),  // Front-top-right
            glm::vec3(-0.5f,  0.5f, -0.5f),  // Front-top-left

            glm::vec3(-0.5f, -0.5f,  0.5f),  // Back-bottom-left
            glm::vec3( 0.5f, -0.5f,  0.5f),  // Back-bottom-right
            glm::vec3( 0.5f,  0.5f,  0.5f),  // Back-top-right
            glm::vec3(-0.5f,  0.5f,  0.5f)   // Back-top-left
        };

        // Define the indices for the cube (two triangles per face)
        indices = {
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

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        // Bind VAO
        glBindVertexArray(VAO);

        // Bind and set VBO (vertex buffer object)
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

        // Bind and set EBO (element buffer object)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

        // Define the vertex attribute pointers
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);

        // Unbind the buffers
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // Unbind VAO
        glBindVertexArray(0);
    }
};
