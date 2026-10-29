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
inline float SeatShift(float Time) { return Blend(Time, 3.05f, 3.95f); }
inline float BodyTurn(float Time) { return Blend(Time, 2.85f, 3.95f); }
inline float TorsoDip(float Time)
{
    return Blend(Time, 1.95f, 2.4f) * (1.f - Blend(Time, 3.8f, 4.f));
}
// Raise the ankle before crossing the sill, then lower it onto the cabin floor.
// Running backwards plants both feet outside before the body rises.
inline float FootTravel(float Progress) { return Blend(Progress, .2f, .8f); }
inline float FootLift(float Progress)
{
    return Blend(Progress, 0.f, .3f) * (1.f - Blend(Progress, .7f, 1.f));
}
constexpr float Duration = 4.8f;
}
