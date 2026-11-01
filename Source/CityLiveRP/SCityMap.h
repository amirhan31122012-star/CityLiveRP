#pragma once
#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "CityMapRoute.h"
class ACityLiveRPPlayerController;
DECLARE_DELEGATE_TwoParams(FOnCityMapSelection,FVector,FString);
class SCityMap : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SCityMap):_Mini(false){}
        SLATE_ARGUMENT(ACityLiveRPPlayerController*,Controller)
        SLATE_ARGUMENT(bool,Mini)
        SLATE_EVENT(FOnCityMapSelection,OnSelection)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    virtual FVector2D ComputeDesiredSize(float) const override{return FVector2D(180,180);}
    virtual int32 OnPaint(const FPaintArgs&,const FGeometry&,const FSlateRect&,FSlateWindowElementList&,int32,const FWidgetStyle&,bool) const override;
    virtual FReply OnMouseButtonDown(const FGeometry&,const FPointerEvent&) override;
    virtual FReply OnMouseMove(const FGeometry&,const FPointerEvent&) override;
    virtual FReply OnMouseButtonUp(const FGeometry&,const FPointerEvent&) override;
    virtual FReply OnMouseWheel(const FGeometry&,const FPointerEvent&) override;
    virtual FReply OnTouchStarted(const FGeometry&,const FPointerEvent&) override;
    virtual FReply OnTouchMoved(const FGeometry&,const FPointerEvent&) override;
    virtual FReply OnTouchEnded(const FGeometry&,const FPointerEvent&) override;
    virtual void OnMouseCaptureLost(const FCaptureLostEvent&) override;
    void ZoomBy(double Factor);
    void CentreOnPlayer();
    void SetPendingPoint(FVector P){PendingPin=CityMap::Point{P.X/100,P.Y/100};}
    void ClearPendingPoint(){PendingPin.Reset();}
private:
    TWeakObjectPtr<ACityLiveRPPlayerController> Controller;
    FOnCityMapSelection OnSelection;
    bool bMini=false,bDragging=false,bMoved=false,bGesture=false;
    double Zoom=1,PinchDistance=0;
    CityMap::Point Camera;
    TOptional<CityMap::Point> PendingPin;
    FVector2D DragStart,PreviousPointer;
    TMap<int32,FVector2D> Touches;
    double ScaleFor(const FGeometry&) const;
    CityMap::Point Centre() const;
    CityMap::Point ToWorld(const FGeometry&,FVector2D) const;
    void Select(const FGeometry&,FVector2D);
    void Pan(const FGeometry&,FVector2D);
    void ZoomAt(const FGeometry&,FVector2D,double);
    void ResetGestures();
};
