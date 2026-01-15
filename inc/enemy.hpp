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
    STARTLED,
    CRAWL,
    RUN,
    SCREAM,
    ATTACK
};

class Enemy : public Entity
{
public:
    Enemy(const std::string& modelPath, const glm::vec3 position, const float initialAngleY, const glm::vec3 scaleFactor)
          : currentPosition(position), initialPosition(position), scaleFactor(scaleFactor), initialAngleY(initialAngleY), currentState(EnemyState::IDLE)
    {
        enemyModel = std::make_unique<AnimatedModel>();
        ModelLoader& gltf = ModelLoader::GetInstance();
        gltf.LoadFromFile(modelPath, *enemyModel);
        blobShadow = std::make_unique<PlaneModel>("assets/blob_shadow.png");

        Reset();
        updateModelMatrix();
    }

    ~Enemy()
    {
        if (sound)
            ma_sound_stop(sound);
    }

    void Update(const float deltaTime, const FPSCamera& camera, class Level& level)
    {
        EnemyState previousState = currentState;
        float distToPlayer = glm::distance(currentPosition, camera.Position);

        // 1. Pathfinding Logic: Deciding where to go
        pathTimer += deltaTime;
        if (pathTimer >= 0.5f) {
            pathTimer = 0.0f;

            // If we can see the player, clear the path and go straight.
            // Otherwise, calculate the A* path.
            if (level.HasLineOfSight(currentPosition, camera.Position))
            {
                currentPath.clear();
                targetDestination = camera.Position;
            }
            else
            {
                currentPath = level.FindPath(currentPosition, camera.Position);
                if (!currentPath.empty())
                    targetDestination = currentPath[0];
            }
        }

        // 2. Transition Logic (Simplified)
        updateStateTransitions(distToPlayer);

        // 3. ON STATE CHANGE: Trigger One-Shots
        if (currentState != previousState)
        {
            onStateEnter(currentState);
        }

        // 4. CONTINUOUS STATE LOGIC (Movement & Timed Audio)
        handleStateLogic(deltaTime, camera.Position);

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
        glm::vec3 shadowPos = glm::vec3(currentPosition.x, 0.01f, currentPosition.z);

        glm::mat4 shadowModelMatrix = glm::translate(glm::mat4(1.0f), shadowPos);
        shadowModelMatrix = glm::scale(shadowModelMatrix, glm::vec3(2.0f));

        shader.SetMat4("modelMatrix", shadowModelMatrix);
        blobShadow->Draw(shader);

        // Re-enable depth writing for the next object in the frame
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    void ToggleSound(const bool pause)
    {
        if (pause)
            ma_sound_stop(sound);
        else
            ma_sound_start(sound);
    }

    void Reset()
    {
        enemyModel->PlayAnimation("2_idle", 0.5f);
        setState(EnemyState::IDLE);
        currentPosition = initialPosition;
        currentRotation = glm::angleAxis(glm::radians(initialAngleY), glm::vec3(0.0f, 1.0f, 0.0f));

        pathTimer = (float)(rand() % 100) / 200.0f; // Randomize start offset so enemies don't pathfind on the same frame
        nextIdleSoundTimer = (float)(rand() % 10 + 5);
    }

    glm::vec3 GetPosition() const { return currentPosition; }
    void SetPosition(const glm::vec3& pos) { currentPosition = pos; }

private:
    std::unique_ptr<AnimatedModel> enemyModel;
    glm::vec3 initialPosition;
    float initialAngleY;
    glm::vec3 scaleFactor;
    glm::vec3 currentPosition;
    glm::quat currentRotation;
    glm::mat4 modelMatrix;
    std::unique_ptr<PlaneModel> blobShadow;
    EnemyState currentState;
    ma_sound* sound = nullptr;
    std::vector<glm::vec3> currentPath;
    glm::vec3 targetDestination; // The specific point the enemy is currently walking toward
    float pathTimer = 0.0f;
    float nextIdleSoundTimer = 0.0f;
    float footstepTimer = 0.0f;

    void updateStateTransitions(float distToPlayer)
    {
        switch (currentState)
        {
            case EnemyState::IDLE:
                if (distToPlayer < 10.0f) setState(EnemyState::CRAWL);
                break;
            case EnemyState::STARTLED:
                if (distToPlayer < 10.0f) setState(EnemyState::RUN);
                break;
            case EnemyState::CRAWL:
                if (distToPlayer < 3.0f) setState(EnemyState::SCREAM);
                else if (distToPlayer > 12.0f) setState(EnemyState::STARTLED);
                break;
            case EnemyState::RUN:
                if (distToPlayer < 3.0f) setState(EnemyState::ATTACK);
                break;
            case EnemyState::SCREAM:
                if (distToPlayer > 3.5f) setState(EnemyState::CRAWL);
                break;
            case EnemyState::ATTACK:
                if (distToPlayer > 3.5f) setState(EnemyState::RUN);
                break;
        }
    }

    void setState(EnemyState newState)
    {
        currentState = newState;
    }

    void onStateEnter(EnemyState state)
    {
        auto& audio = AudioEngine::GetInstance();
        switch (state)
        {
            case EnemyState::IDLE:
                enemyModel->PlayAnimation("1_idle", 0.5f);
                break;
            case EnemyState::STARTLED:
                enemyModel->PlayAnimation("3_idle", 0.5f);
                break;
            case EnemyState::CRAWL:
                enemyModel->PlayAnimation("5_crouch_walk", 0.5f);
                break;
            case EnemyState::RUN:
                enemyModel->PlayAnimation("7_crawl_run", 0.5f);
                break;
            case EnemyState::SCREAM:
                enemyModel->PlayAnimation("4_scream", 0.2f);
                audio.PlayOneShotSound("assets/monster_scream.wav", currentPosition, 1.0f);
                break;
            case EnemyState::ATTACK:
                enemyModel->PlayAnimation("9_attack", 0.2f);
                // audio.PlayOneShotSound("assets/monster_bite.wav", 1.0f);
                break;
        }
    }

    void handleStateLogic(float deltaTime, glm::vec3 playerPos)
    {
        auto& audio = AudioEngine::GetInstance();
        float speed = 0.0f;

        // 1. Determine Movement Speed based on State
        switch (currentState)
        {
            case EnemyState::IDLE:
            case EnemyState::STARTLED:
            case EnemyState::SCREAM:
            case EnemyState::ATTACK: speed = 0.0f; break;
            case EnemyState::CRAWL:  speed = 2.0f; break;
            case EnemyState::RUN:    speed = 3.5f; break;
        }

        // 2. Execute Movement (Passing playerPos for IDLE/SCREAM to face player)
        if (speed > 0.0f)
        {
            handleMovement(deltaTime, targetDestination, speed);

            // Footstep logic
            footstepTimer -= deltaTime;
            if (footstepTimer <= 0.0f)
            {
                audio.PlayOneShotSound("assets/footstep1.wav", currentPosition, 0.2f);
                footstepTimer = (currentState == EnemyState::RUN) ? 0.25f : 0.5f;
            }
        }
        else
        {
            // Stand still but rotate to face player
            handleMovement(deltaTime, playerPos, 0.0f);

            // Randomized Idle Audio (Only when not screaming/attacking)
            if (currentState == EnemyState::IDLE || currentState == EnemyState::STARTLED)
            {
                nextIdleSoundTimer -= deltaTime;
                if (nextIdleSoundTimer <= 0.0f)
                {
                    audio.PlayOneShotSound("assets/monster_scream.wav", currentPosition, 0.4f);
                    nextIdleSoundTimer = (float)(rand() % 10 + 10); // 10-20 seconds
                }
            }
        }
    }

    void handleMovement(float deltaTime, glm::vec3 target, float speed)
    {
        glm::quat correction = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        float turnSpeed = 4.0f;

        glm::vec3 direction = target - currentPosition;
        direction.y = 0.0f;

        if (glm::length(direction) > 0.01f)
        {
            direction = glm::normalize(direction);

            // 1. Position update
            currentPosition += direction * speed * deltaTime;

            // 2. Rotation update (Face the current waypoint)
            glm::quat lookRot = glm::quatLookAt(direction, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::quat targetRot = lookRot * correction;
            currentRotation = glm::slerp(currentRotation, targetRot, turnSpeed * deltaTime);
        }
    }

    void updateModelMatrix()
    {
        glm::mat4 T = glm::translate(glm::mat4(1.0f), currentPosition);
        glm::mat4 R = glm::mat4_cast(currentRotation);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scaleFactor);

        // Model = Translation * Rotation * Scale
        modelMatrix = T * R * S;
    }
};