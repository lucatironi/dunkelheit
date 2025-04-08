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

void AudioEngine::PlaySound(const std::string& path, float volume)
{
    if (initialized)
        ma_engine_play_sound(&engine, path.c_str(), nullptr);
}

bool AudioEngine::LoopSound(const std::string& path, float volume)
{
    if (!initialized)
        return false;

    ma_sound* sound = nullptr;

    // Check if sound is already loaded
    if (sounds.find(path) == sounds.end())
    {
        // Load the sound if it's not in the cache
        ma_sound* newSound = new ma_sound;
        ma_result result = ma_sound_init_from_file(&engine, path.c_str(), MA_SOUND_FLAG_LOOPING, nullptr, nullptr, newSound);
        if (result != MA_SUCCESS)
        {
            std::cerr << "Failed to load sound: " << path << " (error code " << result << ")" << std::endl;
            delete newSound;
            return false;
        }
        sounds[path] = newSound;
        sound = sounds[path];
    }
    else
    {
        sound = sounds[path];
    }

    ma_sound_set_volume(sounds[path], volume);
    ma_sound_start(sound);

    return true;
}

void AudioEngine::StopSound(const std::string& path)
{
    if (sounds.find(path) != sounds.end())
        ma_sound_stop(sounds[path]);
    else
        std::cerr << "Sound not found in cache: " << path << std::endl;
}

void AudioEngine::StopAll()
{
    if (initialized)
        ma_engine_stop(&engine);
}

void AudioEngine::SetSoundVolume(const std::string& path, float volume)
{
    if (sounds.find(path) != sounds.end())
        ma_sound_set_volume(sounds[path], volume);
    else
        std::cerr << "Sound not found in cache: " << path << std::endl;
}