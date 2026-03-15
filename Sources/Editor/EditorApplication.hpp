#pragma once

#include <Termina/Core/Application.hpp>
#include <Termina/Audio/AudioSource.hpp>

#include "EditorContext.hpp"
#include "EditorCamera.hpp"
#include "Panel.hpp"

#include <memory>
#include <vector>

class EditorApplication : public Termina::Application
{
public:
    EditorApplication(const std::string& projectPath = "");
    ~EditorApplication();

    void OnUpdate(float dt) override;
    void OnPostRender(float dt) override;

    template<typename T>
    void RegisterPanel()
    {
        m_Panels.push_back(std::make_unique<T>(m_Context));
    }

private:
    void RenderDockspace();

    // Returns false if the user cancelled (e.g. dismissed the save dialog).
    bool SaveWorld(bool forceDialog = false);
    void OpenWorld();
    void NewWorld();

    EditorContext m_Context;
    EditorCamera  m_Camera;
    std::vector<std::unique_ptr<Panel>> m_Panels;

    struct DebugWindows
    {
        bool SystemManager = false;
        bool Input         = false;
        bool Renderer      = false;
        bool Scripts       = false;
        bool Shaders       = false;
        bool Assets        = false;
        bool Physics       = false;
        bool Log           = false;
    } m_DebugWindows;

    bool m_ShowAbout = false;
};
