#include "CityEntryMotion.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main()
{
    using namespace CityLiveEntry;
    for (int Frame = 0; Frame <= 600; ++Frame)
    {
        const float T = Frame / 120.f;
        assert(Door(T) >= 0.f && Door(T) <= 1.f);
        assert(Hand(T) >= 0.f && Hand(T) <= 1.f);
        assert(Sit(T) >= 0.f && Sit(T) <= 1.f);
        if (Sit(T) > 0.f && Sit(T) < 1.f) assert(Door(T) > .99f);
        if (Door(T) > 0.f && T < 1.95f) assert(Hand(T) > .99f);
        if (T > 2.f) assert(Hand(T) < .99f);
    }
    assert(Door(0.f) == 0.f && Hand(0.f) == 0.f && Sit(0.f) == 0.f);
    assert(Door(Duration) == 0.f && Hand(Duration) == 0.f && Sit(Duration) == 1.f);
    assert(Blend(.0001f, 0.f, 1.f) < .00001f);
    assert(Sit(2.85f) == 1.f && Leg(2.85f, false) == 0.f);
    assert(Leg(3.4f, false) == 1.f && Leg(3.4f, true) < 1.f);
    assert(Leg(4.f, false) == 1.f && Leg(4.f, true) == 1.f && Door(4.f) == 1.f);
    // Exit runs the same phases backwards: feet outside before the torso stands up.
    assert(Sit(3.f) == 1.f && Leg(3.f, true) == 0.f);
    assert(Sit(1.99f) == 0.f && Door(1.99f) > .99f);
    std::cout << "PASS: hand/door timing, torso before separate legs on entry, legs before torso on exit, smooth boundaries\n";
}
