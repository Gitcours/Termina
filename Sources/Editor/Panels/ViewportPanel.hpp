#pragma once

#include "Editor/Panel.hpp"
#include "ImGui/imgui.h"
#include "ImGui/ImGuizmo.h"

class ViewportPanel : public Panel
{
public:
    ViewportPanel(EditorContext& context)
        : Panel("Viewport", context) {}

    void OnImGuiRender() override;

private:
    ImGuizmo::OPERATION m_GizmoOp = ImGuizmo::TRANSLATE;
};
