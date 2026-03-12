#pragma once

#include <Termina/RHI/Device.hpp>
#include <Termina/RHI/RenderContext.hpp>
#include <Termina/World/World.hpp>

#include "GPUBumpAllocator.h"
#include "GPUUploader.h"
#include "Camera.hpp"
#include "ResourceViewCache.h"
#include "SamplerCache.h"
#include "PassIO.hpp"

namespace Termina {
    struct RenderPassExecuteInfo
    {
        RendererDevice* Device;
        RendererSurface* Surface;
        RenderContext* Ctx;
        
        GPUUploader* Uploader;
        GPUBumpAllocator* Allocator;
        ResourceViewCache* ViewCache;
        SamplerCache* SampCache;
        PassIO* IO;

        World* CurrentWorld;
        Camera CurrentCamera;

        uint FrameIndex;
        int32 Width;
        int32 Height;
    };

    class RenderPass
    {
    public:
        virtual ~RenderPass() = default;

        virtual void Resize(int32 width, int32 height) {};
        virtual void Execute(RenderPassExecuteInfo& Info) = 0;
    };
}
