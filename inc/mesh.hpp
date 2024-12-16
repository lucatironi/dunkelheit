#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "texture2d.hpp"

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture {
    Texture2D texture;
    std::string type;
    std::string path;
};

class Mesh
{
public:
    Mesh(const std::vector<Vertex>& _vertices, const std::vector<GLuint>& _indices, const std::vector<Texture>& _textures)
        : VAO(0), VBO(0), EBO(0)
    {
        vertices = _vertices;
        indices = _indices;
        textures = _textures;

        setupBuffers();
    }

    // Draw the mesh
    void Draw()
    {
        // bindTextures();
        glBindVertexArray(VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

private:
    GLuint VAO, VBO, EBO;  // Vertex Array Object, Vertex Buffer Object, Element Buffer Object
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture> textures;

    void setupBuffers()
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        // Bind VAO
        glBindVertexArray(VAO);

        // Bind and set VBO (vertex buffer object)
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        // Bind and set EBO (element buffer object)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

        // vertex position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // vertex texture coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        // Unbind the buffers
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // Unbind VAO
        glBindVertexArray(0);
    }

    void bindTextures() const
    {
        for (size_t i = 0; i < textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            textures[i].texture.Bind();
        }
        glActiveTexture(GL_TEXTURE0);
    }
};
