#pragma once

#include "entity.hpp"
#include "cube_model.hpp"

#include <glad/gl.h>
#include <glm/glm.hpp>

class Object : public Entity
{
public:
    Object(const glm::vec3 pos)
        : position(pos)
    {
        model = std::make_unique<CubeModel>("assets/texture_05.png");
    }

    void Draw(const Shader& shader) const override
    {
        shader.Use();
        glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), position);
        shader.SetMat4("modelMatrix", modelMatrix);
        shader.SetMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(modelMatrix))));
        model->Draw(shader);
    }

private:
    std::unique_ptr<CubeModel> model;
    glm::vec3 position;
};