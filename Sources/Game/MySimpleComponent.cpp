#include "MySimpleComponent.hpp"

void MySimpleComponent::Update(float deltaTime)
{
    glm::vec3 position = m_Transform->GetPosition();

    if (Input::IsKeyHeld(Termina::Key::Left)) {
        position -= m_Transform->GetRight() * deltaTime;
    }
    if (Input::IsKeyHeld(Termina::Key::Right)) {
        position += m_Transform->GetRight() * deltaTime;
    }
    if (Input::IsKeyHeld(Termina::Key::Up)) {
        position += m_Transform->GetForward() * deltaTime;
    }
    if (Input::IsKeyHeld(Termina::Key::Down)) {
        position -= m_Transform->GetForward() * deltaTime;
    }
    if (Input::IsKeyHeld(Termina::Key::Space)) {
        position -= m_Transform->GetUp() * deltaTime;
    }

    m_Transform->SetPosition(position);
}
