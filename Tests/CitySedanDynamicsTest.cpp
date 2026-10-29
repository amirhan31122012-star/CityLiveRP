#include "CitySedanDynamics.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main()
{
    using CityLiveDriving::AdvanceSpeed;
    for (float Speed : {0.f, 100.f, 350.f, 1000.f, 2000.f, 3000.f, -400.f})
    {
        const float Rate = CityLiveDriving::YawRateRadians(1.f, Speed);
        assert(std::abs(Rate * Speed) <= 600.01f);
        assert(std::abs(Rate + CityLiveDriving::YawRateRadians(-1.f, Speed)) < .0001f);
        assert(CityLiveDriving::YawRateRadians(0.f, Speed) == 0.f);
        assert(Rate * Speed >= 0.f);
    }
    assert(CityLiveDriving::YawRateRadians(1.f, 3000.f) < CityLiveDriving::YawRateRadians(1.f, 1000.f));
    float SlowFps = 0.f, FastFps = 0.f;
    for (int I = 0; I < 60; ++I) SlowFps = AdvanceSpeed(SlowFps, true, true, false, 1.f / 30.f);
    for (int I = 0; I < 240; ++I) FastFps = AdvanceSpeed(FastFps, true, true, false, 1.f / 120.f);
    assert(std::abs(SlowFps - FastFps) < 0.1f && SlowFps > 1200.f);
    for (int I = 0; I < 600; ++I) SlowFps = AdvanceSpeed(SlowFps, true, true, false, 1.f / 60.f);
    assert(SlowFps == CityLiveDriving::ForwardLimit);
    assert(AdvanceSpeed(0.f, false, true, false, 1.f) == 0.f);
    assert(AdvanceSpeed(0.f, true, true, true, 1.f) == 0.f);
    assert(AdvanceSpeed(1200.f, true, false, true, 2.f) == 0.f);
    assert(AdvanceSpeed(100.f, true, false, false, 1.f) == 0.f);
    assert(AdvanceSpeed(100.f, true, true, false, 0.f) == 100.f);
    float Reverse = 0.f;
    for (int I = 0; I < 600; ++I) Reverse = AdvanceSpeed(Reverse, true, false, true, 1.f / 60.f);
    assert(Reverse == -400.f && std::abs(Reverse) < SlowFps);
    assert(AdvanceSpeed(1200.f, true, false, true, 2.f) == 0.f);
    assert(AdvanceSpeed(0.f, true, false, true, .1f) < 0.f);
    assert(AdvanceSpeed(-400.f, true, true, false, 1.f) == 0.f);
    assert(AdvanceSpeed(-400.f, true, true, true, 1.f) == 0.f);
    assert(AdvanceSpeed(0.f, false, false, true, 1.f) == 0.f);
    assert(AdvanceSpeed(0.f, true, true, false, 1.f) == CityLiveDriving::EngineAcceleration);
    for (int Fps : {15, 30, 60, 120, 240})
    {
        const float Dt = 1.f / Fps;
        // Suspension/contacts can reintroduce opposite velocity every frame.
        // Engine force must still be present, rather than merely cancelling it.
        for (float Rollback : {-.01f, -.1f, -1.f, -5.f, -10.f})
        {
            const float GasAcceleration = (AdvanceSpeed(Rollback, true, true, false, Dt) - Rollback) / Dt;
            const float ReverseAcceleration = (AdvanceSpeed(-Rollback, true, false, true, Dt) + Rollback) / Dt;
            assert(std::abs(GasAcceleration - CityLiveDriving::EngineAcceleration) < .02f);
            assert(std::abs(ReverseAcceleration + 180.f) < .02f);
        }
        // Both pedals must still stop either direction even inside the threshold.
        assert(AdvanceSpeed(-.1f, true, true, true, Dt) == 0.f);
        assert(AdvanceSpeed(.1f, true, true, true, Dt) == 0.f);
    }
    assert(AdvanceSpeed(-100.f, true, true, false, .01f) < 0.f);
    assert(AdvanceSpeed(100.f, true, false, true, .01f) > 0.f);
    std::cout << "PASS: acceleration, limits, brake-to-reverse, gas-to-forward, pedal priority and empty car\n";
}
