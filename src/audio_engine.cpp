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
    {
        // Clean up cached sounds
        for (auto const& [path, sound] : sounds)
        {
            ma_sound_uninit(sound);
            delete sound;
        }

        // Clean up emitter instances
        for (ma_sound* sound : emitterSounds)
        {
            if (sound)
            {
                ma_sound_uninit(sound);
                delete sound;
            }
        }

        ma_engine_uninit(&engine);
    }
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
    ma_sound* pSound = initSound(path, flags);

    if (pSound)
    {
        ma_sound_set_volume(sounds[path], volume);
        ma_sound_start(pSound);

        return true;
    }
    else
        return false;
}

ma_sound* AudioEngine::AddEmitter(const std::string& path, const glm::vec3& position)
{
    if (!initialized)
        return nullptr;

    ma_uint32 flags = MA_SOUND_FLAG_LOOPING;

    ma_sound* pSound = new ma_sound;
    ma_result result = ma_sound_init_from_file(&engine, path.c_str(), flags, nullptr, nullptr, pSound);

    if (result != MA_SUCCESS)
    {
        delete pSound;
        return nullptr;
    }

    ma_sound_set_position(pSound, position.x, position.y, position.z);
    ma_sound_set_attenuation_model(pSound, ma_attenuation_model_linear);
    ma_sound_set_min_distance(pSound, 1.0f);
    ma_sound_set_max_distance(pSound, 10.0f);

    ma_sound_start(pSound);

    // Track it for global cleanup/stop
    emitterSounds.push_back(pSound);

    return pSound;
}

void AudioEngine::RemoveEmitter(ma_sound* pSound)
{
    if (!pSound)
        return;

    // 1. Find the sound in our vector
    auto it = std::find(emitterSounds.begin(), emitterSounds.end(), pSound);
    if (it != emitterSounds.end())
    {
        // 2. Stop and Uninitialize miniaudio object
        ma_sound_stop(pSound);
        ma_sound_uninit(pSound);

        // 3. Delete the memory
        delete pSound;

        // 4. Remove from tracking list
        emitterSounds.erase(it);
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
    else
        ma_sound_stop(sounds[path]);

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
    else
        ma_sound_set_volume(sounds[path], volume);

    return true;
}

void AudioEngine::SetPlayerPosition(const glm::vec3& position, const glm::vec3& forward)
{
    if (initialized)
    {
        ma_engine_listener_set_position(&engine, 0, position.x, position.y, position.z);
        // This tells miniaudio which way the player is looking
        ma_engine_listener_set_direction(&engine, 0, forward.x, forward.y, forward.z);
    }
}

ma_sound* AudioEngine::initSound(const std::string& path, ma_uint32 flags)
{
    ma_sound* pSound = nullptr;

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
            return pSound;
        }
        sounds[path] = newSound;
        pSound = sounds[path];
    }
    else
        pSound = sounds[path];

    return pSound;
}