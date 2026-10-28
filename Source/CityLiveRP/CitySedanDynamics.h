#pragma once
#include <algorithm>

namespace CityLiveDriving
{
inline float AdvanceSpeed(float Speed, bool DriverPresent, bool Throttle, bool Brake, float DeltaTime)
{
    // Brake first stops forward motion. Once stopped, holding it selects reverse.
    // Gas first stops reverse motion. Both pedals together always brake to zero.
    float Target = 0.f;
    float Acceleration = 180.f;
    if (DriverPresent && Brake)
    {
        Target = !Throttle && Speed <= 0.f ? -400.f : 0.f;
        Acceleration = Speed > 0.f || Throttle ? 900.f : 180.f;
    }
    else if (DriverPresent && Throttle)
    {
        Target = Speed < 0.f ? 0.f : 1200.f;
        Acceleration = Speed < 0.f ? 700.f : 260.f;
    }
    const float Step = Acceleration * std::max(0.f, DeltaTime);
    return Speed < Target ? std::min(Target, Speed + Step) : std::max(Target, Speed - Step);
}
}
