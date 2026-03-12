#pragma once

#include <Termina/Scripting/API/ScriptingAPI.hpp>

using namespace TerminaScript;

class MySimpleComponent : public TerminaScript::ScriptableComponent
{
public:
    MySimpleComponent() = default;
    MySimpleComponent(Termina::Actor* owner) : TerminaScript::ScriptableComponent(owner) {}

    void Update(float deltaTime) override;
};
