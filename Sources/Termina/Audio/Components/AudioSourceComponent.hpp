#pragma once

#include <Termina/Audio/AudioSource.hpp>
#include <Termina/Asset/AssetHandle.hpp>
#include <Termina/Asset/Audio/AudioAsset.hpp>
#include <Termina/World/Component.hpp>

namespace Termina {
    class AudioSourceComponent : public Component
    {
    public:
        AudioSourceComponent();
        AudioSourceComponent(Actor* owner) : Component(owner) {}
        ~AudioSourceComponent();

        void OnPlay() override;
        void OnStop() override;
        void OnUpdate(float deltaTime) override;
        void Inspect() override;

        void SetVelocity(const glm::vec3& velocity) { m_Velocity = velocity; }
        void ShouldPlayOnAwake(bool playOnAwake) { m_PlayOnAwake = playOnAwake; }
        void SetLooping(bool looping) { m_Loop = looping; }
        void SetSpatialized(bool spatialized) { m_Spatialized = spatialized; }
        void SetVolume(float volume) { m_Volume = volume; }

        glm::vec3 GetVelocity() const { return m_Velocity; }
        bool GetPlayOnAwake() const { return m_PlayOnAwake; }
        bool GetLooping() const { return m_Loop; }
        bool GetSpatialized() const { return m_Spatialized; }
        float GetVolume() const { return m_Volume; }

        UpdateFlags GetUpdateFlags() const override { return (UpdateFlags)0; }

    private:
        void OnAssetChange(const std::string& path);

        bool m_PlayOnAwake = false;
        bool m_Loop = false;
        bool m_Spatialized = false;
        float m_Volume = 1.0f;
        glm::vec3 m_Velocity;

        AssetHandle<AudioAsset> m_AudioAsset;
        AudioSource* m_Source;
    };
}
