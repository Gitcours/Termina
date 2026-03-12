#pragma once

#include <MiniAudio/miniaudio.h>

#include <Termina/Core/System.hpp>

namespace Termina {
    class AudioSystem : public ISystem
    {
    public:
        AudioSystem();
        ~AudioSystem();

        ma_device& GetDevice() { return m_Device; }
        ma_engine& GetEngine() { return m_Engine; }

        void SetVolume(float volume) { ma_engine_set_volume(&m_Engine, volume); }

        UpdateFlags GetUpdateFlags() const override { return UpdateFlags::UpdateDuringEditor; }
    	std::string GetName() const override { return "Audio System"; }
        int GetPriority() const override { return 0; }
    private:
        ma_device m_Device;
        ma_engine m_Engine;
    };
}
