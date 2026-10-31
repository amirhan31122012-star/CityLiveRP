#pragma once
#include <cmath>
#include <cstdint>
#include <string_view>
namespace CityDistrictRules
{
enum class HousingClass { Economy, Standard, Premium };
constexpr int CourierReward=5000;
constexpr int Price(HousingClass Class)
{
    switch(Class){case HousingClass::Economy:return 150000;case HousingClass::Standard:return 350000;default:return 500000;}
}
inline bool CanPurchase(int Balance,int Cost,bool Owned)
{return !Owned && Cost>0 && Balance>=Cost;}
inline bool ValidIdentifier(std::string_view Id)
{
    if(Id.empty()||Id.size()>80)return false;
    for(char C:Id)if(!((C>='a'&&C<='z')||(C>='A'&&C<='Z')||(C>='0'&&C<='9')||C=='_'||C==':'))return false;
    return true;
}
inline bool CanDeliver(bool Working,bool Active,bool OnScooter,double Distance,double Speed,int Balance)
{
    return Working && Active && OnScooter && std::isfinite(Distance) && std::isfinite(Speed) &&
        Distance>=0 && Distance<=250 && std::abs(Speed)<=80 && Balance>=0 && Balance<=INT32_MAX-CourierReward;
}
}
