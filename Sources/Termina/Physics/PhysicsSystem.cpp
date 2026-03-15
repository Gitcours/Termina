#include "PhysicsSystem.hpp"
#include "IPhysicsCallbacks.hpp"
#include "JoltDebugRenderer.hpp"
#include "Components/Rigidbody.hpp"
#include "Components/BoxCollider.hpp"
#include "Components/SphereCollider.hpp"
#include "Components/CapsuleCollider.hpp"
#include "Components/CylinderCollider.hpp"
#include "Components/PlaneCollider.hpp"
#include "Components/MeshCollider.hpp"
#include "Components/CharacterController.hpp"

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/PhysicsSettings.h>

#include <Termina/Core/Logger.hpp>
#include <Termina/World/Actor.hpp>
#include <Termina/World/ComponentRegistry.hpp>

#include <ImGui/imgui.h>

#include <thread>

namespace Termina {

    // -----------------------------------------------------------------------
    // Broad-phase layer interface
    // -----------------------------------------------------------------------
    namespace BroadPhaseLayers
    {
        static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
        static constexpr JPH::BroadPhaseLayer MOVING(1);
        static constexpr uint32 NUM_LAYERS(2);
    };

    class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
    {
    public:
        BPLayerInterfaceImpl()
        {
            mObjectToBroadPhase[PhysicsLayers::NON_MOVING]      = BroadPhaseLayers::NON_MOVING;
            mObjectToBroadPhase[PhysicsLayers::MOVING]          = BroadPhaseLayers::MOVING;
            mObjectToBroadPhase[PhysicsLayers::CHARACTER]       = BroadPhaseLayers::MOVING;
            mObjectToBroadPhase[PhysicsLayers::CHARACTER_GHOST] = BroadPhaseLayers::MOVING;
            mObjectToBroadPhase[PhysicsLayers::TRIGGER]         = BroadPhaseLayers::MOVING;
        }

        JPH::uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }

        JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
        {
            JPH_ASSERT(inLayer < PhysicsLayers::NUM_LAYERS);
            return mObjectToBroadPhase[inLayer];
        }

