#include "AudioListenerComponent.hpp"

#include <Termina/World/Actor.hpp>
#include <Termina/World/World.hpp>

#include <ImGui/imgui.h>

namespace Termina {
    AudioListenerComponent::~AudioListenerComponent()
    {
        if (m_Primary)
            m_Owner->GetParentWorld()->SetAudioListener(nullptr);
    }

    void AudioListenerComponent::OnPreUpdate(float deltaTime)
    {
        if (m_Primary)
            m_Owner->GetParentWorld()->SetAudioListener(m_Owner);
    }

    void AudioListenerComponent::Inspect()
    {
        ImGui::Checkbox("Primary", &m_Primary);
        ImGui::InputFloat3("Velocity", &m_Velocity.x);
    }
}
