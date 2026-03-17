#include "Transform.hpp"

#include <ImGui/imgui.h>
#include <Termina/World/Actor.hpp>
#include <Termina/World/ComponentRegistry.hpp>

#include <GLM/gtx/matrix_decompose.hpp>
#include <GLM/gtx/quaternion.hpp>
#include <GLM/gtx/euler_angles.hpp>

namespace Termina {
    Transform::Transform()
        : Component()
        , m_Position(0.0f, 0.0f, 0.0f)
        , m_Rotation(1.0f, 0.0f, 0.0f, 0.0f) // Identity quaternion
        , m_Scale(1.0f, 1.0f, 1.0f)
        , m_WorldMatrix(1.0f)
        , m_Dirty(true)
    {
    }

    Transform::Transform(Actor* owner)
        : Component(owner)
        , m_Position(0.0f, 0.0f, 0.0f)
        , m_Rotation(1.0f, 0.0f, 0.0f, 0.0f) // Identity quaternion
        , m_Scale(1.0f, 1.0f, 1.0f)
        , m_WorldMatrix(1.0f)
        , m_Dirty(true)
    {
    }

    // Position
    void Transform::SetPosition(const glm::vec3& position)
    {
        if (m_Owner && m_Owner->GetParent()) {
            Actor* parent = m_Owner->GetParent();
            if (parent->HasComponent<Transform>()) {
                Transform& parentTransform = parent->GetComponent<Transform>();
                glm::mat4 invParent = parentTransform.GetInverseWorldMatrix();
                glm::vec4 localPos = invParent * glm::vec4(position, 1.0f);
                m_Position = glm::vec3(localPos);
            } else {
                m_Position = position;
            }
        } else {
            m_Position = position;
        }
        MarkDirty();
    }

    glm::vec3 Transform::GetPosition() const
    {
        glm::mat4 world = GetWorldMatrix();
        return glm::vec3(world[3]);
    }

    void Transform::SetLocalPosition(const glm::vec3& position)
    {
        m_Position = position;
        MarkDirty();
    }

    glm::vec3 Transform::GetLocalPosition() const
    {
        return m_Position;
    }

    // Rotation
    void Transform::SetRotation(const glm::quat& rotation)
    {
        if (m_Owner && m_Owner->GetParent()) {
            Actor* parent = m_Owner->GetParent();
            if (parent->HasComponent<Transform>()) {
                Transform& parentTransform = parent->GetComponent<Transform>();
                glm::quat parentRot = parentTransform.GetRotation();
                m_Rotation = glm::inverse(parentRot) * glm::normalize(rotation);
            } else {
                m_Rotation = glm::normalize(rotation);
            }
        } else {
            m_Rotation = glm::normalize(rotation);
        }
        MarkDirty();
    }

    glm::quat Transform::GetRotation() const
    {
        if (m_Owner && m_Owner->GetParent()) {
            Actor* parent = m_Owner->GetParent();
            if (parent->HasComponent<Transform>()) {
                Transform& parentTransform = parent->GetComponent<Transform>();
                return parentTransform.GetRotation() * m_Rotation;
            }
        }
        return m_Rotation;
    }

    void Transform::SetLocalRotation(const glm::quat& rotation)
    {
        m_Rotation = glm::normalize(rotation);
        MarkDirty();
    }

    glm::quat Transform::GetLocalRotation() const
    {
        return m_Rotation;
    }

    // Euler angles
    void Transform::SetEulerAngles(const glm::vec3& eulerDegrees)
    {
        glm::vec3 radians = glm::radians(eulerDegrees);
        m_Rotation = glm::quat(radians);
        MarkDirty();
    }

    glm::vec3 Transform::GetEulerAngles() const
    {
        return glm::degrees(glm::eulerAngles(m_Rotation));
    }

