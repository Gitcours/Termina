#include "Application.hpp"

namespace Termina {
    Application* Application::s_Instance = nullptr;

    Application::Application(const std::string& gameName)
    {
        s_Instance = this;
        m_Window = new Window(1280, 720, ("TERMINA ENGINE | " + gameName).c_str());
    }

    Application::~Application()
    {
        m_SystemManager.Clean();
        delete m_Window;
    }

    void Application::Run()
    {
        while (m_Running && m_Window->IsOpen()) {
            float currentTime = static_cast<float>(glfwGetTime());
            float dt = currentTime - m_LastFrameTime;
            m_LastFrameTime = currentTime;

            m_SystemManager.Begin();

            PreUpdate(dt);
            Update(dt);
            PostUpdate(dt);

            PrePhysics(dt);
            Physics(dt);
            PostPhysics(dt);

            PreRender(dt);
            Render(dt);
            PostRender(dt);

            m_Window->Update();
        }
    }

    void Application::PreUpdate(float dt)
    {
        m_SystemManager.PreUpdate(dt);
        OnPreUpdate(dt);
    }

    void Application::Update(float dt)
    {
        m_SystemManager.Update(dt);
        OnUpdate(dt);
    }

    void Application::PostUpdate(float dt)
    {
        m_SystemManager.PostUpdate(dt);
        OnPostUpdate(dt);
    }

    void Application::PrePhysics(float dt)
    {
        m_SystemManager.PrePhysics(dt);
        OnPrePhysics(dt);
    }

    void Application::Physics(float dt)
    {
        m_SystemManager.Physics(dt);
        OnPhysics(dt);
    }

    void Application::PostPhysics(float dt)
    {
        m_SystemManager.PostPhysics(dt);
        OnPostPhysics(dt);
    }

    void Application::PreRender(float dt)
    {
        m_SystemManager.PreRender(dt);
        OnPreRender(dt);
    }

    void Application::Render(float dt)
    {
        m_SystemManager.Render(dt);
        OnRender(dt);
    }

    void Application::PostRender(float dt)
    {
        m_SystemManager.PostRender(dt);
        OnPostRender(dt);
    }
}
