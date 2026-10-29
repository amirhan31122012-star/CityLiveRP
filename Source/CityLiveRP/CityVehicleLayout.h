#pragma once
#include <array>
#include <limits>

namespace CityVehicleLayout
{
constexpr float SeatRootZ = -20.f;
constexpr float SeatEdgeY = 78.f;
constexpr float OutsideFootY = 130.f;
constexpr float InsideFloorZ = -33.f;
inline float Side(int Seat) { return Seat < 0 || Seat == 2 ? -1.f : 1.f; }
inline float X(int Seat) { return Seat == 1 || Seat == 2 ? -95.f : -25.f; }
inline int Door(int Seat) { return Seat < 0 ? 0 : Seat == 0 ? 1 : Seat == 1 ? 3 : 2; }
inline int SelectPassenger(double XPosition, double YPosition, const std::array<bool, 3>& Occupied)
{
    int Best = -2;
    double BestDistance = std::numeric_limits<double>::max();
    for (int Seat = 0; Seat < 3; ++Seat)
    {
        if (Occupied[Seat]) continue;
        const double DX = XPosition - X(Seat), DY = YPosition - Side(Seat) * 150.f;
        const bool SameSide = YPosition * Side(Seat) >= 0.;
        const double Distance = DX * DX + DY * DY + (SameSide ? 0. : 1.e9);
        if (Distance < BestDistance) { Best = Seat; BestDistance = Distance; }
    }
    return Best;
}
}
