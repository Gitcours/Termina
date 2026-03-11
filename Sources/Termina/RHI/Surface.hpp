#pragma once

#include "Texture.hpp"
#include "TextureView.hpp"
#include "RenderContext.hpp"

namespace Termina {
    constexpr int FRAMES_IN_FLIGHT = 3;

    class RendererSurface
    {
    public:
        RendererSurface() = default;
        virtual ~RendererSurface() = default;
    
        virtual RenderContext* GetContext() = 0;
        virtual RendererTexture* GetCurrentTexture() = 0;
        virtual TextureView* GetCurrentTextureView() = 0;
    
        virtual RenderContext* BeginFrame() = 0;
        virtual void EndFrame() = 0;
    
        virtual int GetFrameIndex() const = 0;
        virtual int GetWidth() const = 0;
        virtual int GetHeight() const = 0;
        virtual void Resize(int width, int height) = 0;
    };
}
