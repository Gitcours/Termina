#pragma once

#include "Common.hpp"

#include <string>

namespace Termina {
    enum class UpdateFlags : uint32
    {
        UpdateDuringEditor = BIT(0),
        PhysicsUpdateDuringEditor = BIT(1),
        RenderUpdateDuringEditor = BIT(2),
    };
    ENUM_CLASS_FLAG_OPERATORS(UpdateFlags)

    class ISystem
    {
    public:
        virtual ~ISystem() = default;

        virtual void PreUpdate(float deltaTime) {};
        virtual void Update(float deltaTime) {};
        virtual void PostUpdate(float deltaTime) {};
        virtual void PrePhysics(float deltaTime) {};
        virtual void Physics(float deltaTime) {};
        virtual void PostPhysics(float deltaTime) {};
        virtual void PreRender(float deltaTime) {};
        virtual void Render(float deltaTime) {};
        virtual void PostRender(float deltaTime) {};

        virtual UpdateFlags GetUpdateFlags() const = 0;
        virtual std::string GetName() const = 0;
        virtual int GetPriority() const = 0;
    };
}
