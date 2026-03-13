#pragma once

#include <GLM/glm.hpp>
#include <Termina/World/Component.hpp>

namespace Termina {
    class AudioListenerComponent : public Component
    {
    public:
        AudioListenerComponent() = default;
        AudioListenerComponent(Actor* owner) : Component(owner) {}
        ~AudioListenerComponent();

        void OnPreUpdate(float deltaTime) override;
        void Inspect() override;

        void SetVelocity(const glm::vec3& velocity) { m_Velocity = velocity; }
        glm::vec3 GetVelocity() const { return m_Velocity; }

        UpdateFlags GetUpdateFlags() const override { return (UpdateFlags)0; }

    private:
        bool m_Primary = false;
        glm::vec3 m_Velocity = glm::vec3(0.0f);
    };
}
