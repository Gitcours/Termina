#pragma once

#include <vector>

#include "Common.hpp"

class FreeList
{
public:
    static const int INVALID = -1;

    FreeList(uint64 maxSlots);
    ~FreeList();

    int Allocate();
    void Free(int index);
private:
    void SetBit(int index);
    void ClearBit(int index);
    bool IsBitSet(int index) const;

    uint64 m_MaxSlots;
    uint64 m_BitmapSize;
    std::vector<uint64> m_Bitmap;
    std::vector<uint32> m_FreeList;
};
