#pragma once
#include <algorithm>

namespace CitySuspension
{
inline float Force(float Compression, float VerticalSpeed, float Mass = 1200.f)
{
    // UE uses centimetres: preload balances a quarter of the chassis weight.
    return std::clamp(Mass * 980.f / 4.f + Compression * 45000.f - VerticalSpeed * 5000.f,
        0.f, Mass * 980.f * 1.5f);
}
constexpr float RestLength = 30.f;
constexpr float Travel = 25.f;
constexpr float WheelRadius = 32.f;
}
