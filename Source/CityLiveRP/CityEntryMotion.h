#pragma once
#include <algorithm>

namespace CityLiveEntry
{
inline float Blend(float Time, float Start, float End)
{
    const float T = std::clamp((Time - Start) / (End - Start), 0.f, 1.f);
    return T * T * (3.f - 2.f * T);
}
inline float Door(float Time)
{
    return Blend(Time, 1.3f, 1.95f) * (1.f - Blend(Time, 4.05f, 4.7f));
}
inline float Hand(float Time)
{
    return Blend(Time, .85f, 1.3f) * (1.f - Blend(Time, 1.95f, 2.3f));
}
inline float Sit(float Time) { return Blend(Time, 2.f, 2.8f); }
inline float Leg(float Time, bool Second) { return Second ? Blend(Time, 3.35f, 3.9f) : Blend(Time, 2.85f, 3.35f); }
constexpr float Duration = 4.8f;
}
