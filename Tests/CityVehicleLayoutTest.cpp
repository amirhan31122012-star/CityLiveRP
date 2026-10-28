#include "CityVehicleLayout.h"
#include <cassert>
#include <iostream>

int main()
{
    using namespace CityVehicleLayout;
    assert(SelectPassenger(-25, -180, {{false, false, false}}) == 2);
    assert(SelectPassenger(-25, 180, {{false, false, false}}) == 0);
    assert(SelectPassenger(-100, 180, {{false, false, false}}) == 1);
    assert(SelectPassenger(-25, 180, {{true, false, false}}) == 1);
    assert(SelectPassenger(-25, -180, {{false, false, true}}) == 0);
    assert(SelectPassenger(-25, 180, {{true, true, false}}) == 2);
    assert(SelectPassenger(-25, 180, {{true, true, true}}) == -2);
    assert(Side(-1) < 0 && Door(-1) == 0);
    assert(Side(2) < 0 && Door(2) == 2);
    assert(Side(0) > 0 && Door(0) == 1);
    assert(Side(1) > 0 && Door(1) == 3);
    std::cout << "PASS: same-side passenger seats, nearest free seat, occupied-seat fallback, full car and four door mapping\n";
}
