#include "ScriptableComponent.hpp"

namespace TerminaScript {
    void ScriptableComponent::OnInit()
    {
        m_Transform = &m_Owner->GetComponent<Termina::Transform>();
        m_Name = m_Owner->GetName();
        Awake();
    }

    void ScriptableComponent::Instantiate(Termina::Actor* actor)
    {
        m_Owner->GetParentWorld()->SpawnActorFrom(actor);
    }

    void ScriptableComponent::Destroy(Termina::Actor* actor)
    {
        m_Owner->GetParentWorld()->DestroyActor(actor);
    }
}
