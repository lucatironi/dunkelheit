#pragma once

#include "audio_engine.hpp"
#include "fps_camera.hpp"
#include "random_generator.hpp"

#include <glm/glm.hpp>

#include <string>
#include <vector>

struct PlayerState {
    glm::vec3 Position;
    glm::vec3 PreviousPosition;
    glm::vec3 Front;
    bool IsMoving = false;
    bool IsTorchOn = false;

    void Init(const FPSCamera& camera) {
        Position = camera.Position;
        PreviousPosition = camera.Position;
        Front = camera.Front;
        IsMoving = false;
        IsTorchOn = true;
    }
};

class PlayerAudioSystem
{
public:
    PlayerAudioSystem(const std::vector<std::string>& footstepSoundPaths, const std::string& torchToggleSoundPath)
        : lastStepTime(0.0f), stepInterval(0.6f), torchToggleSound(torchToggleSoundPath), footstepSounds(footstepSoundPaths)
    {}

    void Update(PlayerState& player, float elapsedTime)
    {
        AudioEngine::GetInstance().SetPlayerPosition(player.Position, player.Front);
        // Check if the player is moving
        float distanceMoved = glm::length(player.Position - player.PreviousPosition);
        player.IsMoving = distanceMoved > movementThreshold;

        // Play footstep sound if moving and enough time has passed
        if (player.IsMoving && (elapsedTime - lastStepTime) > stepInterval)
        {
            playFootstepSound();
            lastStepTime = elapsedTime;
        }
        player.PreviousPosition = player.Position;
    }

    void ToggleTorch(PlayerState& player)
    {
        AudioEngine::GetInstance().PlayOneShotSound(torchToggleSound);
    }

private:
    std::vector<std::string> footstepSounds;
    std::string torchToggleSound;
    float lastStepTime;
    float stepInterval; // Time between steps
    const float movementThreshold = 0.002f; // Tunable threshold for detecting movement
    RandomGenerator& random = RandomGenerator::GetInstance();

    void playFootstepSound()
    {
        // Randomly select a footstep sound
        int randomIndex = random.GetRandomInRange(0, 2);
        float volume = random.GetRandomInRange(3, 6) / 10.f; // Randomize footsteps volume

        AudioEngine::GetInstance().PlayOneShotSound(footstepSounds[randomIndex], volume);
    }
};
