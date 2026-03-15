#include "ModelLoader.hpp"
#include "Core/Logger.hpp"

#include <Termina/Core/Application.hpp>
#include <Termina/Asset/AssetSystem.hpp>
#include <Termina/Renderer/Renderer.hpp>
#include <Termina/RHI/Device.hpp>

#include <CGLTF/cgltf.h>
#include <MikkTSpace/mikktspace.h>
#include <GLM/glm.hpp>
#include <JSON/json.hpp>

#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

namespace fs = std::filesystem;

namespace Termina {

// ---------------------------------------------------------------------------
// MikkTSpace interface helpers
// ---------------------------------------------------------------------------

struct MikkContext
{
    std::vector<ModelVertex>& Vertices;
    const std::vector<uint32>& Indices;  // primitive-local indices
    uint32 FaceCount;
};

static int MikkGetNumFaces(const SMikkTSpaceContext* ctx)
{
    auto* data = static_cast<MikkContext*>(ctx->m_pUserData);
    return static_cast<int>(data->FaceCount);
}

static int MikkGetNumVerticesOfFace(const SMikkTSpaceContext* /*ctx*/, int /*face*/)
{
    return 3;
}

static void MikkGetPosition(const SMikkTSpaceContext* ctx, float pos[3], int face, int vert)
{
    auto* data = static_cast<MikkContext*>(ctx->m_pUserData);
    uint32 idx = data->Indices[face * 3 + vert];
    const glm::vec3& p = data->Vertices[idx].Position;
    pos[0] = p.x; pos[1] = p.y; pos[2] = p.z;
}

static void MikkGetNormal(const SMikkTSpaceContext* ctx, float norm[3], int face, int vert)
{
    auto* data = static_cast<MikkContext*>(ctx->m_pUserData);
    uint32 idx = data->Indices[face * 3 + vert];
    const glm::vec3& n = data->Vertices[idx].Normal;
    norm[0] = n.x; norm[1] = n.y; norm[2] = n.z;
}

static void MikkGetTexCoord(const SMikkTSpaceContext* ctx, float uv[2], int face, int vert)
{
    auto* data = static_cast<MikkContext*>(ctx->m_pUserData);
    uint32 idx = data->Indices[face * 3 + vert];
    const glm::vec2& t = data->Vertices[idx].UV;
    uv[0] = t.x; uv[1] = t.y;
}

static void MikkSetTSpaceBasic(const SMikkTSpaceContext* ctx, const float tangent[3], float sign, int face, int vert)
{
    auto* data = static_cast<MikkContext*>(ctx->m_pUserData);
    uint32 idx = data->Indices[face * 3 + vert];
    data->Vertices[idx].Tangent = glm::vec4(tangent[0], tangent[1], tangent[2], sign);
}

static void GenerateTangents(std::vector<ModelVertex>& vertices, const std::vector<uint32>& indices)
{
    MikkContext userData = { vertices, indices, static_cast<uint32>(indices.size() / 3) };

    SMikkTSpaceInterface iface = {};
    iface.m_getNumFaces          = MikkGetNumFaces;
    iface.m_getNumVerticesOfFace = MikkGetNumVerticesOfFace;
    iface.m_getPosition          = MikkGetPosition;
    iface.m_getNormal            = MikkGetNormal;
    iface.m_getTexCoord          = MikkGetTexCoord;
    iface.m_setTSpaceBasic       = MikkSetTSpaceBasic;

    SMikkTSpaceContext ctx = {};
    ctx.m_pInterface = &iface;
    ctx.m_pUserData  = &userData;

    genTangSpaceDefault(&ctx);
}

// ---------------------------------------------------------------------------
// CGLTF helpers
// ---------------------------------------------------------------------------

/// Read a float accessor element by index, returning a glm vector of size N.
template<int N>
static glm::vec<N, float> ReadFloat(const cgltf_accessor* acc, cgltf_size index)
{
    float buf[N] = {};
    cgltf_accessor_read_float(acc, index, buf, N);
    glm::vec<N, float> result;
    for (int i = 0; i < N; ++i) result[i] = buf[i];
    return result;
}

/// Read indices from an accessor into a vector of uint32.
static std::vector<uint32> ReadIndices(const cgltf_accessor* acc)
{
    std::vector<uint32> out(acc->count);
    for (cgltf_size i = 0; i < acc->count; ++i)
        out[i] = static_cast<uint32>(cgltf_accessor_read_index(acc, i));
    return out;
}

/// Convert a cgltf_node's world transform to a glm::mat4 (column-major, matches GLTF and GLM).
static glm::mat4 NodeWorldMatrix(const cgltf_node* node)
{
    float m[16];
    cgltf_node_transform_world(node, m);
    glm::mat4 result;
    memcpy(&result[0][0], m, sizeof(m));
    return result;
}

/// Find an attribute accessor by type in a GLTF primitive.
static const cgltf_accessor* FindAttribute(const cgltf_primitive& prim, cgltf_attribute_type type)
{
    for (cgltf_size i = 0; i < prim.attributes_count; ++i)
    {
        if (prim.attributes[i].type == type)
            return prim.attributes[i].data;
    }
    return nullptr;
}

/// Write a default .mat sidecar file. If the GLTF material has textures
/// embed their relative paths (resolved from the GLTF directory).
static void WriteSidecarMat(const std::string& matPath,
                             const cgltf_material* cgMat,
                             const std::string& gltfDir)
{
    std::string albedoPath, normalPath, ormPath, emissivePath;

    if (cgMat)
    {
        auto getTexturePath = [&](const cgltf_texture_view& view) -> std::string {
            if (view.texture && view.texture->image && view.texture->image->uri)
            {
                return (fs::path(gltfDir) / view.texture->image->uri).lexically_normal().string();
            }
            return "";
        };

        if (cgMat->has_pbr_metallic_roughness)
        {
            albedoPath = getTexturePath(cgMat->pbr_metallic_roughness.base_color_texture);
            ormPath    = getTexturePath(cgMat->pbr_metallic_roughness.metallic_roughness_texture);
        }

        normalPath   = getTexturePath(cgMat->normal_texture);
        emissivePath = getTexturePath(cgMat->emissive_texture);
    }

    nlohmann::json j;
    j["albedo_texture"]    = albedoPath;
    j["normal_texture"]    = normalPath;
    j["orm_texture"]       = ormPath;
    j["emissive_texture"]  = emissivePath;
    j["alpha_test"]        = false;
    j["color"]             = { 1.0f, 1.0f, 1.0f };
    j["override_metallic"] = false;
    j["override_roughness"]= false;
    j["metallic_factor"]   = (cgMat && cgMat->has_pbr_metallic_roughness)
                                 ? cgMat->pbr_metallic_roughness.metallic_factor  : 0.0f;
    j["roughness_factor"]  = (cgMat && cgMat->has_pbr_metallic_roughness)
                                 ? cgMat->pbr_metallic_roughness.roughness_factor : 0.5f;

    std::ofstream f(matPath);
    f << j.dump(2);
}

// ---------------------------------------------------------------------------
// ModelLoader
// ---------------------------------------------------------------------------

ModelAsset* ModelLoader::LoadFromDisk(const std::string& path)
{
    cgltf_options options = {};
    cgltf_data*   data    = nullptr;

    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success)
    {
        TN_ERROR("ModelLoader: failed to parse '%s'", path.c_str());
        return nullptr;
    }

    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success)
    {
        TN_ERROR("ModelLoader: failed to load buffers for '%s'", path.c_str());
        cgltf_free(data);
        return nullptr;
    }

    auto* asset   = Application::GetSystem<AssetSystem>();
    auto* device  = Application::GetSystem<RendererSystem>()->GetDevice();
    auto* uploader = Application::GetSystem<RendererSystem>()->GetGPUUploader();

    const std::string gltfDir  = fs::path(path).parent_path().string();
    const std::string baseName = fs::path(path).stem().string();

    // -----------------------------------------------------------------------
    // 1. Build sidecar .mat paths and load/create materials
    // -----------------------------------------------------------------------
    auto* modelAsset = new ModelAsset();

    for (cgltf_size mi = 0; mi < data->materials_count; ++mi)
    {
        std::string matPath = (fs::path(gltfDir) / (baseName + "_mat" + std::to_string(mi) + ".mat")).string();
        bool needsWrite = !fs::exists(matPath);
        if (!needsWrite)
        {
            try {
                std::ifstream chk(matPath);
                nlohmann::json chkJson;
                chk >> chkJson;
                if (!chkJson.contains("orm_texture"))
                    needsWrite = true;
            } catch (...) { needsWrite = true; }
        }
        if (needsWrite)
            WriteSidecarMat(matPath, &data->materials[mi], gltfDir);
        modelAsset->Materials.push_back(asset->Load<MaterialAsset>(matPath));
    }

    // If no materials defined, we'll use the default (index 0 will still reference the default).

    // -----------------------------------------------------------------------
    // 2. Walk GLTF scene nodes and collect geometry
    // -----------------------------------------------------------------------
    std::vector<ModelVertex> allVertices;
    std::vector<uint32>      allIndices;

    auto processNode = [&](auto& self, const cgltf_node* node) -> void
    {
        if (node->mesh)
        {
            glm::mat4 worldMat = NodeWorldMatrix(node);
            glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(worldMat)));

            for (cgltf_size pi = 0; pi < node->mesh->primitives_count; ++pi)
            {
                const cgltf_primitive& prim = node->mesh->primitives[pi];
                if (prim.type != cgltf_primitive_type_triangles) continue;

                const cgltf_accessor* posAcc  = FindAttribute(prim, cgltf_attribute_type_position);
                const cgltf_accessor* normAcc = FindAttribute(prim, cgltf_attribute_type_normal);
                const cgltf_accessor* uvAcc   = FindAttribute(prim, cgltf_attribute_type_texcoord);
                const cgltf_accessor* tanAcc  = FindAttribute(prim, cgltf_attribute_type_tangent);

                if (!posAcc)
                {
                    TN_WARN("ModelLoader: primitive missing POSITION, skipping");
                    continue;
                }

                const cgltf_size vertCount = posAcc->count;
                const uint32 baseVertex = static_cast<uint32>(allVertices.size());
                const uint32 firstIndex = static_cast<uint32>(allIndices.size());

                // Build per-primitive vertex array
                std::vector<ModelVertex> primVerts(vertCount);
                for (cgltf_size vi = 0; vi < vertCount; ++vi)
                {
                    glm::vec3 pos = ReadFloat<3>(posAcc, vi);
                    pos = glm::vec3(worldMat * glm::vec4(pos, 1.0f));
                    pos.y = -pos.y;
                    primVerts[vi].Position = pos;

                    if (normAcc)
                    {
                        glm::vec3 n = ReadFloat<3>(normAcc, vi);
                        primVerts[vi].Normal = glm::normalize(normalMat * n);
                    }
                    else
                    {
                        primVerts[vi].Normal = glm::vec3(0, 1, 0);
                    }

                    if (uvAcc)
                        primVerts[vi].UV = ReadFloat<2>(uvAcc, vi);

                    if (tanAcc)
                    {
                        glm::vec4 t = ReadFloat<4>(tanAcc, vi);
                        glm::vec3 tXYZ = glm::normalize(glm::mat3(worldMat) * glm::vec3(t));
                        primVerts[vi].Tangent = glm::vec4(tXYZ, t.w);
                    }
                }

                // Read primitive-local indices
                std::vector<uint32> primIndices;
                if (prim.indices)
                    primIndices = ReadIndices(prim.indices);
                else
                {
                    primIndices.resize(vertCount);
                    for (uint32 i = 0; i < vertCount; ++i) primIndices[i] = i;
                }

                // Generate tangents via MikkTSpace if not provided
                if (!tanAcc)
                    GenerateTangents(primVerts, primIndices);

                // Determine material index
                uint32 materialIndex = 0;
                if (prim.material)
                {
                    // Find the index of this material in data->materials
                    for (cgltf_size mi = 0; mi < data->materials_count; ++mi)
                    {
                        if (&data->materials[mi] == prim.material)
                        {
                            materialIndex = static_cast<uint32>(mi);
                            break;
                        }
                    }
                }

                // Append to global buffers and compute AABB
                AABB bounds;
                for (auto& v : primVerts)
                {
                    allVertices.push_back(v);
                    bounds.Expand(v.Position);
                }
                for (uint32 idx : primIndices) allIndices.push_back(idx);

                // Build instance
                MeshInstance inst;
                inst.Name          = node->name ? node->name : node->mesh->name ? node->mesh->name : "";
                if (node->mesh->primitives_count > 1)
                    inst.Name += "_" + std::to_string(pi);
                inst.MaterialIndex = materialIndex;
                inst.BaseVertex    = baseVertex;
                inst.VertexCount   = static_cast<uint32>(vertCount);
                inst.LocalTransform = glm::mat4(1.0f); // already baked into vertex data
                inst.Bounds        = bounds;
                inst.LODs.push_back({ firstIndex, static_cast<uint32>(primIndices.size()) });

                modelAsset->Instances.push_back(std::move(inst));
            }
        }

        for (cgltf_size ci = 0; ci < node->children_count; ++ci)
            self(self, node->children[ci]);
    };

    // Walk all scenes
    for (cgltf_size si = 0; si < data->scenes_count; ++si)
    {
        const cgltf_scene& scene = data->scenes[si];
        for (cgltf_size ni = 0; ni < scene.nodes_count; ++ni)
            processNode(processNode, scene.nodes[ni]);
    }

    cgltf_free(data);

    if (allVertices.empty())
    {
        TN_WARN("ModelLoader: '%s' produced no geometry", path.c_str());
        delete modelAsset;
        return nullptr;
    }

    // -----------------------------------------------------------------------
    // 3. Create and upload GPU buffers
    // -----------------------------------------------------------------------
    {
        BufferDesc vbDesc = BufferDesc()
            .SetSize(allVertices.size() * sizeof(ModelVertex))
            .SetStride(sizeof(ModelVertex))
            .SetUsage(BufferUsage::VERTEX | BufferUsage::SHADER_READ);
        modelAsset->VertexBuffer = device->CreateBuffer(vbDesc);
        modelAsset->VertexBuffer->SetName((baseName + " VB").c_str());

        uploader->QueueBufferUpload(modelAsset->VertexBuffer,
                                    allVertices.data(),
                                    allVertices.size() * sizeof(ModelVertex));
    }

    {
        BufferDesc ibDesc = BufferDesc()
            .SetSize(allIndices.size() * sizeof(uint32))
            .SetStride(sizeof(uint32))
            .SetUsage(BufferUsage::INDEX);
        modelAsset->IndexBuffer = device->CreateBuffer(ibDesc);
        modelAsset->IndexBuffer->SetName((baseName + " IB").c_str());

        uploader->QueueBufferUpload(modelAsset->IndexBuffer,
                                    allIndices.data(),
                                    allIndices.size() * sizeof(uint32));
    }

    // Flush synchronously so the buffers are ready before the next frame.
    uploader->Flush();

    // Store CPU-side positions and indices for physics mesh colliders.
    modelAsset->CpuPositions.reserve(allVertices.size());
    for (auto& v : allVertices)
        modelAsset->CpuPositions.push_back(v.Position);
    modelAsset->CpuIndices = allIndices;

    // -----------------------------------------------------------------------
    // 4. Create bindless vertex buffer view
    // -----------------------------------------------------------------------
    modelAsset->VertexView = device->CreateBufferView(
        BufferViewDesc().SetBuffer(modelAsset->VertexBuffer).SetType(BufferViewType::SHADER_READ));

    // -----------------------------------------------------------------------
    // 5. Build BLAS for ray tracing (if supported)
    // -----------------------------------------------------------------------
    if (device->SupportsRaytracing() && !modelAsset->Instances.empty())
    {
        BLASDesc blasDesc;
        blasDesc.VertexBuffer = modelAsset->VertexBuffer;
        blasDesc.IndexBuffer  = modelAsset->IndexBuffer;

        for (const MeshInstance& inst : modelAsset->Instances)
        {
            if (inst.LODs.empty()) continue;
            BLASGeometry geo;
            geo.VertexOffset = inst.BaseVertex;
            geo.VertexCount  = inst.VertexCount;
            geo.IndexOffset  = inst.LODs[0].IndexOffset;
            geo.IndexCount   = inst.LODs[0].IndexCount;
            geo.Opaque       = true;
            blasDesc.Geometries.push_back(geo);
        }

        modelAsset->BLASObject = device->CreateBLAS(blasDesc);
    }

    TN_INFO("ModelLoader: loaded '%s' - %zu instances, %zu vertices, %zu indices",
           path.c_str(), modelAsset->Instances.size(), allVertices.size(), allIndices.size());

    return modelAsset;
}

ModelAsset* ModelLoader::LoadFromPackage(const uint8* /*data*/, size_t /*size*/)
{
    return nullptr;
}

std::vector<uint8> ModelLoader::ExportToPackage(const ModelAsset* /*asset*/)
{
    return {};
}

} // namespace Termina
