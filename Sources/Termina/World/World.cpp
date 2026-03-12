#include "World.hpp"
#include "ComponentRegistry.hpp"

#include <Termina/Core/Application.hpp>
#include <Termina/Core/ID.hpp>
#include <Termina/Core/Logger.hpp>
#include <Termina/Scripting/ScriptModuleManager.hpp>
#include <ThirdParty/JSON/json.hpp>

#include <fstream>

namespace Termina {
    World::~World()
    {
        Clear();
    }

    Actor* World::SpawnActor()
    {
        auto actor = std::make_shared<Actor>(this);
        Actor* ptr = actor.get();
        m_Actors.push_back(std::move(actor));
        ptr->AddComponent<Transform>();
        ptr->OnInit();
        return ptr;
    }

    Actor* World::SpawnActorFrom(Actor* actor)
    {
        Actor* newActor = SpawnActor();
        
        nlohmann::json temp;
        for (Component* comp : actor->GetAllComponents()) {
            std::string typeName = ComponentRegistry::Get().GetNameForType(typeid(*comp));
            if (typeName.empty()) continue; // unregistered — skip

            nlohmann::json compJson;
            compJson["type"]   = typeName;
            compJson["active"] = comp->IsActive();
            nlohmann::json data = nlohmann::json::object();
            comp->Serialize(data);
            compJson["data"] = std::move(data);

            temp.push_back(std::move(compJson));
        }
        for (const auto& compJson : temp) {
            std::string type = compJson.value("type", "");
            bool compActive  = compJson.value("active", true);
            const auto& data = compJson.contains("data") ? compJson["data"] : nlohmann::json::object();

            if (type == "Transform") {
                // Already added by SpawnActor — just deserialize its data.
                newActor->GetComponent<Transform>().Deserialize(data);
                newActor->GetComponent<Transform>().SetActive(compActive);
                continue;
            }

            Component* comp = ComponentRegistry::Get().CreateByName(type, newActor);
            if (!comp) {
                TN_WARN("Unknown component type '%s' — skipping.", type.c_str());
                continue;
            }
            comp->SetActive(compActive);
            comp->Deserialize(data);
            newActor->AddComponentRaw(comp);
        }
        return newActor;
    }

    void World::DestroyActor(Actor* actor)
    {
        if (!actor) return;

        for (uint64 i = 0; i < m_Actors.size(); ++i) {
            if (m_Actors[i]->GetID() == actor->GetID()) {
                m_Actors.erase(m_Actors.begin() + i);
                break;
            }
        }
    }

    Actor* World::GetActorById(uint64 id)
    {
        for (uint64 i = 0; i < m_Actors.size(); ++i) {
            if (m_Actors[i]->GetID() == id) {
                return m_Actors[i].get();
            }
        }
        return nullptr;
    }

    Actor* World::GetActorByName(const std::string& name)
    {
        for (uint64 i = 0; i < m_Actors.size(); ++i) {
            if (m_Actors[i]->GetName() == name) {
                return m_Actors[i].get();
            }
        }
        return nullptr;
    }

    std::vector<Actor*> World::GetRootActors() const
    {
        std::vector<Actor*> rootActors;
        for (const auto& actorPtr : m_Actors) {
            if (actorPtr->GetParent() == nullptr) {
                rootActors.push_back(actorPtr.get());
            }
        }
        return rootActors;
    }

    void World::OnInit()
    {
        for (auto& actor : m_Actors) actor->OnInit();
    }

    void World::OnShutdown()
    {
        for (auto& actor : m_Actors) actor->OnShutdown();
    }

    void World::OnPlay()
    {
        for (auto& actor : m_Actors) actor->OnPlay();
    }

    void World::OnStop()
    {
        for (auto& actor : m_Actors) actor->OnStop();
    }

    void World::OnPreUpdate(float deltaTime)
    {
        for (auto& actor : m_Actors) actor->OnPreUpdate(deltaTime);
    }

    void World::OnUpdate(float deltaTime)
    {
        for (auto& actor : m_Actors) actor->OnUpdate(deltaTime);
    }

    void World::OnPostUpdate(float deltaTime)
    {
        for (auto& actor : m_Actors) actor->OnPostUpdate(deltaTime);
    }

    void World::OnPrePhysics(float deltaTime)
    {
        for (auto& actor : m_Actors) actor->OnPrePhysics(deltaTime);
    }

    void World::OnPhysics(float deltaTime)
    {
        for (auto& actor : m_Actors) actor->OnPhysics(deltaTime);
    }

    void World::OnPostPhysics(float deltaTime)
    {
        for (auto& actor : m_Actors) actor->OnPostPhysics(deltaTime);
    }

    void World::OnPreRender(float deltaTime)
    {
        for (auto& actor : m_Actors) actor->OnPreRender(deltaTime);
    }

    void World::OnRender(float deltaTime)
    {
        for (auto& actor : m_Actors) actor->OnRender(deltaTime);
    }

    void World::OnPostRender(float deltaTime)
    {
        for (auto& actor : m_Actors) actor->OnPostRender(deltaTime);
    }

    void World::Clear()
    {
        m_Actors.clear();
    }

