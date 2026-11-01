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
    assert(!CityOrbitCamera::ShouldFollow(0,false,10));
    assert(!CityOrbitCamera::ShouldFollow(100,true,10));
    assert(!CityOrbitCamera::ShouldFollow(100,false,1));
    assert(CityOrbitCamera::ShouldFollow(-100,false,2));
    for(int Fps:{15,30,60,120})
    {
        CityOrbitCamera::State Followed{170.f,-45.f};
        for(int Frame=0;Frame<Fps*2;++Frame)Followed=CityOrbitCamera::Follow(Followed.Yaw,Followed.Pitch,-15.f,1.f/Fps);
        assert(std::abs(Followed.Yaw)<.2f && std::abs(Followed.Pitch+15)<.04f);
    }
    const auto Still=CityOrbitCamera::Follow(120,-35,-15,0);
    assert(Still.Yaw==120 && Still.Pitch==-35);
    const auto Short=CityOrbitCamera::Follow(350,-15,-15,.1f);
    assert(Short.Yaw<0 && Short.Yaw>-10);
    auto Wrapped = Advance(179, -15, 5, 0);
    assert(Wrapped.Yaw == -176.f);
    std::cout << "PASS: front view, full orbit, pitch limits and sample-rate-independent drag\n";
}
