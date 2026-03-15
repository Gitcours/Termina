#pragma once

#include <Termina/Core/IInspectable.hpp>
#include <Termina/RHI/Buffer.hpp>
#include <Termina/RHI/BufferView.hpp>
#include <Termina/RHI/BLAS.hpp>
#include <Termina/Asset/AssetHandle.hpp>
#include <Termina/Asset/Material/MaterialAsset.hpp>

#include <GLM/glm.hpp>
#include <limits>
#include <string>
#include <vector>

namespace Termina {

    struct ModelVertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 UV;
        glm::vec4 Tangent; // xyz + handedness in w
    };

    /// A single level-of-detail for a mesh instance.
    /// Add more LODs by appending entries; LODs[0] is the highest detail.
    struct MeshLOD
    {
        uint32 IndexOffset = 0;           // first index in the global index buffer
        uint32 IndexCount  = 0;
        float  ScreenSizeThreshold = 1.0f; // reserved for future LOD selection
    };

    /// Axis-aligned bounding box in world space (pre-computed at load time).
    struct AABB
    {
        glm::vec3 Min = glm::vec3( std::numeric_limits<float>::max());
        glm::vec3 Max = glm::vec3(-std::numeric_limits<float>::max());

        void Expand(const glm::vec3& p)
        {
            Min = glm::min(Min, p);
            Max = glm::max(Max, p);
        }

        glm::vec3 Center()  const { return (Min + Max) * 0.5f; }
        glm::vec3 Extents() const { return (Max - Min) * 0.5f; }
    };

    /// One GLTF node/primitive pair. References into the model's global buffers.
    struct MeshInstance
    {
        std::string           Name;
        uint32                MaterialIndex = 0; // index into ModelAsset::Materials
        uint32                BaseVertex    = 0; // vertex offset in global VB (passed as DrawIndexed vertexOffset)
        uint32                VertexCount   = 0; // vertex count for this primitive (used by BLAS build)
        std::vector<MeshLOD>  LODs;              // LODs[0] = highest detail
        glm::mat4             LocalTransform = glm::mat4(1.0f); // pre-baked GLTF node world matrix
        AABB                  Bounds;            // world-space AABB, ready for frustum culling
    };

    /// A loaded GLTF model: one global vertex/index buffer pair, a list of
    /// mesh instances, and a list of materials. Expandable to LODs and meshlets
    /// without changing the per-instance layout.
    class ModelAsset : public IInspectable
    {
    public:
        ~ModelAsset();
        void Inspect() override;

        std::vector<MeshInstance>               Instances;
        std::vector<AssetHandle<MaterialAsset>> Materials;

        RendererBuffer* VertexBuffer = nullptr; // BufferUsage::VERTEX | SHADER_READ
        RendererBuffer* IndexBuffer  = nullptr; // BufferUsage::INDEX
        BufferView*     VertexView   = nullptr; // bindless SRV index for shader access
        BLAS*           BLASObject   = nullptr; // null when RT not supported

        // CPU-side positions and indices retained for physics (MeshCollider).
        std::vector<glm::vec3> CpuPositions;
        std::vector<uint32>    CpuIndices;
    };

} // namespace Termina
