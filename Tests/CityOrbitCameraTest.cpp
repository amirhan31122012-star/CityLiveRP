#include "CityOrbitCamera.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main()
{
    using CityOrbitCamera::Advance;
    auto Front = Advance(0.f, -15.f, 180.f, 0.f);
    assert(std::abs(Front.Yaw) == 180.f && Front.Pitch == -15.f);
    auto Above = Advance(Front.Yaw, Front.Pitch, 720.f, -1000.f);
    assert(std::abs(Above.Yaw) == 180.f && Above.Pitch == -55.f);
    assert(Advance(0, -15, 0, 1000).Pitch == 5.f);
    for (int Samples : {30, 60, 120})
    {
        CityOrbitCamera::State View{0.f, -15.f};
        for (int I = 0; I < Samples; ++I) View = Advance(View.Yaw, View.Pitch, 180.f / Samples, -20.f / Samples);
        assert(std::abs(std::abs(View.Yaw) - 180.f) < .01f);
        assert(std::abs(View.Pitch + 35.f) < .01f);
    }
    auto Wrapped = Advance(179, -15, 5, 0);
    assert(Wrapped.Yaw == -176.f);
    std::cout << "PASS: front view, full orbit, pitch limits and sample-rate-independent drag\n";
}
