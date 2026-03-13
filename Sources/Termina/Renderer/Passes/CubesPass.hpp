#pragma once

#include <Termina/Renderer/RenderPass.hpp>
#include <Termina/Asset/AssetHandle.hpp>
#include <Termina/Asset/AssetSystem.hpp>
#include <Termina/Asset/Texture/TextureAsset.hpp>


namespace Termina {
    /// Renders cubes using the given render pass execute info.
    class CubesPass : public RenderPass
    {
    public:
        CubesPass();
        ~CubesPass() override;

        void Resize(int32 width, int32 height) override;
        void Execute(RenderPassExecuteInfo& Info) override;
    private:
        RendererTexture* m_ColorTexture;
        RendererTexture* m_DepthTexture;

        AssetHandle<TextureAsset> m_TextureHandle;
    };
}