    // Scale
    void Transform::SetScale(const glm::vec3& scale)
    {
        if (m_Owner && m_Owner->GetParent()) {
            Actor* parent = m_Owner->GetParent();
            if (parent->HasComponent<Transform>()) {
                Transform& parentTransform = parent->GetComponent<Transform>();
                glm::vec3 parentScale = parentTransform.GetScale();
                m_Scale = scale / parentScale;
            } else {
                m_Scale = scale;
            }
        } else {
            m_Scale = scale;
        }
        MarkDirty();
    }

    glm::vec3 Transform::GetScale() const
    {
        if (m_Owner && m_Owner->GetParent()) {
            Actor* parent = m_Owner->GetParent();
            if (parent->HasComponent<Transform>()) {
                Transform& parentTransform = parent->GetComponent<Transform>();
                return parentTransform.GetScale() * m_Scale;
            }
        }
        return m_Scale;
    }

    void Transform::SetLocalScale(const glm::vec3& scale)
    {
        m_Scale = scale;
        MarkDirty();
    }

    glm::vec3 Transform::GetLocalScale() const
    {
        return m_Scale;
    }

    // Transform matrices
    glm::mat4 Transform::GetLocalMatrix() const
    {
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), m_Position);
        glm::mat4 rotation = glm::toMat4(m_Rotation);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), m_Scale);
        return translation * rotation * scale;
    }

    glm::mat4 Transform::GetWorldMatrix() const
    {
        if (m_Dirty)
            UpdateWorldTransform();
        return m_WorldMatrix;
    }

    glm::mat4 Transform::GetInverseWorldMatrix() const
    {
        return glm::inverse(GetWorldMatrix());
    }

    // Direction vectors
    glm::vec3 Transform::GetForward() const
    {
        return glm::normalize(m_Rotation * glm::vec3(0.0f, 0.0f, -1.0f));
    }

    glm::vec3 Transform::GetRight() const
    {
        return glm::normalize(m_Rotation * glm::vec3(1.0f, 0.0f, 0.0f));
    }

    glm::vec3 Transform::GetUp() const
    {
        return glm::normalize(m_Rotation * glm::vec3(0.0f, 1.0f, 0.0f));
    }

    // Transform operations
    void Transform::Translate(const glm::vec3& delta)
    {
        m_Position += delta;
        MarkDirty();
    }

    void Transform::Rotate(const glm::quat& delta)
    {
        m_Rotation = glm::normalize(delta * m_Rotation);
        MarkDirty();
    }

    void Transform::RotateEuler(const glm::vec3& eulerDegrees)
    {
        glm::vec3 radians = glm::radians(eulerDegrees);
        glm::quat delta = glm::quat(radians);
        Rotate(delta);
    }

    void Transform::ScaleBy(const glm::vec3& scaleFactors)
    {
        m_Scale *= scaleFactors;
        MarkDirty();
    }

    // Look at
    void Transform::LookAt(const glm::vec3& target, const glm::vec3& up)
    {
        glm::vec3 direction = glm::normalize(target - m_Position);

        // Avoid degenerate case where direction and up are parallel
        if (glm::abs(glm::dot(direction, up)) > 0.9999f)
            return;

        glm::mat4 lookAtMatrix = glm::lookAt(m_Position, target, up);
        m_Rotation = glm::quat_cast(glm::inverse(lookAtMatrix));
        MarkDirty();
    }

    // Lifecycle
    void Transform::OnPreUpdate(float deltaTime)
    {
        if (m_Dirty)
            UpdateWorldTransform();
    }

    void Transform::OnUpdate(float deltaTime)
    {
        Component::OnUpdate(deltaTime);
    }

    void Transform::OnDetach(Actor* oldParent)
    {
        if (!oldParent || !oldParent->HasComponent<Transform>())
            return;

        glm::mat4 worldMatrix = oldParent->GetComponent<Transform>().GetWorldMatrix() * GetLocalMatrix();

        glm::vec3 skew;
        glm::vec4 perspective;
        glm::quat rotation;
        glm::decompose(worldMatrix, m_Scale, rotation, m_Position, skew, perspective);
        m_Rotation = glm::conjugate(rotation);
        MarkDirty();
    }

    void Transform::OnAttach(Actor* newParent)
    {
        if (!newParent || !newParent->HasComponent<Transform>())
            return;

        glm::mat4 localMatrix = newParent->GetComponent<Transform>().GetInverseWorldMatrix() * GetLocalMatrix();

        glm::vec3 skew;
        glm::vec4 perspective;
        glm::quat rotation;
        glm::decompose(localMatrix, m_Scale, rotation, m_Position, skew, perspective);
        m_Rotation = glm::conjugate(rotation);
        MarkDirty();
    }

    void Transform::Inspect()
    {
        const float resetButtonWidth = ImGui::CalcTextSize("R").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        const float fieldWidth = ImGui::GetContentRegionAvail().x - resetButtonWidth - ImGui::GetStyle().ItemSpacing.x;

        // Position
        glm::vec3 pos = GetLocalPosition();
        ImGui::SetNextItemWidth(fieldWidth);
        if (ImGui::DragFloat3("##pos", &pos.x, 0.1f))
            SetLocalPosition(pos);
        ImGui::SameLine();
        if (ImGui::SmallButton("R##pos"))
            SetLocalPosition(glm::vec3(0.0f));
        ImGui::SameLine();
        ImGui::TextUnformatted("Position");

        // Rotation — edit as euler degrees, store as quaternion
        glm::vec3 euler = GetEulerAngles();
        ImGui::SetNextItemWidth(fieldWidth);
        if (ImGui::DragFloat3("##rot", &euler.x, 0.5f))
            SetEulerAngles(euler);
        ImGui::SameLine();
        if (ImGui::SmallButton("R##rot"))
            SetEulerAngles(glm::vec3(0.0f));
        ImGui::SameLine();
        ImGui::TextUnformatted("Rotation");

        // Scale
        glm::vec3 scale = GetLocalScale();
        ImGui::SetNextItemWidth(fieldWidth);
        if (ImGui::DragFloat3("##scale", &scale.x, 0.01f))
            SetLocalScale(scale);
        ImGui::SameLine();
        if (ImGui::SmallButton("R##scale"))
            SetLocalScale(glm::vec3(1.0f));
        ImGui::SameLine();
        ImGui::TextUnformatted("Scale");
    }

    // Private helpers
    void Transform::MarkDirty()
    {
        m_Dirty = true;

        // Mark children as dirty too
        if (m_Owner) {
            for (Actor* child : m_Owner->GetChildren()) {
                if (child->HasComponent<Transform>())
                    child->GetComponent<Transform>().MarkDirty();
            }
        }
    }

    void Transform::Serialize(nlohmann::json& out) const
    {
        out["position"] = { m_Position.x, m_Position.y, m_Position.z };
        out["rotation"] = { m_Rotation.w, m_Rotation.x, m_Rotation.y, m_Rotation.z };
        out["scale"]    = { m_Scale.x,    m_Scale.y,    m_Scale.z    };
    }

    void Transform::Deserialize(const nlohmann::json& in)
    {
        if (in.contains("position")) {
            const auto& p = in["position"];
            m_Position = { p[0].get<float>(), p[1].get<float>(), p[2].get<float>() };
        }
        if (in.contains("rotation")) {
            const auto& r = in["rotation"];
            m_Rotation = glm::quat(r[0].get<float>(), r[1].get<float>(), r[2].get<float>(), r[3].get<float>());
        }
        if (in.contains("scale")) {
            const auto& s = in["scale"];
            m_Scale = { s[0].get<float>(), s[1].get<float>(), s[2].get<float>() };
        }
        MarkDirty();
    }

    void Transform::UpdateWorldTransform() const
    {
        if (m_Owner && m_Owner->GetParent()) {
            Actor* parent = m_Owner->GetParent();
            if (parent->HasComponent<Transform>()) {
                Transform& parentTransform = parent->GetComponent<Transform>();
                m_WorldMatrix = parentTransform.GetWorldMatrix() * GetLocalMatrix();
            } else {
                m_WorldMatrix = GetLocalMatrix();
            }
        } else {
            m_WorldMatrix = GetLocalMatrix();
        }

        m_Dirty = false;
    }
}
