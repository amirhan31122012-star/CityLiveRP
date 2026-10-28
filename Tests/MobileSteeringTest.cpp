#include "MobileSteering.h"
#include <cassert>
#include <iostream>

struct Result { float Angle; float Rate; };

Result Hold(float Input, int Fps, int Seconds, float InitialRate = 0.f)
{
    Result Out{0.f, InitialRate};
    for (int i = 0; i < Fps * Seconds; ++i)
        Out.Angle += CityLiveMobile::AdvanceTurn(Out.Rate, Input, 1.f / Fps, 75.f, 8.f);
    return Out;
}

int main()
{
    // Holding either side must continue beyond a full revolution, not stop at a heading.
    const auto Right = Hold(1.f, 60, 10);
    const auto Left = Hold(-1.f, 60, 10);
    assert(Right.Angle > 720.f && Left.Angle < -720.f);
    assert(std::abs(Right.Angle + Left.Angle) < 0.01f);
    std::cout << "PASS: continuous turns in both directions\n";

    // Mobile frame rate must not change how far the character turns.
    assert(std::abs(Hold(1.f, 30, 10).Angle - Hold(1.f, 120, 10).Angle) < 0.02f);
    std::cout << "PASS: consistent rotation at 30 / 120 FPS\n";

    float Rate = 0.f;
    const float FirstAngle = CityLiveMobile::AdvanceTurn(Rate, 1.f, 1.f / 60.f, 75.f, 8.f);
    assert(Rate > 0.f && Rate < 15.f && FirstAngle < 0.15f);
    assert(Right.Rate <= 75.001f);
    assert(std::abs(Hold(0.5f, 60, 10).Angle * 2.f - Right.Angle) < 0.02f);
    std::cout << "PASS: gentle start, speed limit and proportional stick input\n";

    const auto Stopped = Hold(0.f, 60, 3, Right.Rate);
    assert(Stopped.Rate >= 0.f && Stopped.Rate < 0.001f);
    assert(Stopped.Angle > 0.f && Stopped.Angle < 10.f);
    std::cout << "PASS: release slows to rest without reversing or endless spinning\n";

    Rate = Right.Rate;
    CityLiveMobile::AdvanceTurn(Rate, -1.f, 1.f / 60.f, 75.f, 8.f);
    assert(Rate > 0.f && Rate < Right.Rate);
    assert(Hold(-1.f, 60, 1, Right.Rate).Rate < -74.f);
    std::cout << "PASS: direction changes smoothly\n";

    Rate = 25.f;
    assert(CityLiveMobile::AdvanceTurn(Rate, 1.f, 0.f, 75.f, 8.f) == 0.f);
    assert(Rate == 25.f);
    std::cout << "PASS: paused time causes no rotation\n";
}
