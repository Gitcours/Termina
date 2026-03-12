#pragma once

#include "Editor/Panel.hpp"

namespace Termina { class Actor; }

class WorldHierarchyPanel : public Panel
{
public:
    WorldHierarchyPanel(EditorContext& context)
        : Panel("World Hierarchy", context) {}

    void OnImGuiRender() override;

private:
    void DrawActorNode(Termina::Actor* actor);
};
