#include "CitySuspension.h"
#include <cassert>
#include <iostream>

int main()
{
    using CitySuspension::Force;
    assert(Force(0, 0) * 4.f == 1200.f * 980.f);
    assert(Force(10, 0) > Force(0, 0));
    assert(Force(0, -30) > Force(0, 0));
    assert(Force(0, 30) < Force(0, 0));
    assert(Force(-100, 0) == 0.f);
    assert(Force(1000, -1000) <= 1200.f * 980.f * 1.5f);
    assert(Force(0, 0, 1600.f) * 4.f == 1600.f * 980.f);
    std::cout << "PASS: static weight support, bump compression, damping, droop, force limit and chassis mass\n";
}
