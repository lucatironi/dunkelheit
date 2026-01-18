#pragma once

#include "cube_model.hpp"
#include "entity.hpp"
#include "fps_camera.hpp"

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

    void Update(const float deltaTime, const FPSCamera& camera)
    {
        // 1. Update the rotation angle
        rotationY += rotationSpeed * deltaTime;

        // Keep rotation between 0 and 360 to prevent floating point overflow over time
        if (rotationY > 360.0f) rotationY -= 360.0f;
    }

    void Draw(const Shader& shader) const override
    {
        shader.Use();
        shader.SetMat4("modelMatrix", getModelMatrix());
        shader.SetMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(getModelMatrix()))));
        model->Draw(shader);
    }

private:
    std::unique_ptr<CubeModel> model;
    glm::vec3 position;
    float rotationY = 0.0f;
    float rotationSpeed = 20.0f; // Degrees per second

    glm::mat4 getModelMatrix() const
    {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
        // Convert degrees to radians for GLM
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));

        return modelMatrix;
    }
};