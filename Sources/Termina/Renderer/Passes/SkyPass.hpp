#pragma once

#include <Termina/Renderer/RenderPass.hpp>

namespace Termina {

    /// Compute pass: Procedural Sky
    class SkyPass : public RenderPass
    {
    public:
        SkyPass();
        ~SkyPass() override;

        void Resize(int32 width, int32 height) override;
        void Execute(RenderPassExecuteInfo& info) override;
    };

} // namespace Termina