#pragma once
#include <algorithm>
#include <cmath>

namespace CityLiveTouch
{
struct Point { float X = 0.f; float Y = 0.f; };
inline bool CanStartStick(float X, float Y, float Width, float Height)
{
    return Width > 0 && Height > 0 && X >= 0 && X <= Width * .40f && Y >= Height * .55f && Y <= Height;
}
struct FloatingStick
{
    Point Anchor;
    void Begin(float X, float Y, float Width, float Height)
    {
        Anchor = {X / std::max(Width, 1.f), Y / std::max(Height, 1.f)};
    }
    Point Center(float Width, float Height) const { return {Anchor.X * Width, Anchor.Y * Height}; }
    Point Sample(float X, float Y, float Width, float Height, float Radius) const
    {
        const Point Origin = Center(Width, Height);
        float DX = (X - Origin.X) / std::max(Radius, 1.f);
        float DY = (Y - Origin.Y) / std::max(Radius, 1.f);
        const float Length = std::sqrt(DX*DX + DY*DY);
        if (Length < .12f) return {};
        const float Divisor = std::max(Length, 1.f);
        return {DX / Divisor, DY / Divisor};
    }
};
}
