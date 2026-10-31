#pragma once
#include <string_view>
namespace CityProfileRules
{
constexpr int StartingBalance = 20000;
constexpr int PasswordIterations = 100000;
inline bool ValidName(std::string_view Name)
{
    if (Name.size() < 2 || Name.size() > 20) return false;
    for (const char C : Name)
        if (!((C >= 'A' && C <= 'Z') || (C >= 'a' && C <= 'z'))) return false;
    return true;
}
}
