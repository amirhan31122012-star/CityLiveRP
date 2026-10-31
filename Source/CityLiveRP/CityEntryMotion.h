#pragma once
#include <algorithm>
namespace CityLiveEntry
{
constexpr float Duration=6.8f;
inline float AdvanceTime(float Time,float Delta,bool Reverse)
{
    const float Step=std::clamp(Delta,0.f,1.f/15.f);
    return Time+(Reverse ? -Step : Step);
}
inline float Blend(float Time,float Start,float End)
{
    const float T=std::clamp((Time-Start)/(End-Start),0.f,1.f);
    return T*T*(3.f-2.f*T);
}
struct Pose
{
    float Door=0, Hand=0, Sit=0, FirstLeg=0, SecondLeg=0;
    float SeatShift=0, BodyTurn=0, Lean=0, Wheel=0;
};
inline Pose Enter(float T)
{
    Pose P;
    P.Door=Blend(T,1.f,1.7f)*(1-Blend(T,5.65f,6.4f));
    P.Hand=Blend(T,.5f,.95f)*(1-Blend(T,1.7f,1.95f))+
        Blend(T,5.15f,5.6f)*(1-Blend(T,6.4f,6.65f));
    P.Sit=Blend(T,2.05f,3.1f);
    P.FirstLeg=Blend(T,3.2f,3.95f);P.SecondLeg=Blend(T,4.05f,4.8f);
    P.SeatShift=Blend(T,6.4f,6.8f);
    P.BodyTurn=Blend(T,3.2f,5.25f);
    P.Lean=Blend(T,1.75f,2.05f)*(1-Blend(T,4.8f,5.4f));
    P.Wheel=Blend(T,6.4f,6.8f);
    return P;
}
inline Pose Exit(float T)
{
    Pose P;
    P.Door=Blend(T,.4f,1.1f)*(1-Blend(T,5.65f,6.4f));
    P.Hand=Blend(T,0.f,.35f)*(1-Blend(T,1.1f,1.4f))+
        Blend(T,5.15f,5.6f)*(1-Blend(T,6.4f,6.65f));
    P.Sit=1-Blend(T,3.3f,4.5f);
    P.FirstLeg=1-Blend(T,1.45f,2.2f);P.SecondLeg=1-Blend(T,2.3f,3.1f);
    P.SeatShift=1-Blend(T,.25f,1.1f);
    P.BodyTurn=1-Blend(T,1.15f,3.2f);
    P.Lean=Blend(T,1.1f,1.4f)*(1-Blend(T,4.5f,4.85f));
    P.Wheel=1-Blend(T,0.f,.25f);
    return P;
}
inline float FootTravel(float P) {return Blend(P,.2f,.8f);}
inline float FootLift(float P) {return Blend(P,0.f,.3f)*(1-Blend(P,.7f,1.f));}
}
