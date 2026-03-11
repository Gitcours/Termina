#include "FreeList.hpp"

FreeList::FreeList(uint64 maxSlots)
    : m_MaxSlots(maxSlots), m_BitmapSize((maxSlots + 63) / 64)
{
    m_Bitmap.resize(m_BitmapSize, 0);
    m_FreeList.reserve(static_cast<size_t>(maxSlots));

    for (uint64 i = 0; i < maxSlots; ++i) {
        m_FreeList.push_back(static_cast<uint32>(i));
    }
}

FreeList::~FreeList()
{
    m_Bitmap.clear();
    m_FreeList.clear();
}

int32 FreeList::Allocate()
{
    if (m_FreeList.empty()) {
        return INVALID;
    }

    uint32 index = m_FreeList.back();
    m_FreeList.pop_back();
    SetBit(index);
    return static_cast<int32>(index);
}

void FreeList::Free(int32 index)
{
    if (index < 0 || static_cast<uint64>(index) >= m_MaxSlots) {
        return;
    }

    if (IsBitSet(index)) {
        ClearBit(index);
        m_FreeList.push_back(static_cast<uint32>(index));
    }
}

void FreeList::SetBit(int index)
{
    uint64 bitIndex = static_cast<uint64>(index);
    uint64 wordIndex = bitIndex / 64;
    uint64 bitPosition = bitIndex % 64;
    m_Bitmap[wordIndex] |= (1ULL << bitPosition);
}

void FreeList::ClearBit(int index)
{
    uint64 bitIndex = static_cast<uint64>(index);
    uint64 wordIndex = bitIndex / 64;
    uint64 bitPosition = bitIndex % 64;
    m_Bitmap[wordIndex] &= ~(1ULL << bitPosition);
}

bool FreeList::IsBitSet(int index) const
{
    uint64 bitIndex = static_cast<uint64>(index);
    uint64 wordIndex = bitIndex / 64;
    uint64 bitPosition = bitIndex % 64;
    return (m_Bitmap[wordIndex] & (1ULL << bitPosition)) != 0;
}
