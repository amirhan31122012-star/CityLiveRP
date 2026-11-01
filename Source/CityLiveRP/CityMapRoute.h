#pragma once
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace CityMap
{
struct Point{double X=0,Y=0;};
inline Point operator+(Point A,Point B){return {A.X+B.X,A.Y+B.Y};}
inline Point operator-(Point A,Point B){return {A.X-B.X,A.Y-B.Y};}
inline Point operator*(Point A,double S){return {A.X*S,A.Y*S};}
inline double Dot(Point A,Point B){return A.X*B.X+A.Y*B.Y;}
inline double Length(Point A){return std::sqrt(Dot(A,A));}
inline double Distance(Point A,Point B){return Length(A-B);}
struct Rect{double X0,Y0,X1,Y1;};
struct Road{Point A,B;double Width;bool Access=false;};
inline Rect Expand(Rect R,double Pad){return {R.X0-Pad,R.Y0-Pad,R.X1+Pad,R.Y1+Pad};}
inline bool Inside(Point P,Rect R){return P.X>R.X0+1e-6&&P.X<R.X1-1e-6&&P.Y>R.Y0+1e-6&&P.Y<R.Y1-1e-6;}
inline bool Finite(Point P){return std::isfinite(P.X)&&std::isfinite(P.Y);}
inline Point Project(Point P,Point A,Point B)
{const Point D=B-A;const double L=Dot(D,D);return L<1e-12?A:A+D*std::clamp(Dot(P-A,D)/L,0.,1.);}
inline bool OnSegment(Point P,Point A,Point B){return Distance(P,Project(P,A,B))<1e-4;}
inline bool Crosses(Point A,Point B,Rect R)
{
    R=Expand(R,-.001);double Low=0,High=1;const Point D=B-A;
    for(int Axis=0;Axis<2;++Axis)
    {
        const double P=Axis?A.Y:A.X,V=Axis?D.Y:D.X,Min=Axis?R.Y0:R.X0,Max=Axis?R.Y1:R.X1;
        if(std::abs(V)<1e-10){if(P<=Min||P>=Max)return false;continue;}
        double U=(Min-P)/V,W=(Max-P)/V;if(U>W)std::swap(U,W);
        Low=std::max(Low,U);High=std::min(High,W);if(Low>=High)return false;
    }
    return High>0&&Low<1;
}
inline bool Blocked(Point A,Point B,const std::vector<Rect>& Obstacles,double Padding=1.2)
{for(Rect R:Obstacles)if(Crosses(A,B,Expand(R,Padding)))return true;return false;}
inline Point Outdoors(Point P,const std::vector<Rect>& Obstacles)
{
    for(int Pass=0;Pass<4;++Pass)for(Rect R:Obstacles)
    {
        R=Expand(R,1.21);if(!Inside(P,R))continue;
        const Point Choices[]={{R.X0,P.Y},{R.X1,P.Y},{P.X,R.Y0},{P.X,R.Y1}};
        Point Best=Choices[0];for(Point Q:Choices)if(Distance(P,Q)<Distance(P,Best))Best=Q;P=Best;
    }
    return P;
}
inline std::vector<int> Shortest(const std::vector<std::vector<double>>& Edges,int From,int To)
{
    const int N=static_cast<int>(Edges.size());const double Inf=std::numeric_limits<double>::infinity();
    std::vector<double> Cost(N,Inf);std::vector<int> Previous(N,-1);std::vector<bool> Done(N,false);Cost[From]=0;
    for(int Step=0;Step<N;++Step)
    {
        int Current=-1;for(int I=0;I<N;++I)if(!Done[I]&&(Current<0||Cost[I]<Cost[Current]))Current=I;
        if(Current<0||!std::isfinite(Cost[Current]))break;
        if(Current==To)break;
        Done[Current]=true;
        for(int Next=0;Next<N;++Next)if(Edges[Current][Next]>=0&&Cost[Current]+Edges[Current][Next]<Cost[Next])
        {Cost[Next]=Cost[Current]+Edges[Current][Next];Previous[Next]=Current;}
    }
    if(!std::isfinite(Cost[To]))return {};
    std::vector<int> Path;for(int I=To;I!=-1;I=Previous[I]){Path.push_back(I);if(I==From)break;}
    std::reverse(Path.begin(),Path.end());return Path;
}
inline std::vector<Point> Walk(Point A,Point B,const std::vector<Rect>& Obstacles)
{
    A=Outdoors(A,Obstacles);B=Outdoors(B,Obstacles);
    if(!Blocked(A,B,Obstacles))return {A,B};
    std::vector<Point> Nodes{A,B};
    for(Rect R:Obstacles){R=Expand(R,1.22);for(Point P:std::vector<Point>{{R.X0,R.Y0},{R.X1,R.Y0},{R.X1,R.Y1},{R.X0,R.Y1}})
        {bool Valid=true;for(Rect Other:Obstacles)if(Inside(P,Expand(Other,1.2)))Valid=false;if(Valid)Nodes.push_back(P);}}
    std::vector<std::vector<double>> Edges(Nodes.size(),std::vector<double>(Nodes.size(),-1));
    for(size_t I=0;I<Nodes.size();++I)for(size_t J=I+1;J<Nodes.size();++J)
        if(!Blocked(Nodes[I],Nodes[J],Obstacles))Edges[I][J]=Edges[J][I]=Distance(Nodes[I],Nodes[J]);
    std::vector<Point> Result;for(int I:Shortest(Edges,0,1))Result.push_back(Nodes[I]);return Result;
}
inline Point RoadProjection(Point P,const std::vector<Road>& Roads,const std::vector<Rect>& Obstacles)
{
    Point Best{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::quiet_NaN()};double Cost=std::numeric_limits<double>::infinity();
    for(const Road& R:Roads)
    {
        std::vector<Point> Candidates{Project(P,R.A,R.B),R.A,R.B};
        for(Rect B:Obstacles)
        {
            B=Expand(B,1.25);
            for(Point C:std::vector<Point>{{B.X0,P.Y},{B.X1,P.Y},{P.X,B.Y0},{P.X,B.Y1}})Candidates.push_back(Project(C,R.A,R.B));
        }
        for(Point Q:Candidates)
        {
            bool Valid=true;for(Rect B:Obstacles)if(Inside(Q,Expand(B,1.2)))Valid=false;
            if(Valid&&Distance(P,Q)<Cost){Best=Q;Cost=Distance(P,Q);}
        }
    }
    return Best;
}
struct Route
{
    std::vector<Point> Points;
    std::vector<bool> RoadLeg;
    double Metres=0;
    bool Valid=false;
    void Append(Point P,bool OnRoad)
    {if(!Points.empty()){const double L=Distance(Points.back(),P);if(L<1e-5)return;Metres+=L;RoadLeg.push_back(OnRoad);}Points.push_back(P);}
};
inline Route Build(Point Start,Point Target,const std::vector<Road>& Roads,const std::vector<Rect>& Obstacles)
{
    Route Result;if(!Finite(Start)||!Finite(Target)||Roads.empty())return Result;
    Start=Outdoors(Start,Obstacles);Target=Outdoors(Target,Obstacles);
    const Point A=RoadProjection(Start,Roads,Obstacles),B=RoadProjection(Target,Roads,Obstacles);
    if(!Finite(A)||!Finite(B))return Result;
    struct Approach{Point RoadPoint;std::vector<Point> WalkPath;double Walking=0;int Node=-1;};
    std::vector<Approach> Approaches;
    for(const auto& R:Roads)
    {
        const Point P=RoadProjection(Target,{R},Obstacles);if(!Finite(P))continue;
        bool Duplicate=false;for(const auto& C:Approaches)if(Distance(C.RoadPoint,P)<.001)Duplicate=true;if(Duplicate)continue;
        auto Walking=Walk(P,Target,Obstacles);if(Walking.empty())continue;double Metres=0;
        for(size_t I=1;I<Walking.size();++I)Metres+=Distance(Walking[I-1],Walking[I]);
        if(Metres<=100)Approaches.push_back({P,std::move(Walking),Metres,-1});
    }
    if(Approaches.empty())return Result;
    std::vector<Point> Nodes;
    auto Add=[&](Point P){for(size_t I=0;I<Nodes.size();++I)if(Distance(P,Nodes[I])<1e-5)return static_cast<int>(I);Nodes.push_back(P);return static_cast<int>(Nodes.size()-1);};
    const int From=Add(A),Nearest=Add(B);for(auto& C:Approaches)C.Node=Add(C.RoadPoint);
    for(const Road& R:Roads){Add(R.A);Add(R.B);}
    for(const Road& R:Roads)
    {
        for(const Road& S:Roads){Point P{R.A.X,S.A.Y};if(OnSegment(P,R.A,R.B)&&OnSegment(P,S.A,S.B))Add(P);}
        for(Rect Wall:Obstacles)
        {Wall=Expand(Wall,1.25);for(Point Corner:std::vector<Point>{{Wall.X0,Wall.Y0},{Wall.X1,Wall.Y0},{Wall.X1,Wall.Y1},{Wall.X0,Wall.Y1}})
            {const Point Q=Project(Corner,R.A,R.B);if(std::abs(Q.X-Corner.X)<1e-5||std::abs(Q.Y-Corner.Y)<1e-5)Add(Q);}}
    }
    const int To=static_cast<int>(Nodes.size());Nodes.push_back({1e6,1e6});
    std::vector<std::vector<double>> Edges(Nodes.size(),std::vector<double>(Nodes.size(),-1));
    for(const Road& R:Roads)
    {
        std::vector<int> Along;for(size_t I=0;I<Nodes.size();++I)if(OnSegment(Nodes[I],R.A,R.B))Along.push_back(static_cast<int>(I));
        std::sort(Along.begin(),Along.end(),[&](int I,int J){return Dot(Nodes[I]-R.A,R.B-R.A)<Dot(Nodes[J]-R.A,R.B-R.A);});
        for(size_t I=1;I<Along.size();++I)
        {const int U=Along[I-1],V=Along[I];if(!Blocked(Nodes[U],Nodes[V],Obstacles))Edges[U][V]=Edges[V][U]=Distance(Nodes[U],Nodes[V]);}
    }
    if(Shortest(Edges,From,Nearest).empty())return Result;
    // Prefer a useful approach to the entrance, rather than the nearest road
    // behind its building. Walking is weighted to keep the main path on streets.
    for(const auto& C:Approaches)Edges[C.Node][To]=C.Walking*4;
    const auto RoadPath=Shortest(Edges,From,To);if(RoadPath.size()<2)return Result;
    const Approach* Last=nullptr;for(const auto& C:Approaches)if(C.Node==RoadPath[RoadPath.size()-2])Last=&C;
    const auto First=Walk(Start,A,Obstacles);if(First.empty()||!Last)return Result;
    for(Point P:First)Result.Append(P,false);
    for(size_t I=0;I+1<RoadPath.size();++I)
    {
        const Point P=Nodes[RoadPath[I]];bool Asphalt=false;
        if(!Result.Points.empty())for(const auto& R:Roads)if(!R.Access&&OnSegment(Result.Points.back(),R.A,R.B)&&OnSegment(P,R.A,R.B))Asphalt=true;
        Result.Append(P,Asphalt);
    }
    for(Point P:Last->WalkPath)Result.Append(P,false);
    Result.Valid=true;return Result;
}

// Convex clipping for the circular mini-map; all fill geometry stays inside it.
inline std::vector<Point> ClipCircle(std::vector<Point> Polygon,Point Centre,double Radius,int Sides=48)
{
    if(Polygon.empty())return {};
    bool AllInside=true;double X0=Polygon[0].X,X1=X0,Y0=Polygon[0].Y,Y1=Y0;
    const double Inner=Radius*std::cos(3.141592653589793/Sides);
    for(Point P:Polygon){AllInside=AllInside&&Distance(P,Centre)<=Inner;X0=std::min(X0,P.X);X1=std::max(X1,P.X);Y0=std::min(Y0,P.Y);Y1=std::max(Y1,P.Y);}
    if(AllInside)return Polygon;
    const Point Nearest{std::clamp(Centre.X,X0,X1),std::clamp(Centre.Y,Y0,Y1)};
    if(Distance(Nearest,Centre)>Radius)return {};
    for(int Side=0;Side<Sides&&!Polygon.empty();++Side)
    {
        const double T=2*3.141592653589793*Side/Sides;
        const Point N{std::cos(T),std::sin(T)};const double Limit=Radius*std::cos(3.141592653589793/Sides);
        std::vector<Point> Out;
        Point A=Polygon.back();double DA=Dot(A-Centre,N)-Limit;
        for(Point B:Polygon)
        {const double DB=Dot(B-Centre,N)-Limit;if((DA<=0)!=(DB<=0)){const double U=DA/(DA-DB);Out.push_back(A+(B-A)*U);}if(DB<=0)Out.push_back(B);A=B;DA=DB;}
        Polygon=std::move(Out);
    }
    return Polygon;
}
}
