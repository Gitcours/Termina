#include "ScriptAudio.hpp"

#include <Termina/Core/Application.hpp>

namespace TerminaScript {
    void Audio::SetVolume(float volume)
    {
        Termina::AudioSystem* audioSystem = Termina::Application::GetSystem<Termina::AudioSystem>();
        if (audioSystem) {
            audioSystem->SetVolume(volume);
        }
    }
}
