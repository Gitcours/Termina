#include "ViewportPanel.hpp"
#include "ImGui/imgui.h"
#include "ImGui/ImGuizmo.h"
#include "ImGui/imgui_internal.h"
#include "Termina/Core/Application.hpp"
#include "Termina/RHI/TextureView.hpp"
#include "Termina/Renderer/Renderer.hpp"
#include "Termina/Renderer/UIUtils.hpp"
#include "Termina/World/Components/Transform.hpp"
#include "Termina/World/WorldSystem.hpp"

#include <GLM/gtc/type_ptr.hpp>
#include <GLM/gtx/matrix_decompose.hpp>

void ViewportPanel::OnImGuiRender()
{
    auto renderer = Termina::Application::Get().GetSystem<Termina::RendererSystem>();
    auto* worldSystem = Termina::Application::GetSystem<Termina::WorldSystem>();
    auto colorTexture = renderer->GetPassIO()->GetTexture("RendererOutput");

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    Termina::UIUtils::BeginEditorWindow(m_Name.c_str(), &m_Open);
    ImGui::PopStyleVar(2);

    // Toolbar
    ImGui::SetCursorPosX(4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 10, 5 });
    Termina::UIUtils::PushStylized();
    if (worldSystem->IsPlaying()) {
        if (Termina::UIUtils::Button("Stop"))
            worldSystem->Stop();
    } else {
        if (Termina::UIUtils::Button("Play"))
            worldSystem->Play();
    }
    ImGui::PopStyleVar();
    ImGui::SameLine();
    ImGui::Text("|");
    Termina::UIUtils::PopStylized();
    ImGui::SameLine();
    auto gizmoButton = [&](const char* label, ImGuizmo::OPERATION op) {
        bool active = (m_GizmoOp == op);
        if (active)
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::Button(label))
            m_GizmoOp = op;
        if (active)
            ImGui::PopStyleColor();
        ImGui::SameLine();
    };
    gizmoButton("T", ImGuizmo::TRANSLATE);
    gizmoButton("R", ImGuizmo::ROTATE);
    gizmoButton("S", ImGuizmo::SCALE);
    ImGui::NewLine();

    if (colorTexture) {
        auto colorView = renderer->GetResourceViewCache()->GetTextureView(
            Termina::TextureViewDesc()
                .SetFormat(Termina::TextureFormat::RGBA8_UNORM)
                .SetType(Termina::TextureViewType::SHADER_READ)
                .SetDimension(Termina::TextureViewDimension::TEXTURE_2D)
                .SetMipRange(0, 1)
                .SetArrayLayerRange(0, 1)
                .SetTexture(colorTexture)
        );
        auto contentRegion = ImGui::GetContentRegionAvail();
        ImGui::Image((ImTextureID)colorView->GetBindlessIndex(), {contentRegion.x, contentRegion.y}, {0, 0}, {1, 1});
    }

    float windowWidth = (float)ImGui::GetWindowWidth();
    float windowHeight = (float)ImGui::GetWindowHeight();
    
    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);

    if (m_Context.SelectedActor) {
        Termina::Transform& transform = m_Context.SelectedActor->GetComponent<Termina::Transform>();
        glm::mat4 worldMatrix = transform.GetWorldMatrix();

        glm::mat4 view = renderer->GetCurrentCamera().View;
        glm::mat4 projection = renderer->GetCurrentCamera().Projection;
        projection[1][1] *= -1.0f; // Negate Y for ImGuizmo's expected coordinate system

        if (ImGuizmo::Manipulate(glm::value_ptr(view),
                             glm::value_ptr(projection),
                             m_GizmoOp,
                             ImGuizmo::WORLD,
                             glm::value_ptr(worldMatrix))) {
            glm::vec3 position, scale, skew;
            glm::quat rotation;
            glm::vec4 perspective;
            glm::decompose(worldMatrix, scale, rotation, position, skew, perspective);

            // Validate decomposed values - skip if NaN detected
            auto isValid = [](const glm::vec3& v) { return !glm::any(glm::isnan(v)); };
            if (isValid(position) && isValid(scale) && !glm::any(glm::isnan(glm::vec4(rotation.x, rotation.y, rotation.z, rotation.w)))) {
                // Clamp scale to prevent near-zero values
                const float minScale = 0.001f;
                scale.x = glm::max(glm::abs(scale.x), minScale) * glm::sign(scale.x);
                scale.y = glm::max(glm::abs(scale.y), minScale) * glm::sign(scale.y);
                scale.z = glm::max(glm::abs(scale.z), minScale) * glm::sign(scale.z);

                transform.SetLocalPosition(position);
                transform.SetLocalRotation(rotation);
                transform.SetLocalScale(scale);
            }                        
        }
    }

    Termina::UIUtils::EndEditorWindow();
}
