#include "TextureAsset.hpp"
#include "RHI/TextureView.hpp"

#include <Termina/Core/Application.hpp>
#include <Termina/Renderer/Renderer.hpp>
#include <ImGui/imgui.h>

namespace Termina {
    void TextureAsset::Inspect()
    {
        if (!GetTexture()) return;

        RendererSystem* renderer = Application::GetSystem<RendererSystem>();

        TextureViewDesc viewDesc = TextureViewDesc().CreateDefault(GetTexture(), TextureViewType::SHADER_READ, TextureViewDimension::TEXTURE_2D);
        TextureView* view = renderer->GetResourceViewCache()->GetTextureView(viewDesc);

        ImGui::Image(view->GetBindlessIndex(), ImVec2(256, 256));
        ImGui::Separator();
        ImGui::Text("Dimensions: %dx%d", GetWidth(), GetHeight());
    }
}
