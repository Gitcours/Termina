#pragma once

#include "Buffer.hpp"

namespace Termina {
    enum class BufferViewType
    {
        CONSTANT,
        SHADER_READ,
        SHADER_WRITE
    };
    
    struct BufferViewDesc
    {
        RendererBuffer* Buffer;
        BufferViewType Type;
    
        BufferViewDesc& SetBuffer(RendererBuffer* buffer)
        {
            Buffer = buffer;
            return *this;
        }
    
        BufferViewDesc& SetType(BufferViewType type)
        {
            Type = type;
            return *this;
        }
    };
    
    class BufferView
    {
    public:
        virtual ~BufferView() = default;
    
        virtual int32 GetBindlessHandle() const = 0;
        const BufferViewDesc& GetDesc() const { return m_Desc; }
    protected:
        BufferViewDesc m_Desc;
    };
}
