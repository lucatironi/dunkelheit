#pragma once

#include "entity.hpp"
#include "fps_camera.hpp"
#include "model.hpp"

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <string>

class Item : public Entity
{
public:
    Item(const std::string& modelPath, const std::string& texturePath,
         const glm::vec3 posOffset, const glm::vec3 rotOffset, const glm::vec3 scaleFactor)
         : positionOffset(posOffset), rotationOffset(rotOffset), scaleFactor(scaleFactor)
    {
        itemModel = std::make_unique<Model>(modelPath);
        if (!texturePath.empty())
            itemModel->TextureOverride(texturePath);

        AlwaysOnTop = true;
    }

    void Update(const float deltaTime, const FPSCamera& camera)
    {
        updateModelMatrix(camera);
    }

    void Draw(const Shader& shader) const override
    {
        shader.Use();
        shader.SetMat4("model", modelMatrix);

        itemModel->Draw(shader);
    }

private:
    std::unique_ptr<Model> itemModel;
    glm::vec3 position;
    glm::vec3 positionOffset;
    glm::vec3 rotationOffset;
    glm::vec3 scaleFactor;
    glm::mat4 modelMatrix;

    void updateModelMatrix(const FPSCamera& camera)
    {
        // Calculate the item relative position based on the camera position and forward direction
        position = camera.Position + camera.Front * positionOffset.z
                                   + camera.Up * positionOffset.y
                                   + camera.Right * positionOffset.x;

        glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 R = glm::mat4_cast(camera.GetRotation());
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scaleFactor);

        // Model = Translation * Rotation * Scale
        modelMatrix = T * R * S;

        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationOffset.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationOffset.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationOffset.z), glm::vec3(0.0f, 0.0f, 1.0f));
    }
};