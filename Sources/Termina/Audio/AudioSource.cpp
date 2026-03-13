#include "AudioSource.hpp"
#include "Core/Logger.hpp"

namespace Termina {
    AudioSource::AudioSource(ma_engine& engine)
        : m_Data(nullptr)
        , m_Engine(engine)
    {
    }

    AudioSource::AudioSource(ma_engine& engine, AudioData* data, bool looping)
        : m_Looping(looping)
        , m_Engine(engine)
    {
        SetData(data);
    }

    AudioSource::~AudioSource()
    {
        if (m_Data != nullptr) {
            ma_sound_uninit(&m_Sound);
        }
    }

    void AudioSource::SetData(AudioData* data)
    {
        if (m_Data != nullptr) {
            ma_sound_uninit(&m_Sound);
        }
        m_Data = data;
        if (m_Data != nullptr) {
            ma_result result = ma_sound_init_from_data_source(&m_Engine, &data->GetDecoder(), 0, nullptr, &m_Sound);
            if (result != MA_SUCCESS) {
                TN_ERROR("Failed to initialize audio source!");
            }
        }
    }

    void AudioSource::Play()
    {
        if (m_Data != nullptr) {
            ma_sound_start(&m_Sound);
        }
    }

    void AudioSource::Stop()
    {
        if (m_Data != nullptr) {
            ma_sound_stop(&m_Sound);
        }
    }

    void AudioSource::Update()
    {
        ma_sound_set_looping(&m_Sound, m_Looping);
        ma_sound_set_volume(&m_Sound, m_Volume);
    }

    void AudioSource::SetPosition(const glm::vec3& position)
    {
        ma_sound_set_position(&m_Sound, position.x, position.y, position.z);
    }

    void AudioSource::SetDirection(const glm::vec3& direction)
    {
        ma_sound_set_direction(&m_Sound, direction.x, direction.y, direction.z);
    }

    void AudioSource::SetVelocity(const glm::vec3& velocity)
    {
        ma_sound_set_velocity(&m_Sound, velocity.x, velocity.y, velocity.z);
    }

    void AudioSource::SetSpatialization(bool spatialization)
    {
        ma_sound_set_spatialization_enabled(&m_Sound, true);
    }
}
