#pragma once

#include <Termina/Core/System.h>
#include <Termina/Core/Window.hpp>
#include <Termina/RHI/Device.hpp>

namespace Termina {
    class RendererSystem : public ISystem
    {
    public:
        RendererSystem(Window* window);
        ~RendererSystem();

        void Render(float deltaTime) override;

        UpdateFlags GetUpdateFlags() const override {
            return UpdateFlags::RenderUpdateDuringEditor | UpdateFlags::UpdateDuringEditor;
        }
        std::string GetName() const override { return "Renderer System"; }
        int GetPriority() const override { return 0; }
    private:
        Window* m_Window;
	    RendererDevice* m_Device = nullptr;
	    RendererSurface* m_Surface = nullptr;
    };
}
