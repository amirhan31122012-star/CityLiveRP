#include "CityDistrictRules.h"
#include <cassert>
#include <limits>
#include <iostream>
int main()
{
    using namespace CityDistrictRules;
    assert(Price(HousingClass::Economy)==150000);
    assert(Price(HousingClass::Standard)==350000);
    assert(Price(HousingClass::Premium)==500000);
    assert(!CanPurchase(20000,150000,false));assert(CanPurchase(150000,150000,false));
    assert(!CanPurchase(500000,150000,true));assert(!CanPurchase(20000,-1,false));
    assert(ValidIdentifier("North_A:5:4"));assert(!ValidIdentifier("../account"));
    assert(CanDeliver(true,true,true,150,0,20000));
    assert(!CanDeliver(true,false,true,150,0,25000));
    assert(!CanDeliver(true,true,false,150,0,20000));
    assert(!CanDeliver(true,true,true,300,0,20000));
    assert(!CanDeliver(true,true,true,100,200,20000));
    assert(!CanDeliver(false,true,true,100,0,20000));
    assert(!CanDeliver(true,true,true,100,0,INT32_MAX));
    assert(!CanDeliver(true,true,true,std::numeric_limits<double>::quiet_NaN(),0,20000));
    assert(!CanDeliver(true,true,true,100,std::numeric_limits<double>::infinity(),20000));
    std::cout<<"PASS: housing prices, funds/ownership checks, courier eligibility, stop distance and overflow\n";
}
