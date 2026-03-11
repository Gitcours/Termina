#include "Renderer.hpp"
#include "RHI/RenderContext.hpp"

namespace Termina {
    RendererSystem::RendererSystem(Window* window)
        : m_Window(window)
    {
        m_Device = RendererDevice::Create();
        m_Surface = m_Device->CreateSurface(window);
    }

    RendererSystem::~RendererSystem()
    {
        m_Device->WaitIdle();

        delete m_Surface;
        delete m_Device;
    }

    void RendererSystem::Render(float deltaTime)
    {
        uint32 frameIndex = m_Surface->GetFrameIndex();

        RenderContext* context = m_Surface->BeginFrame();

        RenderEncoderInfo rei = RenderEncoderInfo().AddColorAttachment(m_Surface->GetCurrentTextureView(), true, glm::vec4(0.2f, 0.8f, 0.5f, 1.0f))
                                                   .SetName("Clear Color")
                                                   .SetDimensions(m_Window->GetWidth(), m_Window->GetHeight());
        RenderEncoder* re = context->CreateRenderEncoder(rei);
        re->End();

        m_Surface->EndFrame();
    }
}
