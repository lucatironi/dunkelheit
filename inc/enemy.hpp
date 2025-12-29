#pragma once

#include "animated_model.hpp"
#include "entity.hpp"
#include "fps_camera.hpp"
#include "model_loader.hpp"
#include "plane_model.hpp"

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <string>

class Enemy : public Entity
{
public:
    Enemy(const std::string& modelPath, const glm::vec3 position, const float angleY, const glm::vec3 scaleFactor)
          : position(position), scaleFactor(scaleFactor), angleY(angleY)
    {
        enemyModel = std::make_unique<AnimatedModel>();
        ModelLoader& gltf = ModelLoader::GetInstance();
        gltf.LoadFromFile(modelPath, *enemyModel);
        enemyModel->SetCurrentAnimation(0);

        currentRotation = glm::angleAxis(glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));
        updateModelMatrix();
    }

    void Update(const float deltaTime, const FPSCamera& camera)
    {
        glm::quat correction = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        float moveSpeed = 2.0f;
        float turnSpeed = 1.0f;

        glm::vec3 direction = camera.Position - position;
        direction.y = 0.0f;

        if (glm::length(direction) > 0.1f)
        {
            direction = glm::normalize(direction);
            // position += direction * moveSpeed * deltaTime;

            glm::quat lookRot = glm::quatLookAt(direction, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::quat targetRot = lookRot * correction;

            currentRotation = glm::slerp(currentRotation, targetRot, turnSpeed * deltaTime);
        }

        updateModelMatrix();
        enemyModel->UpdateAnimation(deltaTime);
    }

    void Draw(const Shader& shader) const override
    {
        // 1. Draw the Enemy Model as usual
        shader.Use();
        shader.SetMat4("modelMatrix", modelMatrix);
        shader.SetMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(modelMatrix))));
        enemyModel->SetBoneTransformations(shader);
        enemyModel->Draw(shader);

        // 2. Prepare for Transparent Shadow
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Disable depth writing so shadows don't clip each other or the floor
        glDepthMask(GL_FALSE);

        // Calculate shadow position (slightly above the floor to avoid z-fighting)
        // We ignore the enemy's current Y and put it at ground level (e.g., 0.01)
        glm::vec3 shadowPos = glm::vec3(position.x, 0.01f, position.z);

        // Scale the shadow based on the enemy's base scale
        // Note: We don't use the enemy's rotation for the shadow (retro style)
        glm::mat4 shadowModelMatrix = glm::translate(glm::mat4(1.0f), shadowPos);
        shadowModelMatrix = glm::scale(shadowModelMatrix, glm::vec3(1.5f)); // Make shadow slightly wider than enemy

        shader.SetMat4("modelMatrix", shadowModelMatrix);
        blobShadow->Draw(shader);

        // Re-enable depth writing for the next object in the frame
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

private:
    std::unique_ptr<AnimatedModel> enemyModel;
    glm::vec3 position;
    glm::vec3 scaleFactor;
    float angleY;
    glm::quat currentRotation;
    glm::mat4 modelMatrix;
    std::unique_ptr<PlaneModel> blobShadow;

    void updateModelMatrix()
    {
        glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 R = glm::mat4_cast(currentRotation);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scaleFactor);

        // Model = Translation * Rotation * Scale
        modelMatrix = T * R * S;
    }
};