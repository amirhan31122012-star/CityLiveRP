#include "ProceduralCrouchMath.h"
#include <cassert>
#include <iostream>

int main()
{
    // A lowering pelvis must bend the knee without stretching either leg segment.
    for (double Upper : {40., 45., 50.})
    for (double Lower : {40., 45., 50.})
    for (double Distance : {25., 35., 45., 55., 65., 75.})
    {
        double X, Y;
        assert(CityLiveCrouch::KneeOffset(Upper, Lower, Distance, X, Y));
        assert(std::abs(std::hypot(X, Y) - Upper) < 1e-8);
        assert(std::abs(std::hypot(Distance - X, Y) - Lower) < 1e-8);
        assert(Y > 0.);
    }
    std::cout << "PASS: 54 shallow and deep crouch poses preserve leg lengths and ankle target\n";
    double LastBend = 0.;
    for (double Distance = 88.; Distance >= 55.; Distance -= 1.)
    {
        double X, Y;
        assert(CityLiveCrouch::KneeOffset(45., 45., Distance, X, Y));
        assert(Y > LastBend);
        LastBend = Y;
    }
    std::cout << "PASS: lowering the hip progressively increases knee bend\n";
    double X, Y;
    assert(!CityLiveCrouch::KneeOffset(0., 45., 60., X, Y));
    assert(!CityLiveCrouch::KneeOffset(45., 45., 0., X, Y));
    assert(CityLiveCrouch::KneeOffset(45., 45., 150., X, Y));
    assert(std::isfinite(X) && std::isfinite(Y));
    std::cout << "PASS: invalid limbs rejected and excessive reach remains finite\n";
}
