#include "CityPlayerProfile.h"
#include "CityPasswordKdf.h"
#include "Misc/SecureHash.h"
#include "Misc/Guid.h"

namespace
{
struct FEngineSha1
{
    CityPassword::Hash operator()(const uint8_t* Data, size_t Size) const
    {
        return operator()(Data, Size, nullptr, 0);
    }
    CityPassword::Hash operator()(const uint8_t* A, size_t ASize, const uint8_t* B, size_t BSize) const
    {
        FSHA1 Digest;
        Digest.Update(A, static_cast<uint32>(ASize));
        if (BSize) Digest.Update(B, static_cast<uint32>(BSize));
        Digest.Final();
        CityPassword::Hash Result{};
        Digest.GetHash(Result.data());
        return Result;
    }
};
CityPassword::Hash HashPassword(const FString& Password, const FString& Salt)
{
    const FTCHARToUTF8 UtfPassword(*Password), UtfSalt(*Salt);
    return CityPassword::Derive(std::string_view(UtfPassword.Get(), UtfPassword.Length()),
        std::string_view(UtfSalt.Get(), UtfSalt.Length()), CityProfileRules::PasswordIterations, FEngineSha1{});
}
}
FString UCityPlayerProfile::Slot(const FString& First, const FString& Last)
{
    return TEXT("CityV2_") + First.ToLower() + TEXT("_") + Last.ToLower();
}
void UCityPlayerProfile::SetPassword(const FString& Password)
{
    PasswordSalt=FGuid::NewGuid().ToString(EGuidFormats::Digits);
    const auto Hash=HashPassword(Password,PasswordSalt);
    PasswordHash.Reset();
    PasswordHash.Append(Hash.data(),static_cast<int32>(Hash.size()));
}
bool UCityPlayerProfile::CheckPassword(const FString& Password) const
{
    if (PasswordHash.Num()!=20 || PasswordSalt.Len()!=32) return false;
    CityPassword::Hash Expected{};
    std::copy(PasswordHash.GetData(),PasswordHash.GetData()+20,Expected.begin());
    return CityPassword::Equal(Expected,HashPassword(Password,PasswordSalt));
}
