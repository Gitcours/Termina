#include "CameraComponent.hpp"
#include "GLM/ext/matrix_clip_space.hpp"
#include "GLM/ext/matrix_transform.hpp"
#include "ImGui/imgui.h"
#include "World/Components/Transform.hpp"

#include <Termina/World/Actor.hpp>
#include <Termina/World/World.hpp>
#include <Termina/Core/Application.hpp>
#include <Termina/Core/Window.hpp>

namespace Termina {
    CameraComponent::~CameraComponent()
    {
        m_Owner->GetParentWorld()->SetMainCamera(nullptr);
    }

    void CameraComponent::OnUpdate(float deltaTime)
    {
        Transform& transform = GetOwner()->GetComponent<Transform>();

        m_Camera.Position = transform.GetPosition();
        m_Camera.Direction = transform.GetForward();

        float aspectRatio = 16.0f / 9.0f;
        auto* window = Application::Get().GetWindow();
        if (window && window->GetHeight() > 0)
            aspectRatio = static_cast<float>(window->GetWidth()) / static_cast<float>(window->GetHeight());

        m_Camera.Projection = glm::perspective(glm::radians(m_FOV), aspectRatio, m_Camera.Near, m_Camera.Far);
        m_Camera.View = glm::lookAt(m_Camera.Position, m_Camera.Position + m_Camera.Direction, glm::vec3(0.0f, 1.0f, 0.0f));
        m_Camera.ViewProjection = m_Camera.Projection * m_Camera.View;
        
        m_Camera.InverseProjection = glm::inverse(m_Camera.Projection);
        m_Camera.InverseView = glm::inverse(m_Camera.View);
        m_Camera.InverseViewProjection = glm::inverse(m_Camera.ViewProjection);
    }

    void CameraComponent::OnPreUpdate(float deltaTime)
    {
        if (m_Primary)
            m_Owner->GetParentWorld()->SetMainCamera(m_Owner);
    }

    void CameraComponent::OnPostRender(float deltaTime)
    {
        m_Camera.UpdatePrevious();
    }

    void CameraComponent::Inspect()
    {
        ImGui::Checkbox("Primary", &m_Primary);
        ImGui::DragFloat("Near", &m_Camera.Near, 0.01f, 0.01f, 100.0f);
        ImGui::DragFloat("Far", &m_Camera.Far, 0.01f, 0.01f, 100.0f);
        ImGui::DragFloat("FOV", &m_FOV, 1.0f, 1.0f, 179.0f);
    }

    void CameraComponent::Serialize(nlohmann::json& out) const
    {
        out["primary"] = m_Primary;
        out["near"] = m_Camera.Near;
        out["far"] = m_Camera.Far;
        out["fov"] = m_FOV;
    }

    void CameraComponent::Deserialize(const nlohmann::json& in)
    {
        m_Primary = in.value("primary", false);
        m_Camera.Near = in.value("near", 0.1f);
        m_Camera.Far = in.value("far", 100.0f);
        m_FOV = in.value("fov", 75.0f);
    }
}
