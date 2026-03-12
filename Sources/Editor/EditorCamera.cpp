#include "EditorCamera.hpp"

#include <Termina/Input/InputSystem.hpp>

#include <GLM/gtc/matrix_transform.hpp>

#include <cmath>
#include <algorithm>

EditorCamera::EditorCamera()
{
    RecalculateView();
}

void EditorCamera::Update(float dt, float viewportWidth, float viewportHeight)
{
    UpdatePrevious();

    if (viewportWidth != m_LastViewportWidth || viewportHeight != m_LastViewportHeight)
    {
        m_LastViewportWidth  = viewportWidth;
        m_LastViewportHeight = viewportHeight;
        RecalculateProjection(viewportWidth, viewportHeight);
    }

    bool rotating = Termina::InputSystem::IsMouseButtonHeld(Termina::MouseButton::Right);

    if (rotating)
    {
        glm::vec2 delta = Termina::InputSystem::GetMouseDelta();
        m_Yaw   += delta.x * m_Sensitivity;
        m_Pitch += delta.y * m_Sensitivity;
        m_Pitch  = std::clamp(m_Pitch, -89.0f, 89.0f);
    }

    float scroll = Termina::InputSystem::GetScrollDelta().y;
    if (scroll != 0.0f)
    {
        m_FOV = std::clamp(m_FOV - scroll * 2.0f, 10.0f, 120.0f);
        RecalculateProjection(m_LastViewportWidth, m_LastViewportHeight);
    }

    glm::vec3 forward = GetForward();
    glm::vec3 right   = GetRight();
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float speed = m_MoveSpeed;
    if (Termina::InputSystem::IsKeyHeld(Termina::Key::LeftShift) ||
        Termina::InputSystem::IsKeyHeld(Termina::Key::RightShift))
        speed *= m_FastMultiplier;

    glm::vec3 targetVelocity = glm::vec3(0.0f);
    if (Termina::InputSystem::IsKeyHeld(Termina::Key::W)) targetVelocity += forward;
    if (Termina::InputSystem::IsKeyHeld(Termina::Key::S)) targetVelocity -= forward;
    if (Termina::InputSystem::IsKeyHeld(Termina::Key::D)) targetVelocity += right;
    if (Termina::InputSystem::IsKeyHeld(Termina::Key::A)) targetVelocity -= right;
    if (Termina::InputSystem::IsKeyHeld(Termina::Key::E)) targetVelocity += worldUp;
    if (Termina::InputSystem::IsKeyHeld(Termina::Key::Q)) targetVelocity -= worldUp;

    if (glm::length(targetVelocity) > 0.0f)
        targetVelocity = glm::normalize(targetVelocity) * speed;

    float blend = 1.0f - expf(-m_Acceleration * dt);
    m_Velocity = glm::mix(m_Velocity, targetVelocity, blend);

    if (glm::length(m_Velocity) < 0.001f)
        m_Velocity = glm::vec3(0.0f);

    m_Position += m_Velocity * dt;

    RecalculateView();
}

void EditorCamera::RecalculateProjection(float width, float height)
{
    if (width <= 0.0f || height <= 0.0f) return;
    // Projection is kept in standard OpenGL/ImGuizmo convention (Y-up NDC).
    // For rendering we negate Y to counteract the Vulkan backend's viewport Y-flip,
    // so the offscreen texture comes out correctly oriented without a UV flip.
    Projection            = glm::perspective(glm::radians(m_FOV), width / height, Near, Far);
    InverseProjection     = glm::inverse(Projection);
    glm::mat4 renderProj  = Projection;
    renderProj[1][1]      *= -1.0f;
    ViewProjection        = renderProj * View;
    InverseViewProjection = glm::inverse(ViewProjection);
}

void EditorCamera::RecalculateView()
{
    glm::vec3 forward     = GetForward();
    View                  = glm::lookAt(m_Position, m_Position + forward, glm::vec3(0.0f, 1.0f, 0.0f));
    InverseView           = glm::inverse(View);
    Position              = m_Position;
    Direction             = forward;
    glm::mat4 renderProj  = Projection;
    renderProj[1][1]      *= -1.0f;
    ViewProjection        = renderProj * View;
    InverseViewProjection = glm::inverse(ViewProjection);
}

glm::vec3 EditorCamera::GetForward() const
{
    float yr = glm::radians(m_Yaw);
    float pr = glm::radians(m_Pitch);
    return glm::normalize(glm::vec3(
        cosf(pr) * cosf(yr),
        sinf(pr),
        cosf(pr) * sinf(yr)
    ));
}

glm::vec3 EditorCamera::GetRight() const
{
    return glm::normalize(glm::cross(GetForward(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

glm::vec3 EditorCamera::GetUp() const
{
    return glm::normalize(glm::cross(GetRight(), GetForward()));
}
