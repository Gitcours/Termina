#include "CubesPass.hpp"
#include "Asset/AssetHandle.hpp"
#include "Asset/AssetSystem.hpp"
#include "Asset/Texture/TextureAsset.hpp"
#include "RHI/Device.hpp"
#include "RHI/RenderContext.hpp"
#include "RHI/TextureView.hpp"
#include "Renderer/Components/CameraComponent.hpp"

#include <Termina/Core/Application.hpp>
#include <Termina/Renderer/Renderer.hpp>
#include <Termina/Shader/ShaderManager.hpp>
#include <Termina/Shader/ShaderServer.hpp>

namespace Termina {
    CubesPass::CubesPass()
    {
        RendererDevice* device = Application::GetSystem<RendererSystem>()->GetDevice();
        uint32 width = Application::Get().GetWindow()->GetPixelWidth();
        uint32 height = Application::Get().GetWindow()->GetPixelHeight();

        TextureDesc desc = TextureDesc().SetSize(width, height)
                                        .SetFormat(TextureFormat::RGBA8_UNORM)
                                        .SetUsage(TextureUsage::RENDER_TARGET | TextureUsage::SHADER_READ | TextureUsage::SHADER_WRITE);
        m_ColorTexture = device->CreateTexture(desc);
        m_ColorTexture->SetName("Cubes Color Texture");

        desc.SetFormat(TextureFormat::D32_FLOAT);
        desc.SetUsage(TextureUsage::DEPTH_TARGET | TextureUsage::SHADER_READ);
        m_DepthTexture = device->CreateTexture(desc);
        m_DepthTexture->SetName("Cubes Depth Texture");

        // Load texture
        AssetSystem* assetSystem = Application::GetSystem<AssetSystem>();
        m_TextureHandle = assetSystem->Load<TextureAsset>("Assets/Textures/TestTexture.png");

        // Create pipeline
        ShaderServer& server = Application::GetSystem<ShaderManager>()->GetShaderServer();

        RenderPipelineDesc rpDesc = RenderPipelineDesc().AddColorAttachmentFormat(TextureFormat::RGBA8_UNORM)
                                                        .SetName("Cubes Pipeline")
                                                        .SetCullMode(PipelineCullMode::NONE)
                                                        .SetDepthAttachmentFormat(TextureFormat::D32_FLOAT)
                                                        .SetDepthCompareOp(PipelineCompareOp::LESS)
                                                        .SetDepthReadEnabled(true)
                                                        .SetDepthWriteEnabled(true);
        server.WatchPipeline("Assets/Shaders/Cubes.hlsl", rpDesc, PipelineType::Graphics);
    }

    CubesPass::~CubesPass()
    {
        delete m_ColorTexture;
        delete m_DepthTexture;
    }

    void CubesPass::Resize(int32 width, int32 height)
    {
        m_ColorTexture->Resize(width, height);
        m_DepthTexture->Resize(width, height);
    }

    void CubesPass::Execute(RenderPassExecuteInfo& info)
    {

        ShaderServer& server = Application::GetSystem<ShaderManager>()->GetShaderServer();

        TextureView* colorView = info.ViewCache->GetTextureView(TextureViewDesc().SetTexture(m_ColorTexture)
                                                                                 .SetType(TextureViewType::RENDER_TARGET));
        TextureView* depthView = info.ViewCache->GetTextureView(TextureViewDesc().SetTexture(m_DepthTexture)
                                                                                 .SetType(TextureViewType::DEPTH_TARGET));

        RenderEncoderInfo rei = RenderEncoderInfo().AddColorAttachment(colorView, true, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))
                                                   .SetDepthAttachment(depthView)
                                                   .SetName("Cubes Pass")
                                                   .SetDimensions(info.Width, info.Height);
        RenderEncoder* re = info.Ctx->CreateRenderEncoder(rei);
        re->SetPipeline(server.GetPipeline("Assets/Shaders/Cubes.hlsl"));
        re->SetViewport(0.0f, 0.0f, static_cast<float>(info.Width), static_cast<float>(info.Height));
        re->SetScissorRect(0.0f, 0.0f, static_cast<float>(info.Width), static_cast<float>(info.Height));
        for (auto& entity : info.CurrentWorld->GetActors()) {
            if (entity->HasComponent<CameraComponent>()) continue;

            struct Constants {
                glm::mat4 MVP;

                int TextureIndex;
                int SamplerIndex;
            } constants = {
                .MVP = info.CurrentCamera.ViewProjection * entity->GetComponent<Transform>().GetWorldMatrix(),
                .TextureIndex = info.ViewCache->GetTextureView(TextureViewDesc().CreateDefault(m_TextureHandle->GetTexture(), TextureViewType::SHADER_READ, TextureViewDimension::TEXTURE_2D))->GetBindlessIndex(),
                .SamplerIndex = info.SampCache->GetSampler(SamplerDesc())->GetBindlessHandle(),
            };
            re->SetConstants(sizeof(constants), &constants);
            re->Draw(36, 1, 0, 0);
        }
        re->End();

        info.IO->RegisterTexture("RendererOutput", m_ColorTexture);

        // Issue barrier to SRV
        TextureBarrier toSRV = TextureBarrier().SetTargetTexture(m_ColorTexture)
                                               .SetNewLayout(TextureLayout::READ_ONLY)
                                               .SetDstAccess(ResourceAccess::SHADER_READ)
                                               .SetDstStage(PipelineStage::PIXEL_SHADER);
        info.Ctx->Barrier(toSRV);
    }
}
