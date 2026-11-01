#include "CityTouchLayout.h"
#include "CityFloatingStick.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main()
{
    for(const auto& View: {std::pair<double,double>{1280,720},{1600,720},{2400,1080},{1024,768}})
    for(bool Vehicle:{false,true})
    {
        const auto L=CityTouchLayout::Build(View.first,View.second,Vehicle);
        assert(L.RightX+L.RunDraw<View.first&&L.RightY+L.RunDraw<View.second);
        assert(L.Radius==std::min(View.first,View.second)*.12); // preserve stick sensitivity
        assert(!CityLiveTouch::CanStartStick(static_cast<float>(L.InteractX),static_cast<float>(L.InteractY),static_cast<float>(View.first),static_cast<float>(View.second)));
        if(!Vehicle)assert(std::hypot(L.InteractX-L.RightX,L.InteractY-L.RightY)>L.Radius*.65+L.RunHit);
        else assert(L.RightX-L.Radius*2.2-L.Radius>View.first*.4);
    }
    std::cout<<"PASS: touch layout on 16:9, wide and tablet screens; distinct actions and unchanged floating-stick radius\n";
}
