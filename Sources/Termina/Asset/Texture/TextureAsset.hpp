#pragma once

#include "Core/IInspectable.hpp"

#include <Termina/RHI/Texture.hpp>

namespace Termina {

    /// A GPU texture loaded and managed by the asset system.
    class TextureAsset : public IInspectable
    {
    public:
        explicit TextureAsset(RendererTexture* texture)
            : m_Texture(texture)
        {
        }

        ~TextureAsset()
        {
            delete m_Texture;
        }

        RendererTexture* GetTexture() const { return m_Texture; }

        uint32 GetWidth()  const { return m_Texture->GetDesc().Width; }
        uint32 GetHeight() const { return m_Texture->GetDesc().Height; }

        void Inspect() override;

    private:
        RendererTexture* m_Texture = nullptr;
    };

} // namespace Termina
