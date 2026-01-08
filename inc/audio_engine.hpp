#pragma once

#include "miniaudio.h"

#include <glm/glm.hpp>

#include <string>
#include <unordered_map>

class AudioEngine
{
public:
    static AudioEngine& GetInstance() {
        static AudioEngine instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    AudioEngine(const AudioEngine&) = delete;
    void operator=(const AudioEngine&) = delete;

    void PlayOneShotSound(const std::string& path, float volume = 1.0f);
    bool LoopSound(const std::string& path, float volume = 1.0f);
    ma_sound* AddEmitter(const std::string& path, const glm::vec3& position);
    void RemoveEmitter(ma_sound* sound);
    bool StopSound(const std::string& path);
    void StopAll();

    bool SetSoundVolume(const std::string& path, float volume);
    void SetPlayerPosition(const glm::vec3& position, const glm::vec3& forward);

private:
    ma_engine engine;
    bool initialized = false;
    std::unordered_map<std::string, ma_sound*> sounds; // Store sounds by path for volume management
    std::vector<ma_sound*> emitterSounds;

    ma_sound* initSound(const std::string& path, ma_uint32 flags);

    AudioEngine();
    ~AudioEngine();
};