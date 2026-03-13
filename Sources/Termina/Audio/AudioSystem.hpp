#pragma once

#include <MiniAudio/miniaudio.h>

#include <Termina/Core/System.hpp>

namespace Termina {
    /// Audio system that manages audio playback using miniaudio.
    class AudioSystem : public ISystem
    {
    public:
        AudioSystem();
        ~AudioSystem();

        ma_device& GetDevice() { return m_Device; }
        ma_engine& GetEngine() { return m_Engine; }

        void Update(float deltaTime) override;

        /// Sets the volume of the audio engine.
        void SetVolume(float volume) { ma_engine_set_volume(&m_Engine, volume); }

        void RegisterComponents() override;
        void UnregisterComponents() override;

        UpdateFlags GetUpdateFlags() const override { return UpdateFlags::UpdateDuringEditor; }
    	std::string GetName() const override { return "Audio System"; }
        int GetPriority() const override { return 0; }
    private:
        ma_device m_Device;
        ma_engine m_Engine;
    };
}
