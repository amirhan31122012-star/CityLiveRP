#pragma once
#include <algorithm>
namespace CityTouchLayout
{
struct Layout{double LeftX,LeftY,RightX,RightY,Radius,InteractX,InteractY,RunHit,RunDraw;};
inline Layout Build(double Width,double Height,bool Vehicle)
{
    const double R=std::min(Width,Height)*.12;
    return {Width*.18,Height*.76,Width*(Vehicle?.91:.93),Height*(Vehicle?.80:.84),R,Width*.76,Height*.84,R*(Vehicle?1:.70),R*(Vehicle?.85:.65)};
}
}
