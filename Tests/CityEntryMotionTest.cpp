#include "CityEntryMotion.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <string>
int main(int argc,char** argv)
{
    using namespace CityLiveEntry;
    std::cout<<std::setprecision(9);
    for(int I=0;I<=816;++I)
    {
        const float T=I/120.f;
        for(const bool Exiting:{false,true})
        {
            const Pose P=Exiting?Exit(T):Enter(T);
            for(float Value:{P.Door,P.Hand,P.Sit,P.FirstLeg,P.SecondLeg,P.SeatShift,P.BodyTurn,P.Lean,P.Wheel})
                assert(Value>=-.00001f && Value<=1.00001f);
            if(!Exiting && (P.FirstLeg>0 || P.SecondLeg>0)) assert(P.Sit==1);
            if(!Exiting && P.SeatShift>0)assert(P.FirstLeg==1 && P.SecondLeg==1);
            if(Exiting && P.Sit<1)assert(P.FirstLeg==0 && P.SecondLeg==0);
            if((P.Sit>0 && P.Sit<1) || (P.FirstLeg>0 && P.FirstLeg<1) || (P.SecondLeg>0 && P.SecondLeg<1))assert(P.Door>.999f);
            const float Previous=Exiting?Exit(T-.001f).Door:Enter(T-.001f).Door;
            if(std::abs(P.Door-Previous)>.00001f)assert(P.Hand>.999f);
            if(argc>1 && std::string(argv[1])=="--dump")
                std::cout<<Exiting<<' '<<T<<' '<<P.Door<<' '<<P.Hand<<' '<<P.Sit<<' '<<P.FirstLeg<<' '<<P.SecondLeg<<' '<<P.SeatShift<<' '<<P.BodyTurn<<' '<<P.Lean<<' '<<P.Wheel<<'\n';
        }
    }
    assert(AdvanceTime(2,10,false)-2<.067f);
    assert(AdvanceTime(2,-1,false)==2);
    assert(AdvanceTime(2,.01f,true)<2);
    assert(Enter(0).Sit==0 && Enter(Duration).Sit==1);
    assert(Exit(0).Sit==1 && Exit(Duration).Sit==0);
    assert(Enter(3.15f).Sit==1 && Enter(3.15f).FirstLeg==0);
    assert(Enter(4.f).FirstLeg==1 && Enter(4.f).SecondLeg==0);
    assert(Exit(3.2f).FirstLeg==0 && Exit(3.2f).SecondLeg==0 && Exit(3.2f).Sit==1);
    assert(Enter(Duration).Door==0 && Exit(Duration).Door==0);
    assert(FootLift(0)==0 && FootLift(1)==0);
    for(int I=30;I<=70;++I)assert(FootLift(I/100.f)>.999f);
    if(argc==1)std::cout<<"PASS: pelvis before separate legs, both feet before rise, handle during door motion, continuous phase bounds\n";
}
