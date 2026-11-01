#pragma once
#include <cstddef>
#include <cstdint>
namespace CityChatInput
{
template<class Char> inline size_t EraseCount(const Char* Text,size_t Length)
{
    if(!Length)return 0;
    const uint32_t Last=static_cast<uint32_t>(Text[Length-1]);
    if(Length>1&&Last>=0xDC00&&Last<=0xDFFF)
    {const uint32_t Previous=static_cast<uint32_t>(Text[Length-2]);if(Previous>=0xD800&&Previous<=0xDBFF)return 2;}
    return 1;
}
template<class Char> inline size_t Fit(const Char* Text,size_t Length,size_t Available)
{
    size_t Count=Length<Available?Length:Available;
    if(Count){const uint32_t Last=static_cast<uint32_t>(Text[Count-1]);if(Last>=0xD800&&Last<=0xDBFF)--Count;}
    return Count;
}
}
