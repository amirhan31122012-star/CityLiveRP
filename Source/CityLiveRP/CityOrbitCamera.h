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
inline bool ShouldFollow(float Speed,bool Held,float SinceLook)
{return std::abs(Speed)>40.f && !Held && SinceLook>1.2f;}
inline State Follow(float Yaw,float Pitch,float DefaultPitch,float DeltaTime)
{
    const float Keep=std::exp(-3.5f*std::max(0.f,DeltaTime));
    return {std::remainder(Yaw,360.f)*Keep,DefaultPitch+(Pitch-DefaultPitch)*Keep};
}
}
