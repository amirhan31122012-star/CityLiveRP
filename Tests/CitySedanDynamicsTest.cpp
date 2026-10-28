#include "CitySedanDynamics.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main()
{
    using CityLiveDriving::AdvanceSpeed;
    float SlowFps = 0.f, FastFps = 0.f;
    for (int I = 0; I < 60; ++I) SlowFps = AdvanceSpeed(SlowFps, true, true, false, 1.f / 30.f);
    for (int I = 0; I < 240; ++I) FastFps = AdvanceSpeed(FastFps, true, true, false, 1.f / 120.f);
    assert(std::abs(SlowFps - FastFps) < 0.01f && SlowFps > 500.f);
    for (int I = 0; I < 600; ++I) SlowFps = AdvanceSpeed(SlowFps, true, true, false, 1.f / 60.f);
    assert(SlowFps == 1200.f);
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
    std::cout << "PASS: acceleration, limits, brake-to-reverse, gas-to-forward, pedal priority and empty car\n";
}
