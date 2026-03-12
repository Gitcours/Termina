#pragma once

#include <Termina/Renderer/Camera.hpp>
#include <GLM/glm.hpp>

class EditorCamera : public Termina::Camera
{
public:
    EditorCamera();

    void Update(float dt, float viewportWidth, float viewportHeight);

    void SetPosition(const glm::vec3& position) { m_Position = position; }
    void SetYaw(float yaw)                       { m_Yaw = yaw; }
    void SetPitch(float pitch)                   { m_Pitch = pitch; }
    void SetFOV(float fov)                       { m_FOV = fov; }
    void SetMoveSpeed(float speed)               { m_MoveSpeed = speed; }
    void SetSensitivity(float sensitivity)       { m_Sensitivity = sensitivity; }

    glm::vec3 GetPosition()  const { return m_Position; }
    float     GetYaw()       const { return m_Yaw; }
    float     GetPitch()     const { return m_Pitch; }
    float     GetFOV()       const { return m_FOV; }
    glm::vec3 GetForward()   const;
    glm::vec3 GetRight()     const;
    glm::vec3 GetUp()        const;

private:
    void RecalculateProjection(float width, float height);
    void RecalculateView();

    glm::vec3 m_Position      = {  0.0f, 0.0f,  5.0f };
    glm::vec3 m_Velocity      = {  0.0f, 0.0f,  0.0f };

    float m_Yaw           = -90.0f;
    float m_Pitch         =  0.0f;
    float m_FOV           =  75.0f;
    float m_MoveSpeed     =   5.0f;
    float m_FastMultiplier=   4.0f;
    float m_Sensitivity   =   0.12f;
    float m_Acceleration  =  20.0f;

    float m_LastViewportWidth  = 0.0f;
    float m_LastViewportHeight = 0.0f;
};
