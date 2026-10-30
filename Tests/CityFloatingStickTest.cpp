#include "CityFloatingStick.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main()
{
    using namespace CityLiveTouch;
    assert(CanStartStick(100, 700, 2000, 1000));
    assert(CanStartStick(800, 700, 2000, 1000));
    assert(!CanStartStick(1100, 700, 2000, 1000));
    assert(!CanStartStick(100, 100, 2000, 1000));
    assert(!CanStartStick(100, 500, 2000, 1000));
    assert(!CanStartStick(850, 700, 2000, 1000));
    FloatingStick Stick;
    for (auto Start : {Point{100, 500}, Point{800, 700}, Point{1, 999}})
    {
        Stick.Begin(Start.X, Start.Y, 2000, 1000);
        auto Axis = Stick.Sample(Start.X, Start.Y, 2000, 1000, 100);
        assert(Axis.X == 0 && Axis.Y == 0);
        Axis = Stick.Sample(Start.X + 200, Start.Y, 2000, 1000, 100);
        assert(std::abs(Axis.X - 1) < .001 && Axis.Y == 0);
        Axis = Stick.Sample(Start.X + 200, Start.Y - 200, 2000, 1000, 100);
        assert(std::abs(std::sqrt(Axis.X*Axis.X + Axis.Y*Axis.Y)-1) < .001);
        Axis = Stick.Sample(Start.X + 1, Start.Y + 1, 2000, 1000, 100);
        assert(Axis.X == 0 && Axis.Y == 0);
        const auto Resized = Stick.Center(1000, 500);
        assert(std::abs(Resized.X - Start.X*.5f) < .001);
        assert(std::abs(Resized.Y - Start.Y*.5f) < .001);
    }
    std::cout << "PASS: floating origin, deadzone, radial clamping, left touch region, resize\n";
}
