#pragma once

#include "entity.hpp"
#include "fps_camera.hpp"
#include "model.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <string>

class Item : public Entity
{
public:
    Item(const std::string& modelPath, const std::string& texturePath,
         const glm::vec3 posOffset, const glm::vec3 rotOffset, const glm::vec3 scaleFactor)
         : positionOffset(posOffset), rotationOffset(rotOffset)
    {
        itemModel = std::make_unique<Model>(modelPath);
        if (texturePath != "")
            itemModel->TextureOverride(texturePath);

        scaleMatrix    = glm::scale(glm::mat4(1.0f), scaleFactor);
        rotationMatrix = glm::mat4(1.0f); // Initialize with identity matrix

        AlwaysOnTop = true;
    }

    // Update the weapon's position based on the camera's position and orientation
    void Update(const FPSCamera& camera)
    {
        // Calculate the weapon's position based on the camera's position and forward direction
        position = camera.Position + camera.Front * positionOffset.z
                                   + camera.Up * positionOffset.y
                                   + camera.Right * positionOffset.x;

        translationMatrix = glm::translate(glm::mat4(1.0f), position);
        rotationMatrix = glm::mat4_cast(camera.GetRotation());
    }

    void Draw(const Shader& shader) const override
    {
        // Apply the stored rotation matrix
        glm::mat4 modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;

        // Optionally, apply a fixed rotation offset to the weapon itself (if you need any tweaks)
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationOffset.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationOffset.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationOffset.z), glm::vec3(0.0f, 0.0f, 1.0f));

        shader.Use();
        shader.SetMat4("model", modelMatrix);

        itemModel->Draw(shader);
    }

private:
    std::unique_ptr<Model> itemModel;
    glm::vec3 position;
    glm::vec3 positionOffset;
    glm::vec3 rotationOffset;
    glm::mat4 translationMatrix;
    glm::mat4 rotationMatrix;
    glm::mat4 scaleMatrix;
};