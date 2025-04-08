#pragma once

#include "miniaudio.h"

#include <glm/glm.hpp>

#include <string>
#include <unordered_map>

class AudioEngine
{
public:
    AudioEngine();
    ~AudioEngine();

    void PlaySound(const std::string& path, float volume = 1.0f);
    bool LoopSound(const std::string& path, float volume = 1.0f);
    bool AddEmitter(const std::string& path, const glm::vec3& position);
    void StopSound(const std::string& path);
    void StopAll();
    void SetSoundVolume(const std::string& path, float volume);

    void SetPlayerPosition(const glm::vec3& position);

private:
    ma_engine engine;
    bool initialized = false;
    std::unordered_map<std::string, ma_sound*> sounds; // Store sounds by path for volume management
};