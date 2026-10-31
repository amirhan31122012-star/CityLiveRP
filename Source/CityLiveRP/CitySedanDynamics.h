#pragma once
#include <algorithm>
#include <cmath>

namespace CityLiveDriving
{
struct PedalState
{
    bool Throttle = false;
    bool Brake = false;
    void SetThrottle(bool Held) { Throttle = Held; if (Held) Brake = false; }
    void SetBrake(bool Held) { Brake = Held; if (Held) Throttle = false; }
    void Reset() { Throttle = Brake = false; }
};
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
    const float LowSpeedRate = 65.f * DegreesToRadians *
        std::clamp(Speed / 350.f, -1.f, 1.f);
    // Allow slightly tighter urban turns, tapering back to the original 0.6 g
    // bound at top speed so grip and steering do not fight.
    const float HighwayBlend = std::clamp((std::abs(Speed) - 1500.f) / 1500.f, 0.f, 1.f);
    const float Limit = (680.f - 80.f * HighwayBlend) / std::max(std::abs(Speed), 350.f);
    // Apply the stick after the speed limit: a small touch stays a small turn,
    // instead of reaching full cornering force prematurely at high speed.
    return std::clamp(Steering, -1.f, 1.f) * std::clamp(LowSpeedRate, -Limit, Limit);
}
inline float DriftStrength(float Speed)
{
    return std::clamp((std::abs(Speed)-400.f)/600.f,0.f,1.f);
}
inline float LateralGrip(float Drift)
{
    return 6.f-4.6f*std::clamp(Drift,0.f,1.f);
}
inline float HandbrakeSpeed(float Speed,float Drift,float DeltaTime)
{
    const float Step=180.f*std::clamp(Drift,0.f,1.f)*std::max(0.f,DeltaTime);
    return Speed>0.f ? std::max(0.f,Speed-Step) : std::min(0.f,Speed+Step);
}
inline float AdvanceSpeed(float Speed, bool DriverPresent, bool Throttle, bool Brake, float DeltaTime)
{
    // Brake first stops forward motion. Once stopped, holding it selects reverse.
    // Gas first stops reverse motion. Both pedals together always brake to zero.
    const float Dt = std::max(0.f, DeltaTime);
    // Carry remaining frame time through the stop: one held press selects the
    // opposite direction, including at low frame rates, without a pause at zero.
    if (DriverPresent && Throttle && !Brake && Speed < -DirectionChangeSpeed)
    {
        const float StopTime = -Speed / BrakeAcceleration;
        return Dt <= StopTime ? Speed + BrakeAcceleration * Dt
            : std::min(ForwardLimit, EngineAcceleration * (Dt - StopTime));
    }
    if (DriverPresent && Brake && !Throttle && Speed > DirectionChangeSpeed)
    {
        const float StopTime = Speed / BrakeAcceleration;
        return Dt <= StopTime ? Speed - BrakeAcceleration * Dt
            : -std::min(ReverseLimit, 180.f * (Dt - StopTime));
    }
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
