#pragma once
#include <algorithm>
#include <cmath>

namespace CityLiveCrouch
{
// Triangle coordinates for a knee, preserving the two animated leg lengths.
inline bool KneeOffset(double Upper, double Lower, double Distance, double& Along, double& Bend)
{
    if (Upper < 0.001 || Lower < 0.001 || Distance < 0.001) return false;
    const double Reach = std::clamp(Distance, std::abs(Upper - Lower) + 0.0001, Upper + Lower - 0.0001);
    Along = (Upper * Upper - Lower * Lower + Reach * Reach) / (2.0 * Reach);
    Bend = std::sqrt(std::max(0.0, Upper * Upper - Along * Along));
    return true;
}
}
