#include "RuntimeApplication.hpp"

#include <Termina/Renderer/Renderer.hpp>

RuntimeApplication::RuntimeApplication()
    : Application("Runtime")
{
    m_SystemManager.AddSystem<Termina::RendererSystem>(m_Window);
}
