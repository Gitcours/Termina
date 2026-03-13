#pragma once

#include "Editor/Panel.hpp"

class InspectorPanel : public Panel
{
public:
    InspectorPanel(EditorContext& context)
        : Panel("Inspector", context) {}

    void OnImGuiRender() override;
};
