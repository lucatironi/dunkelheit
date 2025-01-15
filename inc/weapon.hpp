#pragma once

#include "entity.hpp"
#include "fps_camera.hpp"
#include "model.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>

class Weapon : public Entity
{
public:
    Weapon(const std::string& modelPath, const std::string& texturePath,
           const glm::vec3 posOffset, const glm::vec3 rotOffset, const glm::vec3 scaleFactor)
        : positionOffset(posOffset), rotationOffset(rotOffset)
    {
        weaponModel = new Model(modelPath);
        if (texturePath != "")
            weaponModel->TextureOverride(texturePath, true);

        scaleMatrix    = glm::scale(glm::mat4(1.0f), scaleFactor);
        rotationMatrix = glm::mat4(1.0f); // Initialize with identity matrix
    }

    // Update the weapon's position based on the camera's position and orientation
    void Update(const FPSCamera& camera)
    {
        // Calculate the weapon's position based on the camera's position and forward direction
        glm::vec3 position = camera.Position + camera.Front * positionOffset.z
                                             + camera.Up * positionOffset.y
                                             + camera.Right * positionOffset.x;

        translationMatrix = glm::translate(glm::mat4(1.0f), position);
        rotationMatrix = glm::mat4_cast(camera.GetRotation());
    }

    void Draw(const Shader& shader) const
    {
        // Apply the stored rotation matrix
        glm::mat4 modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;

        // Optionally, apply a fixed rotation offset to the weapon itself (if you need any tweaks)
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationOffset.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationOffset.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationOffset.z), glm::vec3(0.0f, 0.0f, 1.0f));

        shader.Use();
        shader.SetMat4("model", modelMatrix);

        weaponModel->Draw(shader);
    }

private:
    Model* weaponModel;
    glm::vec3 positionOffset;
    glm::vec3 rotationOffset;
    glm::mat4 translationMatrix;
    glm::mat4 rotationMatrix;
    glm::mat4 scaleMatrix;
};