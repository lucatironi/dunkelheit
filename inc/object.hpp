#pragma once

#include "entity.hpp"
#include "cube_model.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>

class Object : public Entity
{
public:
    Object(const glm::vec3 pos)
        : position(pos)
    {}

    void Draw(const Shader& shader) const
    {
        shader.Use();
        shader.SetMat4("model", glm::translate(glm::mat4(1.0f), position));
        model.Draw(shader);
    }

private:
    CubeModel model;
    glm::vec3 position;
};