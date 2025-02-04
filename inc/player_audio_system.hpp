#pragma once

#include "fps_camera.hpp"
#include "random_generator.hpp"

#include <glm/glm.hpp>
#include <irrKlang.h>

#include <iostream>
#include <string>
#include <vector>

struct PlayerState {
    glm::vec3 Position;
    glm::vec3 PreviousPosition;
    bool IsMoving = false;
    bool IsTorchOn = false;
};

class PlayerAudioSystem
{
public:
    PlayerAudioSystem(irrklang::ISoundEngine* engine,
        const std::vector<std::string>& footstepSoundPaths,
        const std::string& torchToggleSoundFile)
        : soundEngine(engine), lastStepTime(0.0f), stepInterval(0.6f)
    {
        for (const auto& path : footstepSoundPaths)
            footstepSounds.push_back(soundEngine->addSoundSourceFromFile(path.c_str()));
        torchToggleSound = soundEngine->addSoundSourceFromFile(torchToggleSoundFile.c_str());
    }

    void Update(PlayerState& player, float elapsedTime)
    {
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
        soundEngine->play2D(torchToggleSound);
    }

private:
    irrklang::ISoundEngine* soundEngine;
    std::vector<irrklang::ISoundSource*> footstepSounds;
    irrklang::ISoundSource* torchToggleSound;
    float lastStepTime;
    float stepInterval; // Time between steps
    const float movementThreshold = 0.002f; // Tunable threshold for detecting movement
    RandomGenerator& random = RandomGenerator::GetInstance();

    void playFootstepSound()
    {
        // Randomly select a footstep sound
        int randomIndex = random.GetRandomInRange(0, 2);
        irrklang::ISoundSource* sound = footstepSounds[randomIndex];
        sound->setDefaultVolume(static_cast<irrklang::ik_f32>(random.GetRandomInRange(3, 6) / 10.f)); // Randomize footsteps volume
        soundEngine->play2D(sound);
    }
};
