#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
namespace city {
// Server-authored strings are UTF-8; the stock client expects Windows-1251.
// Client-authored chat is already CP1251 and must be sent without this conversion.
inline std::string clientText(std::string_view text) {
    constexpr std::array<uint32_t, 64> extended{
        {0x402, 0x403,  0x201a, 0x453, 0x201e, 0x2026, 0x2020, 0x2021, 0x20ac, 0x2030, 0x409,  0x2039, 0x40a,
         0x40c, 0x40b,  0x40f,  0x452, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0,      0x2122,
         0x459, 0x203a, 0x45a,  0x45c, 0x45b,  0x45f,  0xa0,   0x40e,  0x45e,  0x408,  0xa4,   0x490,  0xa6,
         0xa7,  0x401,  0xa9,   0x404, 0xab,   0xac,   0xad,   0xae,   0x407,  0xb0,   0xb1,   0x406,  0x456,
         0x491, 0xb5,   0xb6,   0xb7,  0x451,  0x2116, 0x454,  0xbb,   0x458,  0x405,  0x455,  0x457}};
    std::string out;
    for (size_t i = 0; i < text.size();) {
        const auto first = static_cast<unsigned char>(text[i++]);
        if (first < 0x80) {
            out += static_cast<char>(first);
            continue;
        }
        uint32_t code = 0, minimum = 0;
        unsigned trailing = 0;
        if (first >= 0xc2 && first <= 0xdf) {
            code = first & 31;
            trailing = 1;
            minimum = 0x80;
        } else if (first >= 0xe0 && first <= 0xef) {
            code = first & 15;
            trailing = 2;
            minimum = 0x800;
        } else if (first >= 0xf0 && first <= 0xf4) {
            code = first & 7;
            trailing = 3;
            minimum = 0x10000;
        } else {
            out += '?';
            continue;
        }
        bool valid = i + trailing <= text.size();
        for (unsigned n = 0; valid && n < trailing; ++n) {
            const auto next = static_cast<unsigned char>(text[i + n]);
            if ((next & 0xc0) != 0x80)
                valid = false;
            else
                code = (code << 6) | (next & 63);
        }
        if (!valid) {
            out += '?';
            continue;
        }
        i += trailing;
        if (code < minimum || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff)) {
            out += '?';
            continue;
        }
        if (code >= 0x410 && code <= 0x44f)
            out += static_cast<char>(code - 0x410 + 0xc0);
        else {
            char encoded = '?';
            for (size_t n = 0; n < extended.size(); ++n)
                if (extended[n] == code) {
                    encoded = static_cast<char>(0x80 + n);
                    break;
                }
            out += encoded;
        }
    }
    return out;
}
} // namespace city
