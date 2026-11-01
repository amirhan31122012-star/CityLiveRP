#include "SCityMap.h"
#include "CityLiveRPPlayerController.h"
#include "CityDistrictGameplayComponent.h"
#include "CityDistrictMapData.h"
#include "CityUiTheme.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "Misc/CoreDelegates.h"

void SCityMap::Construct(const FArguments& Args)
{
    Controller=Args._Controller;bMini=Args._Mini;OnSelection=Args._OnSelection;ForceVolatile(true);
    FCoreDelegates::ApplicationWillDeactivateDelegate.AddSP(this,&SCityMap::ResetGestures);
    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddSP(this,&SCityMap::ResetGestures);
}
void SCityMap::ResetGestures(){Touches.Empty();bDragging=false;bMoved=false;bGesture=false;PinchDistance=0;}
void SCityMap::OnMouseCaptureLost(const FCaptureLostEvent& E)
{Touches.Remove(E.PointerIndex);bDragging=false;bGesture=true;SLeafWidget::OnMouseCaptureLost(E);}
double SCityMap::ScaleFor(const FGeometry& G) const
{const auto S=G.GetLocalSize();return bMini?FMath::Min(S.X,S.Y)/145.:FMath::Min(S.X/460.,S.Y/380.)*Zoom;}
CityMap::Point SCityMap::Centre() const
{if(bMini&&Controller.IsValid()){const FVector P=Controller->DistrictGameplay->MapPosition();return {P.X/100,P.Y/100};}return Camera;}
CityMap::Point SCityMap::ToWorld(const FGeometry& G,FVector2D P) const
{const auto D=(P-G.GetLocalSize()*.5)/ScaleFor(G);return Centre()+CityMap::Point{D.X,D.Y};}
void SCityMap::CentreOnPlayer()
{if(Controller.IsValid()){const auto P=Controller->DistrictGameplay->MapPosition();Camera={P.X/100,P.Y/100};}}
void SCityMap::ZoomBy(double Factor){Zoom=FMath::Clamp(Zoom*Factor,.75,5.);}
void SCityMap::ZoomAt(const FGeometry& G,FVector2D P,double Factor)
{const auto Before=ToWorld(G,P);ZoomBy(Factor);const auto After=ToWorld(G,P);Camera=Camera+(Before-After);}
void SCityMap::Pan(const FGeometry& G,FVector2D D)
{Camera=Camera-CityMap::Point{D.X/ScaleFor(G),D.Y/ScaleFor(G)};Camera.X=FMath::Clamp(Camera.X,-230.,230.);Camera.Y=FMath::Clamp(Camera.Y,-190.,190.);}
void SCityMap::Select(const FGeometry& G,FVector2D P)
{
    if(!Controller.IsValid())return;
    if(bMini){Controller->DistrictGameplay->OpenMap();return;}
    auto World=ToWorld(G,P);if(std::abs(World.X)>210||std::abs(World.Y)>170)return;
    FString Label=TEXT("Своя метка");double Best=24./ScaleFor(G);
    for(const auto& Place:CityDistrictMapData::Places())
        if(CityMap::Distance(World,Place.Position)<Best){Best=CityMap::Distance(World,Place.Position);Label=UTF8_TO_TCHAR(Place.Label);World=Place.Position;}
    World=CityMap::Outdoors(World,CityDistrictMapData::Buildings());
    OnSelection.ExecuteIfBound(FVector(World.X*100,World.Y*100,100),Label);
}
FReply SCityMap::OnMouseButtonDown(const FGeometry& G,const FPointerEvent& E)
{if(E.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();bDragging=true;bMoved=false;DragStart=PreviousPointer=G.AbsoluteToLocal(E.GetScreenSpacePosition());return FReply::Handled().CaptureMouse(SharedThis(this));}
FReply SCityMap::OnMouseMove(const FGeometry& G,const FPointerEvent& E)
{if(!bDragging)return FReply::Unhandled();const auto P=G.AbsoluteToLocal(E.GetScreenSpacePosition());if((P-DragStart).Size()>5)bMoved=true;if(!bMini&&bMoved)Pan(G,P-PreviousPointer);PreviousPointer=P;return FReply::Handled();}
FReply SCityMap::OnMouseButtonUp(const FGeometry& G,const FPointerEvent& E)
{if(!bDragging)return FReply::Unhandled();bDragging=false;if(!bMoved)Select(G,G.AbsoluteToLocal(E.GetScreenSpacePosition()));return FReply::Handled().ReleaseMouseCapture();}
FReply SCityMap::OnMouseWheel(const FGeometry& G,const FPointerEvent& E)
{if(!bMini)ZoomAt(G,G.AbsoluteToLocal(E.GetScreenSpacePosition()),std::pow(1.18,static_cast<double>(E.GetWheelDelta())));return FReply::Handled();}
FReply SCityMap::OnTouchStarted(const FGeometry& G,const FPointerEvent& E)
{
    const auto P=G.AbsoluteToLocal(E.GetScreenSpacePosition());Touches.Add(E.GetPointerIndex(),P);
    if(Touches.Num()==1){DragStart=PreviousPointer=P;bMoved=false;bGesture=false;}
    if(Touches.Num()==2){TArray<FVector2D> Values;Touches.GenerateValueArray(Values);PinchDistance=(Values[0]-Values[1]).Size();bGesture=true;}
    return FReply::Handled().CaptureMouse(SharedThis(this));
}
FReply SCityMap::OnTouchMoved(const FGeometry& G,const FPointerEvent& E)
{
    if(!Touches.Contains(E.GetPointerIndex()))return FReply::Unhandled();
    const auto P=G.AbsoluteToLocal(E.GetScreenSpacePosition());const auto Before=Touches[E.GetPointerIndex()];Touches[E.GetPointerIndex()]=P;
    if(Touches.Num()==2&&!bMini)
    {TArray<FVector2D> Values;Touches.GenerateValueArray(Values);const double Span=(Values[0]-Values[1]).Size();if(PinchDistance>8&&Span>8)ZoomAt(G,(Values[0]+Values[1])*.5,Span/PinchDistance);PinchDistance=Span;}
    else if(!bMini){if((P-DragStart).Size()>5)bMoved=true;if(bMoved)Pan(G,P-Before);}
    return FReply::Handled();
}
FReply SCityMap::OnTouchEnded(const FGeometry& G,const FPointerEvent& E)
{if(!Touches.Remove(E.GetPointerIndex()))return FReply::Unhandled();if(Touches.IsEmpty()&&!bMoved&&!bGesture)Select(G,G.AbsoluteToLocal(E.GetScreenSpacePosition()));return FReply::Handled().ReleaseMouseCapture();}

int32 SCityMap::OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const
{
    using CityMap::Point;const auto Size=G.GetLocalSize();const double Scale=ScaleFor(G);const auto Origin=Centre();
    const Point Mid{Size.X*.5,Size.Y*.5};const double Radius=FMath::Min(Size.X,Size.Y)*.48;
    auto Screen=[&](Point P){return Mid+(P-Origin)*Scale;};
    TArray<FSlateVertex> Vertices;TArray<SlateIndex> Indices;
    auto Polygon=[&](std::vector<Point> Points,FLinearColor Colour,bool World=true)
    {
        if(World)for(auto& P:Points)P=Screen(P);
        if(bMini)Points=CityMap::ClipCircle(std::move(Points),Mid,Radius);
        else
        {
            // Rectangular viewport clipping also bounds custom vertices during zoom.
            for(int Side=0;Side<4&&!Points.empty();++Side)
            {
                std::vector<Point> Clipped;Point A=Points.back();
                auto Distance=[&](Point P){return Side==0?P.X:Side==1?Size.X-P.X:Side==2?P.Y:Size.Y-P.Y;};
                double DA=Distance(A);for(Point B:Points){double DB=Distance(B);if((DA>=0)!=(DB>=0))Clipped.push_back(A+(B-A)*(DA/(DA-DB)));if(DB>=0)Clipped.push_back(B);A=B;DA=DB;}Points=std::move(Clipped);
            }
        }
        if(Points.size()<3)return;const int32 Base=Vertices.Num();const FColor Tint=Colour.ToFColor(true);
        for(Point P:Points)Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(G.GetAccumulatedRenderTransform(),FVector2f(static_cast<float>(P.X),static_cast<float>(P.Y)),FVector2f(0,0),Tint));
        for(size_t I=1;I+1<Points.size();++I){Indices.Add(static_cast<SlateIndex>(Base));Indices.Add(static_cast<SlateIndex>(Base+I));Indices.Add(static_cast<SlateIndex>(Base+I+1));}
    };
    auto Rect=[&](CityMap::Rect R,FLinearColor Colour){Polygon({{R.X0,R.Y0},{R.X1,R.Y0},{R.X1,R.Y1},{R.X0,R.Y1}},Colour);};
    auto Disc=[&](Point P,double R,FLinearColor Colour,bool World=true){std::vector<Point> Circle;for(int I=0;I<20;++I){double A=I*2*PI/20;Circle.push_back(P+Point{std::cos(A)*R,std::sin(A)*R});}Polygon(std::move(Circle),Colour,World);};
    auto Line=[&](Point A,Point B,double Width,FLinearColor Colour,bool World=true)
    {const Point Delta=B-A;const double L=CityMap::Length(Delta);if(L<.0001)return;const Point N{-Delta.Y/L*Width*.5,Delta.X/L*Width*.5};Polygon({A+N,B+N,B-N,A-N},Colour,World);};
    Polygon({{0,0},{Size.X,0},{Size.X,Size.Y},{0,Size.Y}},FLinearColor(.006f,.007f,.011f),false);
    Rect({-210,-170,210,170},FLinearColor(.012f,.016f,.019f));
    for(const auto& R:CityDistrictMapData::Walkways())Rect(R,FLinearColor(.032f,.038f,.047f));
    for(const auto& Area:CityDistrictMapData::Areas())Rect(Area.Bounds,Area.Kind?FLinearColor(.045f,.05f,.06f):FLinearColor(.075f,.052f,.032f));
    for(const auto& R:CityDistrictMapData::Roads())
    {if(R.Access)continue;Line(R.A,R.B,R.Width+1,FLinearColor(.17f,.18f,.21f));Line(R.A,R.B,R.Width,FLinearColor(.065f,.07f,.084f));}
    if(!bMini)for(const auto& R:CityDistrictMapData::ParkingLines())Rect(R,FLinearColor(.24f,.26f,.30f));
    for(Point P:CityDistrictMapData::Trees())Disc(P,2.5,FLinearColor(.026f,.064f,.042f));
    for(const auto& B:CityDistrictMapData::Buildings())
    {Rect({B.X0+1,B.Y0+1,B.X1+1,B.Y1+1},FLinearColor(.004f,.005f,.008f));Rect(B,FLinearColor(.12f,.13f,.16f));Rect(CityMap::Expand(B,-.6),FLinearColor(.075f,.081f,.10f));}
    for(const auto& R:CityDistrictMapData::RoofDetails())Rect(R,FLinearColor(.15f,.16f,.18f));
    if(PendingPin.IsSet()&&!bMini)
    {const auto P=Screen(PendingPin.GetValue());Disc(P,12,FLinearColor(.005f,.006f,.008f),false);Disc(P,8,CityUi::Orange(),false);Disc(P,3,FLinearColor::White,false);}
    if(Controller.IsValid())
    {
        const auto& Route=Controller->DistrictGameplay->MapRoute();
        for(size_t I=1;I<Route.Points.size();++I)
        {
            const Point A=Screen(Route.Points[I-1]),B=Screen(Route.Points[I]);
            if(Route.RoadLeg[I-1]){Line(A,B,bMini?5:6,FLinearColor(.006f,.007f,.01f),false);Line(A,B,bMini?2.7:3.5,CityUi::Orange(),false);}
            else{const double L=CityMap::Distance(A,B);for(double D=0;D<L;D+=8)Line(A+(B-A)*(D/L),A+(B-A)*(std::min(D+4.,L)/L),2,FLinearColor(1,.55f,.12f),false);}
        }
    }
    TArray<TPair<FVector2D,FString>> Labels;
    for(const auto& Place:CityDistrictMapData::Places())
    {
        const Point P=Screen(Place.Position);if(bMini&&CityMap::Distance(P,Mid)>Radius-12)continue;
        if(P.X<12||P.Y<12||P.X>Size.X-12||P.Y>Size.Y-12)continue;
        const FLinearColor Colour=Place.Symbol==3?FLinearColor(.32f,.65f,.23f):Place.Symbol==1?FLinearColor(.95f,.18f,.12f):Place.Symbol==0?FLinearColor(.15f,.43f,.85f):CityUi::Orange();
        Disc(P,bMini?7.5:11,FLinearColor(.008f,.009f,.012f),false);Disc(P,bMini?5.5:8,Colour,false);
        const double K=bMini?.75:1.1;
        auto Q=[&](double X,double Y){return P+Point{X*K,Y*K};};
        auto Mark=[&](double AX,double AY,double BX,double BY){Line(Q(AX,AY),Q(BX,BY),1.2*K,FLinearColor::White,false);};
        if(Place.Symbol==0)Polygon({Q(-4,-5),Q(4,-5),Q(3,2),Q(0,5),Q(-3,2)},FLinearColor::White,false);
        else if(Place.Symbol==1){Line(Q(-4,0),Q(4,0),2.3*K,FLinearColor::White,false);Line(Q(0,-4),Q(0,4),2.3*K,FLinearColor::White,false);}
        else if(Place.Symbol==3){Polygon({Q(-5,-1),Q(0,-5),Q(5,-1)},FLinearColor::White,false);Polygon({Q(-3.4,-1),Q(3.4,-1),Q(3.4,4),Q(-3.4,4)},FLinearColor::White,false);Polygon({Q(-1,1),Q(1,1),Q(1,4),Q(-1,4)},Colour,false);}
        else if(Place.Symbol==4){Mark(-2,4,-2,-4);Mark(-2,-4,2,-4);Mark(2,-4,3,-1);Mark(3,-1,-2,0);}
        else{Mark(-3.5,-3,3.5,-3);Mark(3.5,-3,3.5,4);Mark(3.5,4,-3.5,4);Mark(-3.5,4,-3.5,-3);if(Place.Symbol==2){Mark(-3.5,-3,0,0);Mark(0,0,3.5,-3);Mark(0,0,0,4);}else{Mark(-2,-3,-2,-5);Mark(-2,-5,2,-5);Mark(2,-5,2,-3);}}
        if(!bMini&&(Place.Symbol==0||Place.Symbol==1||Place.Symbol==2))Labels.Emplace(FVector2D(P.X+(Place.Symbol==2?-20:14),P.Y+(Place.Symbol==2?13:-6)),FString(UTF8_TO_TCHAR(Place.Label)));
    }
    if(Controller.IsValid())
    {
        FVector Goal;FString Name;if(Controller->DistrictGameplay->NavigationTarget(Goal,Name))
        {Point P=Screen({Goal.X/100,Goal.Y/100});if(bMini){const auto D=P-Mid;if(CityMap::Length(D)>Radius-12)P=Mid+D*((Radius-12)/CityMap::Length(D));}
            Disc(P,8,FLinearColor(.008f,.008f,.01f),false);Disc(P,5,FLinearColor(1,.06f,.025f),false);Polygon({P+Point{-5,3},P+Point{5,3},P+Point{0,13}},FLinearColor(1,.06f,.025f),false);}
        const auto Position=Controller->DistrictGameplay->MapPosition();const Point P=Screen({Position.X/100,Position.Y/100});
        const double Yaw=Controller->GetPawn()?Controller->GetPawn()->GetActorRotation().Yaw*PI/180:0;
        auto Turn=[&](Point Q){return P+Point{Q.X*std::cos(Yaw)-Q.Y*std::sin(Yaw),Q.X*std::sin(Yaw)+Q.Y*std::cos(Yaw)};};
        Disc(P,11,FLinearColor(.005f,.006f,.008f,.9f),false);Polygon({Turn({10,0}),Turn({-7,-6}),Turn({-4,0}),Turn({-7,6})},FLinearColor(1,.52f,.08f),false);
    }
    if(!Vertices.IsEmpty())
    {const auto Handle=FSlateApplication::Get().GetRenderer()->GetResourceHandle(*FCoreStyle::Get().GetBrush("WhiteBrush"));FSlateDrawElement::MakeCustomVerts(Out,Layer,Handle,Vertices,Indices,nullptr,0,0,ESlateDrawEffect::None);}
    for(const auto& Label:Labels)FSlateDrawElement::MakeText(Out,Layer+1,G.ToPaintGeometry(FVector2D(40,24),FSlateLayoutTransform(Label.Key)),Label.Value,FCoreStyle::GetDefaultFontStyle("Bold",12),ESlateDrawEffect::None,FLinearColor::White);
    if(!bMini)for(const auto& Caption:CityDistrictMapData::Captions())
    {
        const auto P=Screen(Caption.Position);
        if(P.X>120&&P.X<Size.X-120&&P.Y>10&&P.Y<Size.Y-30)
            FSlateDrawElement::MakeText(Out,Layer+1,G.ToPaintGeometry(FVector2D(240,24),FSlateLayoutTransform(FVector2D(P.X-78,P.Y))),FString(UTF8_TO_TCHAR(Caption.Text)),FCoreStyle::GetDefaultFontStyle("Bold",11),ESlateDrawEffect::None,FLinearColor(.45f,.48f,.53f));
    }
    if(bMini)
    {
        TArray<FVector2D> Rim;for(int I=0;I<=72;++I){double A=I*2*PI/72;Rim.Add(FVector2D(Mid.X+std::cos(A)*Radius,Mid.Y+std::sin(A)*Radius));}
        FSlateDrawElement::MakeLines(Out,Layer+2,G.ToPaintGeometry(),Rim,ESlateDrawEffect::None,FLinearColor(.7f,.73f,.78f),true,1.5);
    }
    return Layer+2;
}
