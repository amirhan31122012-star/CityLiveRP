#pragma once
#include <algorithm>
#include <cmath>

namespace CitySuspension
{
inline float SupportCenter(float A, float B, float C, float D)
{
    return (A + B + C + D) * .25f;
}
inline float Force(float Compression, float VerticalSpeed, float Mass = 1200.f, float DeltaTime = 1.f / 60.f)
{
    // Implicit spring/damper: raw explicit feedback was unstable at low FPS.
    const float WheelMass = std::max(Mass / 4.f, 1.f);
    const float Preload = WheelMass * 980.f;
    const float Stiffness = 12000.f;
    const float Damping = 2.f * std::sqrt(Stiffness * WheelMass);
    const float Dt = std::clamp(DeltaTime, .001f, .1f);
    const float Error = std::clamp(Compression, -25.f, 25.f);
    // Use a conservative contact mass: roll/pitch have less effective inertia
    // than pure vertical translation, and all four contacts share the body.
    const float ContactMass = std::max(WheelMass * .25f, 1.f);
    const float Denominator = 1.f + (Damping * Dt + Stiffness * Dt * Dt) / ContactMass;
    const float Correction = (Stiffness * Error - (Damping + Stiffness * Dt) * VerticalSpeed) / Denominator;
    return std::clamp(Preload + Correction, 0.f, Preload * 2.f);
}
constexpr float RestLength = 30.f;
constexpr float Travel = 25.f;
constexpr float WheelRadius = 32.f;
}