    private:
        JPH::BroadPhaseLayer mObjectToBroadPhase[PhysicsLayers::NUM_LAYERS];
    };

    class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
    {
    public:
        bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
        {
            switch (inLayer1)
            {
            case PhysicsLayers::NON_MOVING:      return inLayer2 == BroadPhaseLayers::MOVING;
            case PhysicsLayers::MOVING:          return true;
            case PhysicsLayers::CHARACTER:       return true;
            case PhysicsLayers::CHARACTER_GHOST: return true;
            case PhysicsLayers::TRIGGER:         return inLayer2 == BroadPhaseLayers::MOVING;
            default:                             return false;
            }
        }
    };

    class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
    {
    public:
        bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
        {
            switch (inObject1)
            {
            case PhysicsLayers::TRIGGER:
                return inObject2 == PhysicsLayers::CHARACTER_GHOST ||
                       inObject2 == PhysicsLayers::CHARACTER ||
                       inObject2 == PhysicsLayers::MOVING;
            case PhysicsLayers::NON_MOVING:
                return inObject2 == PhysicsLayers::MOVING ||
                       inObject2 == PhysicsLayers::CHARACTER_GHOST;
            case PhysicsLayers::MOVING:
                return inObject2 == PhysicsLayers::NON_MOVING ||
                       inObject2 == PhysicsLayers::MOVING ||
                       inObject2 == PhysicsLayers::TRIGGER;
            case PhysicsLayers::CHARACTER_GHOST:
                return inObject2 == PhysicsLayers::TRIGGER;
            case PhysicsLayers::CHARACTER:
                return inObject2 != PhysicsLayers::CHARACTER_GHOST &&
                       inObject2 != PhysicsLayers::CHARACTER;
            default:
                return false;
            }
        }
    };

    // -----------------------------------------------------------------------
    // Contact listener implementation
    // -----------------------------------------------------------------------
    class PhysicsSystem::ContactListenerImpl : public JPH::ContactListener
    {
    public:
        ContactListenerImpl(PhysicsSystem* owner) : m_Owner(owner) {}

        void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2,
                            const JPH::ContactManifold&, JPH::ContactSettings&) override
        {
            bool isTrigger = (inBody1.GetObjectLayer() == PhysicsLayers::TRIGGER ||
                              inBody2.GetObjectLayer() == PhysicsLayers::TRIGGER);
            std::lock_guard<std::mutex> lock(m_Owner->m_ContactMutex);
            m_Owner->m_PendingContacts.push_back({
                inBody1.GetID(), inBody2.GetID(), ContactEvent::Type::Enter, isTrigger
            });
        }

        void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2,
                                const JPH::ContactManifold&, JPH::ContactSettings&) override
        {
            bool isTrigger = (inBody1.GetObjectLayer() == PhysicsLayers::TRIGGER ||
                              inBody2.GetObjectLayer() == PhysicsLayers::TRIGGER);
            std::lock_guard<std::mutex> lock(m_Owner->m_ContactMutex);
            m_Owner->m_PendingContacts.push_back({
                inBody1.GetID(), inBody2.GetID(), ContactEvent::Type::Stay, isTrigger
            });
        }

        void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override
        {
            std::lock_guard<std::mutex> lock(m_Owner->m_ContactMutex);
            m_Owner->m_PendingContacts.push_back({
                inSubShapePair.GetBody1ID(), inSubShapePair.GetBody2ID(),
                ContactEvent::Type::Exit, false
            });
        }

    private:
        PhysicsSystem* m_Owner;
    };

    // -----------------------------------------------------------------------
    // Static filter objects (one per process)
    // -----------------------------------------------------------------------
    static BPLayerInterfaceImpl              s_BPLayerInterface;
    static ObjectVsBroadPhaseLayerFilterImpl s_ObjVsBP;
    static ObjectLayerPairFilterImpl         s_ObjPairFilter;

    // -----------------------------------------------------------------------
    // PhysicsSystem
    // -----------------------------------------------------------------------
    PhysicsSystem::PhysicsSystem()
    {
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();

        const uint32 maxBodies             = 4096;
        const uint32 numBodyMutexes        = 0;
        const uint32 maxBodyPairs          = 2048;
        const uint32 maxContactConstraints = 2048;
        const int    availableThreads      = std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1);

        m_System = new JPH::PhysicsSystem();
        m_System->Init(maxBodies, numBodyMutexes, maxBodyPairs, maxContactConstraints,
                       s_BPLayerInterface, s_ObjVsBP, s_ObjPairFilter);
        m_System->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

        m_BodyInterface = &m_System->GetBodyInterface();
        m_ThreadPool    = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, availableThreads);
        m_TempAllocator = new JPH::TempAllocatorMalloc();

        m_ContactListener = new ContactListenerImpl(this);
        m_System->SetContactListener(m_ContactListener);

        m_DebugRenderer = new JoltDebugRenderer();
        JPH::DebugRenderer::sInstance = m_DebugRenderer;
    }

    PhysicsSystem::~PhysicsSystem()
    {
        delete m_DebugRenderer;
        JPH::DebugRenderer::sInstance = nullptr;

        delete m_ContactListener;
        delete m_TempAllocator;
        delete m_ThreadPool;
        delete m_System;

        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    void PhysicsSystem::Physics(float /*deltaTime*/)
    {
        try {
            auto error = m_System->Update(PhysicsUpdateRate, 1, m_TempAllocator, m_ThreadPool);
            if (error != JPH::EPhysicsUpdateError::None)
            {
                switch (error)
                {
                case JPH::EPhysicsUpdateError::ManifoldCacheFull:
                    TN_ERROR("Jolt error: Manifold cache full"); break;
                case JPH::EPhysicsUpdateError::BodyPairCacheFull:
                    TN_ERROR("Jolt error: Body pair cache full"); break;
                case JPH::EPhysicsUpdateError::ContactConstraintsFull:
                    TN_ERROR("Jolt error: Contact constraints full"); break;
                default: break;
                }
            }
        } catch (const std::exception& e) {
            TN_ERROR("Jolt exception: %s", e.what());
        }
    }

    void PhysicsSystem::PostPhysics(float /*deltaTime*/)
    {
        // Drain contact events and dispatch to IPhysicsCallbacks components.
        std::vector<ContactEvent> events;
        {
            std::lock_guard<std::mutex> lock(m_ContactMutex);
            events.swap(m_PendingContacts);
        }

        for (const auto& evt : events)
        {
            Actor* actorA = GetActorForBody(evt.A);
            Actor* actorB = GetActorForBody(evt.B);
            if (!actorA || !actorB) continue;

            auto dispatch = [&](Actor* self, Actor* other)
            {
                for (auto* comp : self->GetAllComponents())
                {
                    if (auto* cb = dynamic_cast<IPhysicsCallbacks*>(comp))
                    {
                        switch (evt.type)
                        {
                        case ContactEvent::Type::Enter:
                            evt.isTrigger ? cb->OnTriggerEnter(other) : cb->OnCollisionEnter(other); break;
                        case ContactEvent::Type::Stay:
                            evt.isTrigger ? cb->OnTriggerStay(other)  : cb->OnCollisionStay(other);  break;
                        case ContactEvent::Type::Exit:
                            evt.isTrigger ? cb->OnTriggerExit(other)  : cb->OnCollisionExit(other);  break;
                        }
                    }
                }
            };

            dispatch(actorA, actorB);
            dispatch(actorB, actorA);
        }
    }

    void PhysicsSystem::PreRender(float /*deltaTime*/)
    {
        if (m_DebugDrawEnabled && m_DebugRenderer) {
            JPH::BodyManager::DrawSettings drawSettings = {};
            drawSettings.mDrawShape = true;
            drawSettings.mDrawShapeWireframe = true;

            m_System->DrawBodies(drawSettings, m_DebugRenderer);
        }
    }

    void PhysicsSystem::RegisterComponents()
    {
        auto& reg = ComponentRegistry::Get();
        reg.Register<BoxCollider>("Box Collider");
        reg.Register<SphereCollider>("Sphere Collider");
        reg.Register<CapsuleCollider>("Capsule Collider");
        reg.Register<CylinderCollider>("Cylinder Collider");
        reg.Register<PlaneCollider>("Plane Collider");
        reg.Register<MeshCollider>("Mesh Collider");
        reg.Register<Rigidbody>("Rigidbody");
        reg.Register<CharacterController>("Character Controller");
    }

    void PhysicsSystem::UnregisterComponents()
    {
        auto& reg = ComponentRegistry::Get();
        reg.UnregisterByName("Box Collider");
        reg.UnregisterByName("Sphere Collider");
        reg.UnregisterByName("Capsule Collider");
        reg.UnregisterByName("Cylinder Collider");
        reg.UnregisterByName("Plane Collider");
        reg.UnregisterByName("Mesh Collider");
        reg.UnregisterByName("Rigidbody");
        reg.UnregisterByName("Character Controller");
    }

    void PhysicsSystem::SetGravity(const glm::vec3& gravity)
    {
        m_System->SetGravity(JPH::Vec3(gravity.x, gravity.y, gravity.z));
    }

    RayResult PhysicsSystem::Raycast(const Ray& ray) const
    {
        JPH::RRayCast jRay(
            JPH::RVec3(ray.Origin.x, ray.Origin.y, ray.Origin.z),
            JPH::Vec3(ray.Direction.x * ray.MaxDistance,
                      ray.Direction.y * ray.MaxDistance,
                      ray.Direction.z * ray.MaxDistance)
        );

        JPH::RayCastResult result;
        if (!m_System->GetNarrowPhaseQuery().CastRay(jRay, result))
            return {};

        RayResult out;
        out.Hit      = true;
        out.T        = result.mFraction;
        JPH::RVec3 hitPos = jRay.mOrigin + result.mFraction * jRay.mDirection;
        out.Position = glm::vec3(hitPos.GetX(), hitPos.GetY(), hitPos.GetZ());
        out.HitActor = GetActorForBody(result.mBodyID);
        return out;
    }

    void PhysicsSystem::RegisterBody(JPH::BodyID id, Actor* actor)
    {
        m_BodyActorMap[id.GetIndex()] = actor;
    }

    void PhysicsSystem::UnregisterBody(JPH::BodyID id)
    {
        m_BodyActorMap.erase(id.GetIndex());
    }

    Actor* PhysicsSystem::GetActorForBody(JPH::BodyID id) const
    {
        auto it = m_BodyActorMap.find(id.GetIndex());
        return (it != m_BodyActorMap.end()) ? it->second : nullptr;
    }

    void PhysicsSystem::ShowDebugWindow(bool* open)
    {
        if (!ImGui::Begin("Physics System", open))
        {
            ImGui::End();
            return;
        }

        ImGui::Checkbox("Debug Draw", &m_DebugDrawEnabled);

        ImGui::End();
    }

} // namespace Termina
