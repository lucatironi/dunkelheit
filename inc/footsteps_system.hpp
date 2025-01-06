#pragma once

#include "random_generator.hpp"

#include <glm/glm.hpp>
#include <irrKlang.h>

#include <iostream>
#include <string>
#include <vector>

// Player movement state
struct PlayerState {
    glm::vec3 position;
    glm::vec3 previousPosition;
    bool isMoving = false;
};

class FootstepSystem
{
public:
    FootstepSystem(irrklang::ISoundEngine* engine, const std::vector<std::string> footstepSoundPaths)
        : soundEngine(engine), lastStepTime(0.0f), stepInterval(0.6f)
    {
        for (const auto& path : footstepSoundPaths)
            footstepSounds.push_back(soundEngine->addSoundSourceFromFile(path.c_str()));
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
