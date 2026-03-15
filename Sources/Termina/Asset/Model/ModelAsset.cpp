#include <Termina/Asset/AssetSystem.hpp>

#include "ModelAsset.hpp"

#include <ImGui/imgui.h>

namespace Termina {

    ModelAsset::~ModelAsset()
    {
        delete BLASObject;
        delete VertexView;
        delete VertexBuffer;
        delete IndexBuffer;
    }

    void ModelAsset::Inspect()
    {
        ImGui::Text("Instances: %zu", Instances.size());
        ImGui::Text("Materials: %zu", Materials.size());

        if (!Instances.empty() && ImGui::CollapsingHeader("Instances"))
        {
            for (size_t i = 0; i < Instances.size(); ++i)
            {
                const MeshInstance& inst = Instances[i];
                ImGui::PushID((int)i);
                if (ImGui::TreeNode(inst.Name.empty() ? "(unnamed)" : inst.Name.c_str()))
                {
                    ImGui::Text("Material index: %u", inst.MaterialIndex);
                    ImGui::Text("Base vertex: %u", inst.BaseVertex);
                    if (!inst.LODs.empty())
                        ImGui::Text("Index count: %u", inst.LODs[0].IndexCount);
                    ImGui::Text("AABB min: (%.2f, %.2f, %.2f)", inst.Bounds.Min.x, inst.Bounds.Min.y, inst.Bounds.Min.z);
                    ImGui::Text("AABB max: (%.2f, %.2f, %.2f)", inst.Bounds.Max.x, inst.Bounds.Max.y, inst.Bounds.Max.z);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }
    }

} // namespace Termina
