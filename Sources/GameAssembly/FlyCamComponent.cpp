#include "FlyCamComponent.hpp"

#include <algorithm>
#include <cmath>

void FlyCamComponent::Serialize(nlohmann::json& out) const
{
    out["moveSpeed"]      = m_MoveSpeed;
    out["fastMultiplier"] = m_FastMultiplier;
    out["sensitivity"]    = m_Sensitivity;
    out["acceleration"]   = m_Acceleration;
    out["yaw"]            = m_Yaw;
    out["pitch"]          = m_Pitch;
    out["velocity"]       = { m_Velocity.x, m_Velocity.y, m_Velocity.z };
    out["wasRotating"]    = m_WasRotating;
}

void FlyCamComponent::Deserialize(const nlohmann::json& in)
{
    if (in.contains("moveSpeed"))      m_MoveSpeed      = in["moveSpeed"];
    if (in.contains("fastMultiplier")) m_FastMultiplier = in["fastMultiplier"];
    if (in.contains("sensitivity"))    m_Sensitivity    = in["sensitivity"];
    if (in.contains("acceleration"))   m_Acceleration   = in["acceleration"];
    if (in.contains("yaw"))            m_Yaw            = in["yaw"];
    if (in.contains("pitch"))          m_Pitch          = in["pitch"];
    if (in.contains("velocity")) {
        const auto& v = in["velocity"];
        m_Velocity = { v[0], v[1], v[2] };
    }
    if (in.contains("wasRotating")) m_WasRotating = in["wasRotating"];
}

void FlyCamComponent::Update(float deltaTime)
{
    bool rotating = Input::IsMouseButtonHeld(Termina::MouseButton::Right);

    if (rotating != m_WasRotating)
    {
        Input::SetCursorVisible(!rotating);
        Input::SetCursorLocked(rotating);
        m_WasRotating = rotating;
    }

    if (rotating)
    {
        glm::vec2 delta = Input::GetMouseDelta();
        m_Yaw   -= delta.x * m_Sensitivity;
        m_Pitch -= delta.y * m_Sensitivity;
        m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);

        glm::quat pitchQuat = glm::angleAxis(glm::radians(m_Pitch), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::quat yawQuat   = glm::angleAxis(glm::radians(m_Yaw), glm::vec3(0.0f, 1.0f, 0.0f));
        m_Transform->SetRotation(yawQuat * pitchQuat);
    }

    glm::vec3 forward = m_Transform->GetForward();
    glm::vec3 right   = m_Transform->GetRight();
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float speed = m_MoveSpeed;
    if (Input::IsKeyHeld(Termina::Key::LeftShift) || Input::IsKeyHeld(Termina::Key::RightShift))
        speed *= m_FastMultiplier;

    glm::vec3 targetVelocity = glm::vec3(0.0f);
    if (Input::IsKeyHeld(Termina::Key::W)) targetVelocity += forward;
    if (Input::IsKeyHeld(Termina::Key::S)) targetVelocity -= forward;
    if (Input::IsKeyHeld(Termina::Key::D)) targetVelocity += right;
    if (Input::IsKeyHeld(Termina::Key::A)) targetVelocity -= right;
    if (Input::IsKeyHeld(Termina::Key::E)) targetVelocity += worldUp;
    if (Input::IsKeyHeld(Termina::Key::Q)) targetVelocity -= worldUp;

    if (glm::length(targetVelocity) > 0.0f)
        targetVelocity = glm::normalize(targetVelocity) * speed;

    float blend = 1.0f - expf(-m_Acceleration * deltaTime);
    m_Velocity = glm::mix(m_Velocity, targetVelocity, blend);

    if (glm::length(m_Velocity) < 0.001f)
        m_Velocity = glm::vec3(0.0f);

    m_Transform->SetPosition(m_Transform->GetPosition() + m_Velocity * deltaTime);
}
