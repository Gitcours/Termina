#pragma once

#include "AudioData.hpp"

#include <GLM/glm.hpp>

namespace Termina {
    /// Represents an audio source that can be played back and spatialized.
    class AudioSource
    {
    public:
        AudioSource(ma_engine& engine);
        AudioSource(ma_engine& engine, AudioData* data, bool looping = false);
        ~AudioSource();

        /// Plays the audio source.
        void Play();
        /// Stops the audio source.
        void Stop();
        /// Updates the audio source's properties
        void Update();
        /// Loads audio data into the source
        void SetData(AudioData* data);
        /// Sets the audio source's looping state
        void SetLooping(bool looping) { m_Looping = looping; }

        /// Sets the audio source's position
        void SetPosition(const glm::vec3& position);

        /// Sets the audio source's rotation
        void SetDirection(const glm::vec3& direction);

        /// Sets the audio source's velocity
        void SetVelocity(const glm::vec3& velocity);

        /// Sets the sound spatialization mode
        void SetSpatialization(bool spatialization);

        bool IsValid() const { return m_Data != nullptr; }
        ma_sound& GetSound() { return m_Sound; }
        const ma_sound& GetSound() const { return m_Sound; }

        float GetVolume() const { return m_Volume; }
        void SetVolume(float volume) { m_Volume = volume; }
    private:
        ma_engine& m_Engine;
        AudioData* m_Data;
        ma_sound m_Sound;
        bool m_Looping = false;
        float m_Volume = 1.0f;
    };
}
