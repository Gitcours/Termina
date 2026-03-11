#pragma once

#include <Termina/Core/Window.hpp>
#include <Termina/RHI/Device.hpp>

namespace Termina {
    class Renderer
    {
    public:
        Renderer(Window* window);
        ~Renderer();

        void Render();
    private:
        Window* m_Window;
	    RendererDevice* m_Device = nullptr;
	    RendererSurface* m_Surface = nullptr;
    };
}
