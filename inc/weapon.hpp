#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "fps_camera.hpp"
#include "mesh.hpp"

class Weapon {
public:
    // Constructor
    Weapon() {
        // Default position and rotation offsets relative to the camera
        positionOffset = glm::vec3(1.0f, -1.0f, 1.5f);
        rotationOffset = glm::vec3(0.0f, 10.0f, 0.0f);
        rotationMatrix = glm::mat4(1.0f); // Initialize with identity matrix
    }

    // Update the weapon's position based on the camera's position and orientation
    void Update(const FPSCamera& camera) {
        // Calculate the weapon's position based on the camera's position and forward direction
        position = camera.Position + camera.Front * positionOffset.z
                                   + camera.Up * positionOffset.y
                                   + camera.Right * positionOffset.x;

        rotationMatrix = glm::mat4_cast(camera.GetRotation());
    }

    // Draw the weapon
    void Draw(const Shader& shader) {
        glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), position);

        // Apply the stored rotation matrix
        modelMatrix = modelMatrix * rotationMatrix;

        // Optionally, apply a fixed rotation offset to the weapon itself (if you need any tweaks)
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationOffset.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationOffset.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationOffset.z), glm::vec3(0.0f, 0.0f, 1.0f));

        shader.Use();
        shader.SetMat4("model", modelMatrix);

        weaponMesh.Draw();
    }

private:
    Mesh weaponMesh; // Weapon's geometry (simple cube for now)
    glm::vec3 position; // Position of the weapon in the world
    glm::vec3 rotation; // Rotation of the weapon in the world
    glm::vec3 positionOffset;
    glm::vec3 rotationOffset;
    glm::mat4 rotationMatrix; // Rotation matrix to be applied to the weapon
};