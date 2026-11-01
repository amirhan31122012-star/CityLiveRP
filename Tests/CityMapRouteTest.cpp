#include "CityDistrictMapData.h"
#include <cassert>
#include <iostream>
#include <random>
#include <iomanip>
int main(int argc,char**)
{
    using namespace CityMap;const auto& Roads=CityDistrictMapData::Roads();const auto& Walls=CityDistrictMapData::Buildings();
    int Checked=0;
    for(Point Start:std::vector<Point>{{0,0},{-78,-24},{-140,100},{140,80}})
    for(const auto& Place:CityDistrictMapData::Places())
    {
        const auto Route=Build(Start,Place.Position,Roads,Walls);
        if(!Route.Valid){std::cerr<<"No route to "<<Place.Id<<" from "<<Start.X<<","<<Start.Y<<"\n";return 1;}
        assert(Route.Points.size()==Route.RoadLeg.size()+1);double Length=0;
        for(size_t I=1;I<Route.Points.size();++I)
        {
            Point A=Route.Points[I-1],B=Route.Points[I];assert(Finite(A)&&Finite(B));assert(!Blocked(A,B,Walls));Length+=Distance(A,B);
            if(Route.RoadLeg[I-1]){bool Found=false;for(const auto& Road:Roads)if(OnSegment(A,Road.A,Road.B)&&OnSegment(B,Road.A,Road.B))Found=true;assert(Found);}
        }
        assert(std::abs(Length-Route.Metres)<1e-6);assert(Distance(Route.Points.back(),Outdoors(Place.Position,Walls))<.001);++Checked;
        if(argc>1 && Start.X==-78)
        {
            std::cout<<std::setprecision(8)<<"{\"id\":\""<<Place.Id<<"\",\"metres\":"<<Route.Metres<<",\"points\":[";
            for(size_t I=0;I<Route.Points.size();++I){if(I)std::cout<<",";std::cout<<"["<<Route.Points[I].X<<","<<Route.Points[I].Y<<"]";}
            std::cout<<"],\"road\":[";for(size_t I=0;I<Route.RoadLeg.size();++I){if(I)std::cout<<",";std::cout<<(Route.RoadLeg[I]?"true":"false");}std::cout<<"]}\n";
        }
    }
    const auto Straight=Build({-100,0},{-20,0},Roads,Walls);assert(Straight.Valid&&std::abs(Straight.Metres-80)<.001);
    const auto FrontDoor=Build({-78,-24},{56.2,-62},Roads,Walls);assert(FrontDoor.Valid&&FrontDoor.Metres<300);
    assert(!Build({NAN,0},{0,0},Roads,Walls).Valid);
    assert(!Build({0,0},{0,0},{},Walls).Valid);
    std::vector<Road> Disconnected={{{0,0},{10,0},4},{{100,0},{110,0},4}};
    assert(!Build({0,0},{110,0},Disconnected,{}).Valid);
    assert(!Build({0,0},{110,0},Disconnected,{{-5,-5,115,5}}).Valid);
    std::mt19937 Gen(73);std::uniform_real_distribution<double> Sample(-200,200);
    for(int I=0;I<300;++I)
    {
        std::vector<Point> Polygon;for(int J=0;J<4;++J)Polygon.push_back({Sample(Gen),Sample(Gen)});
        for(Point P:ClipCircle(Polygon,{0,0},80))assert(Finite(P)&&Length(P)<=80.00001);
    }
    if(argc==1)std::cout<<"PASS: "<<Checked<<" district routes, road adherence, building avoidance, disconnected roads and circular clipping\n";
}
