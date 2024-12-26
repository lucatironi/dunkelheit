#pragma once

#include <iostream>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <irrKlang.h>

#include "file_system.hpp"
#include "random_generator.hpp"

// Player movement state
struct PlayerState {
    glm::vec3 position;
    glm::vec3 previousPosition;
    bool isMoving = false;
};

class FootstepSystem
{
public:
    FootstepSystem(irrklang::ISoundEngine* engine)
        : soundEngine(engine), lastStepTime(0.0f), stepInterval(0.6f)
    {
        // Add footstep sounds to the list
        std::vector<std::string> footstepSoundPaths = {
            "assets/footstep1.wav",
            "assets/footstep2.wav",
            "assets/footstep3.wav"
        };

        for (const auto& path : footstepSoundPaths)
            footstepSounds.push_back(soundEngine->addSoundSourceFromFile(FileSystem::GetPath(path).c_str()));
    }

    void Update(float elapsedTime, PlayerState& player)
    {
        // Check if the player is moving
        float distanceMoved = glm::length(player.position - player.previousPosition);
        player.isMoving = distanceMoved > movementThreshold;

        // Play footstep sound if moving and enough time has passed
        if (player.isMoving && (elapsedTime - lastStepTime) > stepInterval)
        {
            playFootstepSound();
            lastStepTime = elapsedTime;
        }

        // Update player's previous position
        player.previousPosition = player.position;
    }

private:
    irrklang::ISoundEngine* soundEngine;
    std::vector<irrklang::ISoundSource*> footstepSounds;
    float lastStepTime;
    float stepInterval; // Time between steps
    const float movementThreshold = 0.002f; // Tunable threshold for detecting movement

    void playFootstepSound()
    {
        // Randomly select a footstep sound
        int randomIndex = getRandomInRange(0, 2);
        irrklang::ISoundSource* sound = footstepSounds[randomIndex];
        sound->setDefaultVolume(static_cast<irrklang::ik_f32>(getRandomInRange(3, 6) / 10.f)); // Randomize footsteps volume
        soundEngine->play2D(sound);
    }
};
