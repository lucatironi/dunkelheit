#pragma once

#include "animated_model.hpp"
#include "audio_engine.hpp"
#include "entity.hpp"
#include "fps_camera.hpp"
#include "model_loader.hpp"
#include "plane_model.hpp"

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <string>

enum class EnemyState {
    IDLE,
    CRAWL,
    RUN,
    SCREAM,
    ATTACK
};

class Enemy : public Entity
{
public:
    Enemy(const std::string& modelPath, const glm::vec3 position, const float angleY, const glm::vec3 scaleFactor)
          : position(position), scaleFactor(scaleFactor), angleY(angleY), currentState(EnemyState::IDLE)
    {
        enemyModel = std::make_unique<AnimatedModel>();
        ModelLoader& gltf = ModelLoader::GetInstance();
        gltf.LoadFromFile(modelPath, *enemyModel);
        enemyModel->PlayAnimation("1_idle", 0.5f);
        blobShadow = std::make_unique<PlaneModel>("assets/blob_shadow.png");
        sound = AudioEngine::GetInstance().AddEmitter("assets/gizmo.wav", position);

        currentRotation = glm::angleAxis(glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));
        updateModelMatrix();
    }

    ~Enemy()
    {
        if (sound)
            ma_sound_stop(sound);
    }

    void Update(const float deltaTime, const FPSCamera& camera)
    {
        float distToPlayer = glm::distance(position, camera.Position);

        // State Transition Logic
        switch (currentState) {
            case EnemyState::IDLE:
                if (distToPlayer < 7.0f)
                {
                    currentState = EnemyState::CRAWL;
                    enemyModel->PlayAnimation("6_crawl_run", 0.5f);
                }
                break;

            case EnemyState::CRAWL:
                handleMovement(deltaTime, camera.Position, 3.5f);

                if (distToPlayer < 3.0f)
                {
                    currentState = EnemyState::SCREAM;
                    enemyModel->PlayAnimation("3_scream", 0.5f);
                }
                else if (distToPlayer > 10.0f)
                {
                    currentState = EnemyState::IDLE;
                    enemyModel->PlayAnimation("1_idle", 0.5f);
                }
                break;

            case EnemyState::SCREAM:
                // Face the player while attacking
                handleMovement(deltaTime, camera.Position, 0.0f);

                // Logic: If animation finished or player moved away, resume chase
                if (distToPlayer > 3.5f)
                {
                    currentState = EnemyState::CRAWL;
                    enemyModel->PlayAnimation("6_crawl_run", 0.5f);
                }
                break;
        }

        updateModelMatrix();
        enemyModel->UpdateAnimation(deltaTime);

        if (sound)
            ma_sound_set_position(sound, position.x, position.y, position.z);
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
    EnemyState currentState;
    ma_sound* sound = nullptr;

    void handleMovement(float deltaTime, glm::vec3 targetPos, float speed) {
        glm::quat correction = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        float turnSpeed = 2.0f;

        glm::vec3 direction = targetPos - position;
        direction.y = 0.0f;

        if (glm::length(direction) > 0.1f) {
            direction = glm::normalize(direction);

            // Move toward player
            position += direction * speed * deltaTime;

            // Rotation
            glm::quat lookRot = glm::quatLookAt(direction, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::quat targetRot = lookRot * correction;
            currentRotation = glm::slerp(currentRotation, targetRot, turnSpeed * deltaTime);
        }
    }

    void updateModelMatrix()
    {
        glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 R = glm::mat4_cast(currentRotation);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scaleFactor);

        // Model = Translation * Rotation * Scale
        modelMatrix = T * R * S;
    }
};