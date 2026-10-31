#include "CityProfileRules.h"
#include "CityPasswordKdf.h"
#include "CitySedanDynamics.h"
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
struct Digest
{
    CityPassword::Hash operator()(const uint8_t* Data,size_t Size) const
    { CityPassword::Hash H{};SHA1(Data,Size,H.data());return H; }
    CityPassword::Hash operator()(const uint8_t* A,size_t ASize,const uint8_t* B,size_t BSize) const
    {
        std::vector<uint8_t> Data(A,A+ASize);
        Data.insert(Data.end(),B,B+BSize);
        return operator()(Data.data(),Data.size());
    }
};
std::string Hex(const CityPassword::Hash& Value)
{
    const char* Digits="0123456789abcdef";std::string Result;
    for(auto C:Value) {Result+=Digits[C>>4];Result+=Digits[C&15];}
    return Result;
}
int main()
{
    assert(CityProfileRules::StartingBalance==20000);
    for(auto Name:{"Ivan","Ivanov","ab","AbCdEfGhIjKlMnOpQrSt"}) assert(CityProfileRules::ValidName(Name));
    for(auto Name:{"","a","Иван","Ivan Ivanov","Ivan_1","AbCdEfGhIjKlMnOpQrStu","../../file","<name>"}) assert(!CityProfileRules::ValidName(Name));
    // RFC 6070 vectors: independently published PBKDF2-HMAC-SHA1 values.
    assert(Hex(CityPassword::Derive("password","salt",1,Digest{}))=="0c60c80f961f0e71f3a9b524af6012062fe037a6");
    assert(Hex(CityPassword::Derive("password","salt",2,Digest{}))=="ea6c014dc72d6f8ccd1ed92ace1d41f0d8de8957");
    assert(Hex(CityPassword::Derive("password","salt",4096,Digest{}))=="4b007901b765489abead49d926f721d065a429c1");
    for(const std::string& Password:{std::string("секрет123"),std::string(128,'a')})
    {
        const std::string Salt="0123456789abcdef0123456789abcdef";
        const auto Actual=CityPassword::Derive(Password,Salt,CityProfileRules::PasswordIterations,Digest{});
        CityPassword::Hash Expected{};
        assert(PKCS5_PBKDF2_HMAC_SHA1(Password.data(),static_cast<int>(Password.size()),
            reinterpret_cast<const unsigned char*>(Salt.data()),static_cast<int>(Salt.size()),
            CityProfileRules::PasswordIterations,20,Expected.data())==1);
        assert(CityPassword::Equal(Actual,Expected));
        Expected[0]^=1;assert(!CityPassword::Equal(Actual,Expected));
    }
    assert(CityLiveDriving::DriftStrength(0)==0);
    assert(CityLiveDriving::DriftStrength(400)==0);
    assert(CityLiveDriving::DriftStrength(-1000)==1);
    assert(CityLiveDriving::LateralGrip(0)==6);
    assert(CityLiveDriving::LateralGrip(1)>1);
    assert(CityLiveDriving::LateralGrip(1)<CityLiveDriving::LateralGrip(0));
    for(int Fps:{15,30,60,120})
    {
        float Forward=2000,Reverse=-300;
        for(int I=0;I<Fps;++I)
        {
            const float Next=CityLiveDriving::HandbrakeSpeed(Forward,1,1.f/Fps);
            assert(Next<=Forward && Next>=0);Forward=Next;
            const float Back=CityLiveDriving::HandbrakeSpeed(Reverse,1,1.f/Fps);
            assert(Back>=Reverse && Back<=0);Reverse=Back;
        }
        assert(std::abs(Forward-1820)<.1f && std::abs(Reverse+120)<.1f);
    }
    assert(CityLiveDriving::HandbrakeSpeed(10,1,1)==0);
    assert(CityLiveDriving::HandbrakeSpeed(-10,1,1)==0);
    std::cout<<"PASS: name rules, initial balance, password KDF reference vectors, drift braking and grip\n";
}
