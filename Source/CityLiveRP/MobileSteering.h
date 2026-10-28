#pragma once

#include <algorithm>
#include <cmath>

namespace CityLiveMobile
{
// Exact integration of a smoothed angular velocity, independent of frame rate.
inline float AdvanceTurn(float& Rate, float Steering, float DeltaTime,
    float DegreesPerSecond, float Response)
{
    if (DeltaTime <= 0.f) return 0.f;
    const float Target = std::clamp(Steering, -1.f, 1.f) * std::max(0.f, DegreesPerSecond);
    const float Speed = std::max(Response, 0.01f);
    const float Blend = -std::expm1(-Speed * DeltaTime);
    const float Angle = Target * DeltaTime + (Rate - Target) * Blend / Speed;
    Rate += (Target - Rate) * Blend;
    return Angle;
}
}
