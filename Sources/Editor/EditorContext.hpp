#pragma once

#include <Termina/Core/IInspectable.hpp>
#include <Termina/Core/Project.hpp>
#include <Termina/Core/Common.hpp>

#include <vector>

namespace Termina { class Actor; }

class ContentViewerPanel;

struct EditorContext
{
    Termina::IInspectable* ItemToInspect = nullptr;
    std::vector<Termina::Actor*> SelectedActors;

    std::vector<uint64> Clipboard;
    bool ClipboardIsCut = false;

    ContentViewerPanel* ContentViewer = nullptr;
    Termina::Project CurrentProject;
    float ViewportWidth  = 0.0f;
    float ViewportHeight = 0.0f;
    float LastDeltaTime  = 0.016f;
};
