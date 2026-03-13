#include "AudioSourceComponent.hpp"
#include "Audio/AudioSystem.hpp"
#include "Core/Application.hpp"
#include "World/Components/Transform.hpp"
#include "World/Actor.hpp"
#include "Asset/AssetSystem.hpp"

#include <ImGui/imgui.h>
#include <GLM/gtc/type_ptr.hpp>

namespace Termina {
    AudioSourceComponent::AudioSourceComponent()
    {
        AudioSystem* system = Application::GetSystem<AudioSystem>();

        m_Source = new AudioSource(system->GetEngine());
    }

    AudioSourceComponent::~AudioSourceComponent()
    {
        delete m_Source;
    }

    void AudioSourceComponent::OnPlay()
    {
        if (!m_AudioAsset) return;

        m_Source->Play();
    }

    void AudioSourceComponent::OnStop()
    {
        if (!m_AudioAsset) return;

        m_Source->Stop();
    }

    void AudioSourceComponent::OnUpdate(float deltaTime)
    {
        if (!m_AudioAsset) return;

        m_Source->SetVolume(m_Volume);
        m_Source->SetLooping(m_Loop);
        if (m_Spatialized) {
            Transform transform = m_Owner->GetComponent<Transform>();

            m_Source->SetSpatialization(true);
            m_Source->SetPosition(transform.GetPosition());
            m_Source->SetDirection(transform.GetForward());
            m_Source->SetVelocity(m_Velocity);
        } else {
            m_Source->SetSpatialization(false);
        }

        m_Source->Update();
    }

    void AudioSourceComponent::OnAssetChange(const std::string& path)
    {
        m_AudioAsset = Application::GetSystem<AssetSystem>()->Load<AudioAsset>(path);
        if (m_AudioAsset) {
            m_Source->SetData(m_AudioAsset->GetData());
        }
    }

    void AudioSourceComponent::Inspect()
    {
        ImGui::Checkbox("Play on Awake", &m_PlayOnAwake);
        ImGui::Checkbox("Loop", &m_Loop);
        ImGui::Checkbox("Spatialized", &m_Spatialized);
        ImGui::SliderFloat("Volume", &m_Volume, 0.0f, 1.0f);
        ImGui::InputFloat3("Velocity", glm::value_ptr(m_Velocity));
    }
}