    void World::LoadFromFile(const std::string& filename)
    {
        m_CurrentPath = filename;

        std::ifstream file(filename);
        if (!file.is_open()) {
            TN_ERROR("Could not open '%s' for reading.", filename.c_str());
            return;
        }

        nlohmann::json root;
        try {
            file >> root;
        } catch (const nlohmann::json::exception& e) {
            TN_ERROR("JSON parse error in '%s': %s", filename.c_str(), e.what());
            return;
        }

        int version = root.value("version", 0);
        if (version != 1)
            TN_WARN("Unknown world version %d, attempting load anyway.", version);

        m_Name = root.value("name", "World");

        // Load DLL modules before deserializing actors.
        if (root.contains("modules")) {
            for (const auto& mod : root["modules"]) {
                std::string modName = mod.value("name", "");
                std::string modPath = mod.value("path", "");
                if (!modName.empty() && !modPath.empty() && !ScriptModuleManager::Get().IsLoaded(modName))
                    ScriptModuleManager::Get().Load(modName, modPath);
            }
        }

        Clear();
        IDGenerator::Get().Clear();

        if (!root.contains("actors")) return;

        // Pass 1: Spawn actors flat, deserialize components, inject saved IDs.
        struct ActorEntry {
            Actor* actor;
            uint64 parentId;
        };
        std::vector<ActorEntry> entries;

        for (const auto& actorJson : root["actors"]) {
            uint64 savedId = std::stoull(actorJson.value("id", "0"));
            std::string actorName = actorJson.value("name", "Actor");
            bool actorActive     = actorJson.value("active", true);

            // Construct actor directly to avoid SpawnActor's auto-OnInit.
            auto actorPtr = std::make_shared<Actor>(this, actorName);
            Actor* actor  = actorPtr.get();

            // Replace the auto-generated ID with the saved one.
            IDGenerator::Get().Release(actor->GetID());
            IDGenerator::Get().Reserve(savedId);
            actor->SetID(savedId);

            actor->SetActive(actorActive);

            // Auto-add Transform (mirrors what SpawnActor does).
            actor->AddComponent<Transform>();

            // Deserialize components.
            if (actorJson.contains("components")) {
                for (const auto& compJson : actorJson["components"]) {
                    std::string type = compJson.value("type", "");
                    bool compActive  = compJson.value("active", true);
                    const auto& data = compJson.contains("data") ? compJson["data"] : nlohmann::json::object();

                    if (type == "Transform") {
                        // Already added above — just deserialize its data.
                        actor->GetComponent<Transform>().Deserialize(data);
                        actor->GetComponent<Transform>().SetActive(compActive);
                        continue;
                    }

                    Component* comp = ComponentRegistry::Get().CreateByName(type, actor);
                    if (!comp) {
                        TN_WARN("Unknown component type '%s' — skipping.", type.c_str());
                        continue;
                    }
                    comp->SetActive(compActive);
                    comp->Deserialize(data);
                    actor->AddComponentRaw(comp);
                }
            }

            uint64 parentId = 0;
            if (actorJson.contains("parentId") && !actorJson["parentId"].is_null())
                parentId = std::stoull(actorJson["parentId"].get<std::string>());

            m_Actors.push_back(std::move(actorPtr));
            entries.push_back({ actor, parentId });
        }

        // Pass 2: Wire parent-child hierarchy.
        for (const auto& entry : entries) {
            if (entry.parentId == 0) continue;
            Actor* parent = GetActorById(entry.parentId);
            if (parent)
                parent->AttachChild(entry.actor);
            else
                TN_WARN("Parent ID %llu not found for actor '%s'.",
                    static_cast<unsigned long long>(entry.parentId), entry.actor->GetName().c_str());
        }

        // Pass 3: Initialize all actors.
        for (auto& actor : m_Actors)
            actor->OnInit();
    }

    void World::SaveToFile(const std::string& filename)
    {
        if (!filename.empty())
            m_CurrentPath = filename;

        if (m_CurrentPath.empty()) {
            TN_ERROR("SaveToFile called with no path.");
            return;
        }

        nlohmann::json root;
        root["version"] = 1;
        root["name"]    = m_Name;

        // Emit loaded script modules.
        nlohmann::json modulesJson = nlohmann::json::array();
        for (const auto& [modName, modPath] : ScriptModuleManager::Get().GetLoadedModules()) {
            modulesJson.push_back({ {"name", modName}, {"path", modPath} });
        }
        root["modules"] = modulesJson;

        // DFS traversal: parents before children.
        nlohmann::json actorsJson = nlohmann::json::array();

        std::function<void(Actor*)> serializeActor = [&](Actor* actor) {
            nlohmann::json actorJson;
            actorJson["id"]   = std::to_string(actor->GetID());
            actorJson["name"] = actor->GetName();
            actorJson["active"] = actor->IsActive();

            if (actor->GetParent())
                actorJson["parentId"] = std::to_string(actor->GetParent()->GetID());
            else
                actorJson["parentId"] = nullptr;

            nlohmann::json compsJson = nlohmann::json::array();
            for (Component* comp : actor->GetAllComponents()) {
                std::string typeName = ComponentRegistry::Get().GetNameForType(typeid(*comp));
                if (typeName.empty()) continue; // unregistered — skip

                nlohmann::json compJson;
                compJson["type"]   = typeName;
                compJson["active"] = comp->IsActive();
                nlohmann::json data = nlohmann::json::object();
                comp->Serialize(data);
                compJson["data"] = std::move(data);
                compsJson.push_back(std::move(compJson));
            }
            actorJson["components"] = std::move(compsJson);
            actorsJson.push_back(std::move(actorJson));

            for (Actor* child : actor->GetChildren())
                serializeActor(child);
        };

        for (Actor* root_actor : GetRootActors())
            serializeActor(root_actor);

        root["actors"] = std::move(actorsJson);

        std::ofstream file(m_CurrentPath);
        if (!file.is_open()) {
            TN_ERROR("Could not open '%s' for writing.", m_CurrentPath.c_str());
            return;
        }
        file << root.dump(4);
    }
}
