#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "fps_camera.hpp"
#include "model.hpp"

class Weapon
{
public:
    // Constructor
    Weapon()
    {
        weaponModel = new Model(FileSystem::GetPath("assets/r_hard_open.fbx"));
        weaponModel->TextureOverride("assets/hard_open_AO.png", true);

        // Default position and rotation offsets relative to the camera
        positionOffset = glm::vec3(1.2f, -1.0f, 1.4f);
        rotationOffset = glm::vec3(-45.0f, 170.0f, 0.0f);
        scaleMatrix    = glm::scale(glm::mat4(1.0f), glm::vec3(4.0f));
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

    // Draw the weapon
    void Draw(const Shader& shader)
    {
        // Apply the stored rotation matrix
        glm::mat4 modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;

        // Optionally, apply a fixed rotation offset to the weapon itself (if you need any tweaks)
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationOffset.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationOffset.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationOffset.z), glm::vec3(0.0f, 0.0f, 1.0f));

        shader.Use();
        shader.SetMat4("model", modelMatrix);

        weaponModel->Draw();
    }

private:
    Model* weaponModel;
    glm::vec3 positionOffset;
    glm::vec3 rotationOffset;
    glm::mat4 translationMatrix;
    glm::mat4 rotationMatrix;
    glm::mat4 scaleMatrix;
};