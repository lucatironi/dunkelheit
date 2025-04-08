#pragma once

#include "miniaudio.h"

#include <string>
#include <unordered_map>

class AudioEngine
{
public:
    AudioEngine();
    ~AudioEngine();

    void PlaySound(const std::string& path, float volume = 1.0f);
    bool LoopSound(const std::string& path, float volume = 1.0f);
    void StopSound(const std::string& path);
    void StopAll();
    void SetSoundVolume(const std::string& path, float volume);

private:
    ma_engine engine;
    bool initialized = false;
    std::unordered_map<std::string, ma_sound*> sounds; // Store sounds by path for volume management
};