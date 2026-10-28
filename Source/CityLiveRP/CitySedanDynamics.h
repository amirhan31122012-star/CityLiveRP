#pragma once
#include <algorithm>

namespace CityLiveDriving
{
inline float AdvanceSpeed(float Speed, bool DriverPresent, bool Throttle, bool Brake, float DeltaTime)
{
    const float Target = DriverPresent && Throttle && !Brake ? 1200.f : 0.f;
    const float Acceleration = Brake ? 900.f : Target > 0.f ? 260.f : 180.f;
    const float Step = Acceleration * std::max(0.f, DeltaTime);
    return Speed < Target ? std::min(Target, Speed + Step) : std::max(Target, Speed - Step);
}
}
