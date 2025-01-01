#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "file_system.hpp"
#include "cube_model.hpp"

class Object
{
public:
    Object(const glm::vec3 pos)
        : position(pos)
    {
    }

    void Draw(const Shader& shader)
    {
        shader.Use();
        shader.SetMat4("model", glm::translate(glm::mat4(1.0f), position));
        model.Draw(shader);
    }

private:
    CubeModel model;
    glm::vec3 position;
};