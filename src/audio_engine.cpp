#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include "audio_engine.hpp"

#include <iostream>

AudioEngine::AudioEngine()
{
    if (ma_engine_init(nullptr, &engine) == MA_SUCCESS)
        initialized = true;
    else
        std::cerr << "Failed to initialize miniaudio engine." << std::endl;
}

AudioEngine::~AudioEngine()
{
    if (initialized)
        ma_engine_uninit(&engine);
}

void AudioEngine::PlayOneShotSound(const std::string& path, float volume)
{
    if (initialized)
        ma_engine_play_sound(&engine, path.c_str(), nullptr);
}

bool AudioEngine::LoopSound(const std::string& path, float volume)
{
    if (!initialized)
        return false;

    ma_uint32 flags = MA_SOUND_FLAG_LOOPING | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_NO_PITCH;
    ma_sound* sound = initSound(path, flags);
    if (!sound)
        return false;
    else
    {
        ma_sound_set_volume(sounds[path], volume);
        ma_sound_start(sound);

        return true;
    }
}

bool AudioEngine::AddEmitter(const std::string& path, const glm::vec3& position)
{
    if (!initialized)
        return false;

    ma_uint32 flags = MA_SOUND_FLAG_LOOPING | MA_SOUND_FLAG_NO_PITCH;
    ma_sound* sound = initSound(path, flags);
    if (!sound)
        return false;
    else
    {
        ma_sound_set_position(sound, position.x, position.y, position.z);
        ma_sound_start(sound);

        return true;
    }
}

bool AudioEngine::StopSound(const std::string& path)
{
    if (!initialized)
        return false;

    if (sounds.find(path) == sounds.end())
    {
        std::cerr << "Sound not found in cache: " << path << std::endl;
        return false;
    }
    else {
        ma_sound_stop(sounds[path]);
    }

    return true;
}

void AudioEngine::StopAll()
{
    if (initialized)
        ma_engine_stop(&engine);
}

bool AudioEngine::SetSoundVolume(const std::string& path, float volume)
{
    if (!initialized)
        return false;

    if (sounds.find(path) == sounds.end())
    {
        std::cerr << "Sound not found in cache: " << path << std::endl;
        return false;
    }
    else {
        ma_sound_set_volume(sounds[path], volume);
    }

    return true;
}

void AudioEngine::SetPlayerPosition(const glm::vec3& position)
{
    if (initialized)
        ma_engine_listener_set_position(&engine, 0, position.x, position.y, position.z);
}

ma_sound* AudioEngine::initSound(const std::string& path, ma_uint32 flags)
{
    ma_sound* sound = nullptr;

    // Check if sound is already loaded
    if (sounds.find(path) == sounds.end())
    {
        // Load the sound if it's not in the cache
        ma_sound* newSound = new ma_sound;
        ma_result result = ma_sound_init_from_file(&engine, path.c_str(), flags, nullptr, nullptr, newSound);
        if (result != MA_SUCCESS)
        {
            std::cerr << "Failed to load sound: " << path << " (error code " << result << ")" << std::endl;
            delete newSound;
            return sound;
        }
        sounds[path] = newSound;
        sound = sounds[path];
    }
    else
        sound = sounds[path];

    return sound;
}