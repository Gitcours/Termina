#pragma once

#include <Termina/Core/Common.hpp>

#include "Resource.hpp"

namespace Termina {
    enum class BufferUsage
    {
        VERTEX = BIT(0),
        INDEX = BIT(1),
        CONSTANT = BIT(2),
        SHADER_READ = BIT(3),
        SHADER_WRITE = BIT(4),
        TRANSFER = BIT(5),
        READBACK = BIT(6),
        ACCELERATION_STRUCTURE = BIT(7),
        INDIRECT_COMMANDS = BIT(8)
    };
    ENUM_CLASS_FLAG_OPERATORS(BufferUsage)
    
    struct BufferDesc
    {
        uint64 Size = 0;
        uint64 Stride = 0;
        BufferUsage Usage;
    
        BufferDesc& SetSize(uint64 size)
        {
            Size = size;
            return *this;
        }
    
        BufferDesc& SetStride(uint64 stride)
        {
            Stride = stride;
            return *this;
        }
    
        BufferDesc& SetUsage(BufferUsage usage)
        {
            Usage = usage;
            return *this;
        }
    };
    
    class RendererBuffer : public RendererResource
    {
    public:
        virtual ~RendererBuffer() = default;
    
        virtual void* Map() = 0;
        virtual void Unmap() = 0;
        virtual uint64 GetGPUAddress() const = 0;
    
        BufferDesc GetDesc() const { return m_Desc; }
        uint64 GetSize() const { return m_Desc.Size; };
        uint64 GetStride() const { return m_Desc.Stride; };
        BufferUsage GetUsage() const { return m_Desc.Usage; };
    protected:
        BufferDesc m_Desc;
    };
}
