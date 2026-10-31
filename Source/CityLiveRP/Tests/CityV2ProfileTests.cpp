#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"
#include "CityPlayerProfile.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCityV2ProfilePersistence,
    "CityLive.V2.ProfilePersistence",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCityV2ProfilePersistence::RunTest(const FString& Parameters)
{
    const FString Slot=TEXT("CityV2_Automation_")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    UCityPlayerProfile* Profile=NewObject<UCityPlayerProfile>();
    TestEqual(TEXT("New balance"),Profile->Balance,20000);
    TestEqual(TEXT("Nickname slot is case insensitive"),UCityPlayerProfile::Slot(TEXT("Ivan"),TEXT("Ivanov")),
        UCityPlayerProfile::Slot(TEXT("IVAN"),TEXT("ivanov")));
    Profile->FirstName=TEXT("Ivan");Profile->Surname=TEXT("Ivanov");
    Profile->SetPassword(TEXT("secret123"));
    TestTrue(TEXT("Right password"),Profile->CheckPassword(TEXT("secret123")));
    TestFalse(TEXT("Wrong password"),Profile->CheckPassword(TEXT("secret124")));
    Profile->Balance=12345;Profile->bHasLastPosition=true;
    Profile->LastPosition=FVector(120,340,100);Profile->LastYaw=80;Profile->LastMap=TEXT("/Game/TestCity");
    TestTrue(TEXT("Save succeeds"),UGameplayStatics::SaveGameToSlot(Profile,Slot,0));
    UCityPlayerProfile* Loaded=Cast<UCityPlayerProfile>(UGameplayStatics::LoadGameFromSlot(Slot,0));
    if(TestNotNull(TEXT("Reload profile"),Loaded))
    {
        TestEqual(TEXT("Existing balance is not reset"),Loaded->Balance,12345);
        TestTrue(TEXT("Password survives restart"),Loaded->CheckPassword(TEXT("secret123")));
        TestTrue(TEXT("Saved position survives restart"),Loaded->LastPosition.Equals(Profile->LastPosition));
        TestTrue(TEXT("Position enabled"),Loaded->bHasLastPosition);
        TestEqual(TEXT("Map survives restart"),Loaded->LastMap,Profile->LastMap);
    }
    UGameplayStatics::DeleteGameInSlot(Slot,0);
    return true;
}
#endif
