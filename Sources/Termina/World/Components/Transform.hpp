#pragma once

#include <Termina/World/Component.hpp>

#include <GLM/glm.hpp>
#include <GLM/gtc/quaternion.hpp>
#include <GLM/gtc/matrix_transform.hpp>

namespace Termina {
    /// Represents the transform (position, rotation, scale) of an actor.
    class Transform : public Component
    {
    public:
        Transform();
        Transform(Actor* owner);
        ~Transform() override = default;

        // Position
        void SetPosition(const glm::vec3& position);
        glm::vec3 GetPosition() const;

        void SetLocalPosition(const glm::vec3& position);
        glm::vec3 GetLocalPosition() const;

        // Rotation (using quaternions for smooth interpolation and no gimbal lock)
        void SetRotation(const glm::quat& rotation);
        glm::quat GetRotation() const;

        void SetLocalRotation(const glm::quat& rotation);
        glm::quat GetLocalRotation() const;

        // Euler angles helper (in degrees)
        void SetEulerAngles(const glm::vec3& eulerDegrees);
        glm::vec3 GetEulerAngles() const;

        // Scale
        void SetScale(const glm::vec3& scale);
        glm::vec3 GetScale() const;

        void SetLocalScale(const glm::vec3& scale);
        glm::vec3 GetLocalScale() const;

        // Transform matrices
        glm::mat4 GetLocalMatrix() const;
        glm::mat4 GetWorldMatrix() const;
        glm::mat4 GetInverseWorldMatrix() const;

        // Direction vectors (world space)
        glm::vec3 GetForward() const;
        glm::vec3 GetRight() const;
        glm::vec3 GetUp() const;

        // Transform operations
        void Translate(const glm::vec3& delta);
        void Rotate(const glm::quat& delta);
        void RotateEuler(const glm::vec3& eulerDegrees);
        void ScaleBy(const glm::vec3& scaleFactors);

        // Look at functionality
        void LookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));

        // Lifecycle
        void OnPreUpdate(float deltaTime) override;
        void OnUpdate(float deltaTime) override;
        void OnAttach(Actor* newParent) override;
        void OnDetach(Actor* oldParent) override;
        void Inspect() override;

        // Call this after manually modifying transform properties via reflection
        void MarkTransformDirty() { MarkDirty(); }

        void Serialize(nlohmann::json& out) const override;
        void Deserialize(const nlohmann::json& in) override;

        virtual UpdateFlags GetUpdateFlags() const override { return UpdateFlags::UpdateDuringEditor; }
    private:
        void MarkDirty();
        void UpdateWorldTransform() const;

        glm::vec3 m_Position;
        glm::quat m_Rotation;
        glm::vec3 m_Scale;

        // Cached world transform
        mutable glm::mat4 m_WorldMatrix;
        mutable bool m_Dirty;
    };
}
