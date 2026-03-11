#pragma once

#include <cstdint>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef float float32;
typedef double float64;

#define BIT(x) (1 << x)

#define ENUM_CLASS_FLAG_OPERATORS(EnumType)                                 \
    inline EnumType operator|(EnumType a, EnumType b)                      \
    {                                                                     \
        return static_cast<EnumType>(                                     \
            static_cast<std::underlying_type<EnumType>::type>(a) |       \
            static_cast<std::underlying_type<EnumType>::type>(b));       \
    }                                                                     \
                                                                          \
    inline EnumType &operator|=(EnumType &a, EnumType b)                 \
    {                                                                     \
        a = a | b;                                                       \
        return a;                                                        \
    }                                                                     \
                                                                          \
    inline EnumType operator&(EnumType a, EnumType b)                    \
    {                                                                     \
        return static_cast<EnumType>(                                     \
            static_cast<std::underlying_type<EnumType>::type>(a) &       \
            static_cast<std::underlying_type<EnumType>::type>(b));       \
    }                                                                     \
                                                                          \
    inline EnumType &operator&=(EnumType &a, EnumType b)                 \
    {                                                                     \
        a = a & b;                                                       \
        return a;                                                        \
    }                                                                     \
                                                                          \
    inline EnumType operator~(EnumType a)                                \
    {                                                                     \
        return static_cast<EnumType>(                                     \
            ~static_cast<std::underlying_type<EnumType>::type>(a));      \
    }                                                                     \
                                                                            \
    inline bool Any(EnumType value, EnumType flag)                          \
    {                                                                     \
        using UnderlyingType = std::underlying_type<EnumType>::type;     \
        return (static_cast<UnderlyingType>(value) &                    \
                static_cast<UnderlyingType>(flag)) != 0;                 \
    }                                                                     

template<typename T>
inline T AlignUp(T value, T alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

#define KILOBYTES(x) ((x)*1024)
#define MEGABYTES(x) (KILOBYTES(x) * 1024)
#define GIGABYTES(x) (MEGABYTES(x) * 1024)
