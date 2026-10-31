#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string_view>

// PBKDF2-HMAC-SHA1 (RFC 8018). SHA1 collision attacks do not break HMAC.
// Digest is supplied by Unreal's FSHA1; password material is never serialized.
namespace CityPassword
{
using Hash = std::array<uint8_t, 20>;
template<class Digest>
Hash Derive(std::string_view Password, std::string_view Salt, unsigned Iterations, Digest Sha1)
{
    std::array<uint8_t,64> Key{};
    if (Password.size() > Key.size())
    {
        const auto Short = Sha1(reinterpret_cast<const uint8_t*>(Password.data()), Password.size());
        std::copy(Short.begin(), Short.end(), Key.begin());
    }
    else std::memcpy(Key.data(), Password.data(), Password.size());
    std::array<uint8_t,84> Inner{};
    std::array<uint8_t,84> Outer{};
    for (unsigned I=0; I<64; ++I) { Inner[I]=Key[I]^0x36; Outer[I]=Key[I]^0x5c; }
    auto Hmac = [&](const uint8_t* Data, size_t Size) {
        // Engine digest wrapper accepts several segments, avoiding per-round allocation.
        const Hash Inside = Sha1(Inner.data(), size_t(64), Data, Size);
        std::copy(Inside.begin(), Inside.end(), Outer.begin()+64);
        return Sha1(Outer.data(), Outer.size());
    };
    // Salt is a random GUID encoded as 32 ASCII characters, plus block index 1.
    // Fixed buffer also permits RFC test vectors with shorter salts.
    if (Salt.size() > 64 || Iterations == 0) return Hash{};
    std::array<uint8_t,68> Block{};
    std::memcpy(Block.data(), Salt.data(), Salt.size());
    Block[Salt.size()+3]=1;
    Hash U=Hmac(Block.data(), Salt.size()+4), Result=U;
    for (unsigned Round=1; Round<Iterations; ++Round)
    {
        U=Hmac(U.data(), U.size());
        for (unsigned I=0; I<Result.size(); ++I) Result[I]^=U[I];
    }
    return Result;
}
inline bool Equal(const Hash& A, const Hash& B)
{
    uint8_t Difference=0;
    for (unsigned I=0; I<A.size(); ++I) Difference |= A[I]^B[I];
    return Difference==0;
}
}
