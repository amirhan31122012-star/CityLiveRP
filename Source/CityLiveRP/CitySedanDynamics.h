#pragma once
#include <algorithm>
#include <cmath>

namespace CityLiveDriving
{
constexpr float ForwardLimit = 3000.f; // 108 km/h
constexpr float ReverseLimit = 400.f;
constexpr float EngineAcceleration = 650.f;
constexpr float BrakeAcceleration = 1200.f;
// Physics may report a few cm/s of rollback while the car is effectively stopped.
// Requiring exactly zero makes each new frame select braking instead of drive.
constexpr float DirectionChangeSpeed = 10.f;
inline float YawRateRadians(float Steering, float Speed)
{
    constexpr float DegreesToRadians = .01745329252f;
    const float LowSpeedRate = std::clamp(Steering, -1.f, 1.f) * 55.f * DegreesToRadians *
        std::clamp(Speed / 350.f, -1.f, 1.f);
    // Bound requested lateral acceleration to about 0.6 g. A fixed yaw rate
    // at top speed asks for several g and makes grip and steering fight.
    const float Limit = 600.f / std::max(std::abs(Speed), 350.f);
    return std::clamp(LowSpeedRate, -Limit, Limit);
}
inline float AdvanceSpeed(float Speed, bool DriverPresent, bool Throttle, bool Brake, float DeltaTime)
{
    // Brake first stops forward motion. Once stopped, holding it selects reverse.
    // Gas first stops reverse motion. Both pedals together always brake to zero.
    float Target = 0.f;
    float Acceleration = 180.f;
    if (DriverPresent && Brake)
    {
        Target = !Throttle && Speed <= DirectionChangeSpeed ? -ReverseLimit : 0.f;
        Acceleration = Speed > DirectionChangeSpeed || Throttle ? BrakeAcceleration : 180.f;
    }
    else if (DriverPresent && Throttle)
    {
        Target = Speed < -DirectionChangeSpeed ? 0.f : ForwardLimit;
        Acceleration = Speed < -DirectionChangeSpeed ? BrakeAcceleration : EngineAcceleration;
    }
    const float Step = Acceleration * std::max(0.f, DeltaTime);
    return Speed < Target ? std::min(Target, Speed + Step) : std::max(Target, Speed - Step);
}
}
