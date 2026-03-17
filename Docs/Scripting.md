# Scripting

Scripting in Termina is done in **C++**. You write components — small classes that attach to actors in your scene and run game logic. The engine hot-reloads your code from a shared library (`GameAssembly`), so you can iterate quickly without restarting.

---

## How it works

Every component lives in `Sources/GameAssembly/`. When you build that target, the engine picks up your new code automatically.

The entry point is `GameAssembly.cpp`. Every component you create must be registered there:

```cpp
COMPONENT_MODULE_BEGIN()
    REGISTER_COMPONENT(MyComponent, "My Component")
COMPONENT_MODULE_END()
```

---

## Creating a component

### 1. Write the header

Create `MyComponent.hpp`:

```cpp
#pragma once
#include <Termina/Scripting/API/ScriptingAPI.hpp>

using namespace TerminaScript;

class MyComponent : public TerminaScript::ScriptableComponent
{
public:
    MyComponent() = default;
    MyComponent(Termina::Actor* owner) : TerminaScript::ScriptableComponent(owner) {}

    void Start()  override;
    void Update(float deltaTime) override;
};
```

### 2. Write the source

Create `MyComponent.cpp`:

```cpp
#include "MyComponent.hpp"

void MyComponent::Start()
{
    // Called once when the scene starts playing.
}

void MyComponent::Update(float deltaTime)
{
    // Called every frame. deltaTime is seconds since last frame.
}
```

### 3. Register it

In `GameAssembly.cpp`, add:

```cpp
#include "MyComponent.hpp"

COMPONENT_MODULE_BEGIN()
    REGISTER_COMPONENT(MyComponent, "My Component")
    // ... existing components
COMPONENT_MODULE_END()
```

Build the project — your component will appear in the editor's component list.

---

## Lifecycle methods

Override whichever ones you need:

| Method | When it runs |
|--------|-------------|
| `Awake()` | Before `Start()`, before the play-mode snapshot is taken. Use for one-time initialization that must happen early (e.g. seeding an RNG). |
| `Start()` | Once, when the scene begins playing. |
| `Update(float deltaTime)` | Every frame while playing. |
| `Stop()` | When the scene stops playing. Clean up anything you created in `Start()`. |
| `OnCollisionEnter(Actor* other)` | When this actor's collider first touches another. |

---

## Reading input

```cpp
#include <Termina/Scripting/API/ScriptingAPI.hpp>

// Keyboard
if (Input::IsKeyHeld(Termina::Key::W))    { /* moving forward */ }
if (Input::IsKeyHeld(Termina::Key::LeftShift)) { /* sprinting */ }

// Mouse buttons
if (Input::IsMouseButtonHeld(Termina::MouseButton::Right)) { /* right-click held */ }

// Mouse movement
glm::vec2 delta = Input::GetMouseDelta();

// Cursor control
Input::SetCursorVisible(false);
Input::SetCursorLocked(true);
```

---

## Moving things with Transform

Every actor has a `Transform` accessible via `m_Transform`:

```cpp
void MyComponent::Update(float deltaTime)
{
    glm::vec3 pos = m_Transform->GetPosition();
    pos.x += 1.0f * deltaTime;
    m_Transform->SetPosition(pos);

    // Direction vectors
    glm::vec3 forward = m_Transform->GetForward();
    glm::vec3 right   = m_Transform->GetRight();
    glm::vec3 up      = m_Transform->GetUp();

    // Rotation (as a quaternion)
    glm::quat rot = glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0));
    m_Transform->SetRotation(rot);

    // Scale
    m_Transform->SetScale(glm::vec3(2.0f));
}
```

---

## Spawning and destroying actors

```cpp
// Spawn a blank actor
Termina::Actor* obj = Instantiate();

// Spawn from a prefab
Termina::Actor* obj = Instantiate(myPrefab);

// Destroy an actor
Destroy(obj);
```

Actors start with a `Transform` component. Add others with `AddComponent`:

```cpp
auto& rb = obj->AddComponent<Termina::Rigidbody>();
rb.Type = Termina::Rigidbody::BodyType::Dynamic;
rb.Mass = 1.0f;

auto& col = obj->AddComponent<Termina::BoxCollider>();
col.HalfExtents = glm::vec3(0.5f);
```

Retrieve existing components with `GetComponent`:

```cpp
auto& tr = obj->GetComponent<Termina::Transform>();
tr.SetPosition(glm::vec3(0, 5, 0));
```

---

## Physics components

| Component | Purpose |
|-----------|---------|
| `Termina::Rigidbody` | Gives an actor physics simulation. Set `Type` to `Static` or `Dynamic`. |
| `Termina::BoxCollider` | Box-shaped collider. Configure `HalfExtents`. |
| `Termina::SphereCollider` | Sphere collider. Configure `Radius`. |
| `Termina::CapsuleCollider` | Capsule collider. Configure `Radius` and `HalfHeight`. |
| `Termina::CylinderCollider` | Cylinder collider. Configure `Radius` and `HalfHeight`. |

---

## Inspector UI (Inspect)

Override `Inspect()` to expose fields in the editor panel using ImGui:

```cpp
#include <ImGui/imgui.h>

void MyComponent::Inspect()
{
    ImGui::DragFloat("Speed",  &m_Speed,  0.1f);
    ImGui::DragInt  ("Count",  &m_Count,  1, 0, 100);
    ImGui::DragFloat3("Gravity", &m_Gravity.x, 0.1f);
}
```

---

## Saving and loading (Serialize / Deserialize)

Override these to persist your component's state with the scene:

```cpp
void MyComponent::Serialize(nlohmann::json& out) const
{
    out["speed"] = m_Speed;
    out["count"] = m_Count;
}

void MyComponent::Deserialize(const nlohmann::json& in)
{
    if (in.contains("speed")) m_Speed = in["speed"];
    if (in.contains("count")) m_Count = in["count"];
}
```

Always guard with `contains()` — fields may be missing in older save files.

---

## Logging

```cpp
#include <Termina/Core/Logger.hpp>

TN_INFO("Spawned %d objects", count);
```

---

## Real examples

The three components in `Sources/GameAssembly/` are good references:

- [FlyCamComponent](../Sources/GameAssembly/FlyCamComponent.hpp) — keyboard + mouse camera using `Input` and `m_Transform`.
- [PhysicsTestComponent](../Sources/GameAssembly/PhysicsTestComponent.hpp) — spawns actors with colliders and rigidbodies in `Start()`, uses `OnCollisionEnter`, and exposes settings via `Inspect()`.
- [ParticleSystem](../Sources/GameAssembly/ParticleSystem.hpp) — object-pool pattern across `Awake`, `Start`, `Update`, and `Stop`, with a `Prefab` field and full serialization.
