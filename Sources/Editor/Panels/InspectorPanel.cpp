#include "InspectorPanel.hpp"

#include "ImGui/imgui.h"
#include "Termina/Renderer/UIUtils.hpp"
#include <Termina/Core/IInspectable.hpp>

void InspectorPanel::OnImGuiRender()
{
    Termina::UIUtils::BeginEditorWindow(m_Name.c_str(), &m_Open);

    Termina::IInspectable* item = m_Context.ItemToInspect;
    if (!item)
    {
        ImGui::TextDisabled("No item selected");
        Termina::UIUtils::EndEditorWindow();
        return;
    }

    // Let the inspected item render its own inspector UI.
    item->Inspect();

    Termina::UIUtils::EndEditorWindow();
}