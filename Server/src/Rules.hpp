#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
namespace city {
constexpr int StartingBalance = 20000, DeliveryReward = 5000;
inline bool validName(std::string_view name) {
    auto separator = name.find('_');
    if (separator == name.npos || separator < 2 || separator > 20 || name.size() - separator - 1 < 2 ||
        name.size() - separator - 1 > 20)
        return false;
    for (size_t i = 0; i < name.size(); ++i)
        if (i != separator && !((name[i] >= 'a' && name[i] <= 'z') || (name[i] >= 'A' && name[i] <= 'Z')))
            return false;
    return true;
}
inline std::string cleanChat(std::string_view value) {
    std::string out;
    for (unsigned char c : value) {
        if (out.size() >= 128)
            break;
        if (c >= 32 && c != 127 && c != '{' && c != '}')
            out += static_cast<char>(c);
    }
    return out;
}
inline bool validPosition(double x, double y, double z, double angle) {
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z) && std::isfinite(angle) &&
           std::abs(x) < 20000 && std::abs(y) < 20000 && z > -100 && z < 20000;
}
inline bool canDeliver(bool working, bool active, bool rider, double distance, double speed, int balance) {
    return working && active && rider && std::isfinite(distance) && std::isfinite(speed) && distance >= 0 &&
           distance <= 4 && std::abs(speed) < 2.9 && balance >= 0 && balance <= 2147483647 - DeliveryReward;
}
} // namespace city
