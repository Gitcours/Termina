#include "PhysicsTestComponent.hpp"
#include "Termina/Scripting/API/ScriptDebug.hpp"

#include <Termina/Physics/Components/Rigidbody.hpp>
#include <Termina/Physics/Components/BoxCollider.hpp>
#include <Termina/Physics/Components/SphereCollider.hpp>
#include <Termina/Physics/Components/CapsuleCollider.hpp>
#include <Termina/Physics/Components/CylinderCollider.hpp>
#include <Termina/World/Components/Transform.hpp>
#include <Termina/Core/Logger.hpp>

#include <GLM/gtc/constants.hpp>

#include <cstdlib>
#include <ImGui/imgui.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

Termina::Actor* PhysicsTestComponent::MakeFunnelPanel(const glm::vec3& pos,
                                                      const glm::vec3& eulerDeg,
                                                      const glm::vec3& halfExtents)
{
    Termina::Actor* panel = Instantiate();

    auto& tr = panel->GetComponent<Termina::Transform>();
    tr.SetPosition(pos);
    glm::quat rot = glm::quat(glm::radians(eulerDeg));
    tr.SetRotation(rot);

    auto& col = panel->AddComponent<Termina::BoxCollider>();
    col.HalfExtents = halfExtents;

    auto& rb = panel->AddComponent<Termina::Rigidbody>();
    rb.Type = Termina::Rigidbody::BodyType::Static;

    return panel;
}

// ---------------------------------------------------------------------------
// Start
// ---------------------------------------------------------------------------

void PhysicsTestComponent::Start()
{
    SpawnFunnel();
    SpawnObjects();
}

void PhysicsTestComponent::SpawnFunnel()
{
    // 4 angled side panels forming a square funnel, plus a thin floor ring
    // (represented as 4 floor strips with a gap in the center).

    struct PanelDesc { glm::vec3 pos; glm::vec3 eulerDeg; glm::vec3 halfExtents; };

    // Side panels tilted inward to form a regular funnel ~35 degrees
    const PanelDesc panels[] =
    {
        {{ 0.0f,  1.0f,  1.6f}, {-35.0f,  0.0f,  0.0f}, {1.5f, 0.05f, 1.5f}},  // front
        {{ 0.0f,  1.0f, -1.6f}, { 35.0f,  0.0f,  0.0f}, {1.5f, 0.05f, 1.5f}},  // back
        {{ 1.6f,  1.0f,  0.0f}, {  0.0f,  0.0f,  35.0f},{1.5f, 0.05f, 1.5f}},  // right
        {{-1.6f,  1.0f,  0.0f}, {  0.0f,  0.0f, -35.0f},{1.5f, 0.05f, 1.5f}},  // left
    };

    for (auto& p : panels)
        MakeFunnelPanel(p.pos, p.eulerDeg, p.halfExtents);

    // Floor strips around a central gap (4 strips, leaves ~0.4 m hole in middle)
    const PanelDesc floors[] =
    {
        {{ 1.0f, -0.5f,  0.0f}, {0,0,0}, {0.6f, 0.05f, 1.5f}},
        {{-1.0f, -0.5f,  0.0f}, {0,0,0}, {0.6f, 0.05f, 1.5f}},
        {{ 0.0f, -0.5f,  1.0f}, {0,0,0}, {1.5f, 0.05f, 0.6f}},
        {{ 0.0f, -0.5f, -1.0f}, {0,0,0}, {1.5f, 0.05f, 0.6f}},
    };

    for (auto& f : floors)
        MakeFunnelPanel(f.pos, f.eulerDeg, f.halfExtents);
}

void PhysicsTestComponent::SpawnObjects()
{
    for (int i = 0; i < SpawnCount; ++i)
    {
        // Random offset within ±1 m in XZ, staggered heights
        float rx = ((float)rand() / (float)RAND_MAX - 0.5f) * 2.0f;
        float rz = ((float)rand() / (float)RAND_MAX - 0.5f) * 2.0f;
        float ry = SpawnHeight + ((float)rand() / (float)RAND_MAX) * 3.0f;

        Termina::Actor* obj = Instantiate();

        auto& tr = obj->GetComponent<Termina::Transform>();
        tr.SetPosition(glm::vec3(rx, ry, rz));

        int shape = i % 4;
        switch (shape)
        {
            case 0:
            {
                auto& col = obj->AddComponent<Termina::BoxCollider>();
                col.HalfExtents = glm::vec3(0.2f);
                break;
            }
            case 1:
            {
                auto& col = obj->AddComponent<Termina::SphereCollider>();
                col.Radius = 0.2f;
                break;
            }
            case 2:
            {
                auto& col = obj->AddComponent<Termina::CapsuleCollider>();
                col.HalfHeight = 0.2f;
                col.Radius     = 0.1f;
                break;
            }
            case 3:
            {
                auto& col = obj->AddComponent<Termina::CylinderCollider>();
                col.HalfHeight = 0.2f;
                col.Radius     = 0.1f;
                break;
            }
        }

        auto& rb = obj->AddComponent<Termina::Rigidbody>();
        rb.Type = Termina::Rigidbody::BodyType::Dynamic;
        rb.Mass = 1.0f;
    }

    TN_INFO("PhysicsTestComponent: spawned funnel + %d dynamic bodies", SpawnCount);
}

void PhysicsTestComponent::OnCollisionEnter(Termina::Actor* other)
{
    (void)other; // collisions logged — extend as needed
}

void PhysicsTestComponent::Update(float dt)
{
}

void PhysicsTestComponent::Inspect()
{
    ImGui::DragInt("Spawn Count", &SpawnCount, 1, 1, 1000);
    ImGui::DragFloat("Spawn Height", &SpawnHeight, 0.1f);
}

void PhysicsTestComponent::Serialize(nlohmann::json& out) const
{
    out["spawn_count"]  = SpawnCount;
    out["spawn_height"] = SpawnHeight;
}

void PhysicsTestComponent::Deserialize(const nlohmann::json& in)
{
    if (in.contains("spawn_count"))  SpawnCount  = in["spawn_count"];
    if (in.contains("spawn_height")) SpawnHeight = in["spawn_height"];
}
