#pragma once
#include <algorithm>
#include <cmath>

namespace CityOrbitCamera
{
struct State { float Yaw, Pitch; };
inline State Advance(float Yaw, float Pitch, float YawDelta, float PitchDelta)
{
    return {std::remainder(Yaw + YawDelta, 360.f), std::clamp(Pitch + PitchDelta, -55.f, 5.f)};
}
}
