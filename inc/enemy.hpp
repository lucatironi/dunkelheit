#pragma once

#include "animated_model.hpp"
#include "audio_engine.hpp"
#include "entity.hpp"
#include "fps_camera.hpp"
#include "level.hpp"
#include "model_loader.hpp"
#include "plane_model.hpp"

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <string>
#include <vector>

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
        pathTimer = (float)(rand() % 100) / 200.0f; // Randomize start offset so enemies don't pathfind on the same frame

        currentRotation = glm::angleAxis(glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));
        updateModelMatrix();
    }

    ~Enemy()
    {
        if (sound)
            ma_sound_stop(sound);
    }

    void Update(const float deltaTime, const FPSCamera& camera, class Level& level)
    {
        float distToPlayer = glm::distance(position, camera.Position);

        // 1. Pathfinding Logic: Deciding where to go
        pathTimer += deltaTime;
        if (pathTimer >= 0.5f) {
            pathTimer = 0.0f;

            // If we can see the player, clear the path and go straight.
            // Otherwise, calculate the A* path.
            if (level.HasLineOfSight(position, camera.Position))
            {
                currentPath.clear();
                targetDestination = camera.Position;
            }
            else
            {
                currentPath = level.FindPath(position, camera.Position);
                if (!currentPath.empty())
                {
                    targetDestination = currentPath[0];
                }
            }
        }

        // 2. State Machine Logic
        switch (currentState) {
            case EnemyState::IDLE:
                if (distToPlayer < 10.0f)
                {
                    currentState = EnemyState::CRAWL;
                    enemyModel->PlayAnimation("6_crawl_run", 0.5f);
                }
                break;

            case EnemyState::CRAWL:
                // Move toward the current waypoint or target destination
                handleMovement(deltaTime, targetDestination, 3.5f);

                // If we reached a waypoint, move to the next one
                if (!currentPath.empty() && glm::distance(position, targetDestination) < 0.5f)
                {
                    currentPath.erase(currentPath.begin());
                    if (!currentPath.empty())
                    {
                        targetDestination = currentPath[0];
                    }
                }

                if (distToPlayer < 3.0f)
                {
                    currentState = EnemyState::SCREAM;
                    enemyModel->PlayAnimation("3_scream", 0.5f);
                }
                else if (distToPlayer > 12.0f)
                {
                    currentState = EnemyState::IDLE;
                    enemyModel->PlayAnimation("1_idle", 0.5f);
                }
                break;

            case EnemyState::SCREAM:
                handleMovement(deltaTime, camera.Position, 0.0f);
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

    glm::vec3 GetPosition() const { return position; }
    void SetPosition(const glm::vec3& pos) { position = pos; }

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
    std::vector<glm::vec3> currentPath;
    glm::vec3 targetDestination; // The specific point the enemy is currently walking toward
    float pathTimer = 0.0f;

    void handleMovement(float deltaTime, glm::vec3 target, float speed) {
        glm::quat correction = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        float turnSpeed = 4.0f;

        glm::vec3 direction = target - position;
        direction.y = 0.0f;

        if (glm::length(direction) > 0.01f) {
            direction = glm::normalize(direction);

            // 1. Position update
            position += direction * speed * deltaTime;

            // 2. Rotation update (Face the current waypoint)
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