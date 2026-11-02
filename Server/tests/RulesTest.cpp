#include "Rules.hpp"
#include "ClientText.hpp"
#include "Config.hpp"
#include "World.hpp"
#include <cassert>
#include <iostream>
#include <limits>
int main() {
    assert(city::validName("Alex_Ivanov"));
    for (auto name :
         {"A_Ivanov", "Alex_I", "Alex_Ivanov_extra", "Alex_1vanov", "Alex Ivanov", "Alex_' OR 1=1"})
        assert(!city::validName(name));
    assert(city::cleanChat("{FF0000}hello\nworld") == "FF0000helloworld");
    assert(city::cleanChat(std::string(300, 'a')).size() == 128);
    assert(city::validPosition(1958, 1343, 15, 90));
    assert(!city::validPosition(std::numeric_limits<float>::quiet_NaN(), 0, 0, 0));
    assert(!city::validPosition(1e9, 0, 0, 0));
    assert(city::canDeliver(true, true, true, 3, 0, 20000));
    assert(!city::canDeliver(true, true, true, 4.1, 0, 20000));
    assert(!city::canDeliver(true, true, true, 3, 4, 20000));
    assert(!city::canDeliver(false, true, true, 3, 0, 20000));
    assert(!city::canDeliver(true, false, true, 3, 0, 20000));
    assert(!city::canDeliver(true, true, false, 3, 0, 20000));
    assert(!city::canDeliver(true, true, true, 3, 0, 2147483647));
    assert(city::clientText("Привет") == std::string("\xcf\xf0\xe8\xe2\xe5\xf2"));
    assert(city::clientText("Ёж") == std::string("\xa8\xe6"));
    assert(city::clientText("Hello") == "Hello");
    assert(city::clientText("— … · «»") == std::string("\x97 \x85 \xb7 \xab\xbb"));
    assert(city::clientText("😀") == "?");
    assert(city::clientText(std::string("\xf0\x80\x80\x80")) == "?");
    assert(city::StartingBalance == 20000 && city::DeliveryReward == 5000);
    assert(city::validApartment({0, 5, 4}));
    assert(!city::validApartment({0, 6, 1}));
    assert(city::validApartment({7, 6, 4}));
    assert(!city::validApartment({-1, 1, 1}));
    assert(!city::validApartment({8, 1, 1}));
    assert(!city::validApartment({0, 1, 5}));
    assert(city::passedQuiz(1, 2) && city::passedQuiz(2, 3));
    assert(!city::passedQuiz(0, 3) && !city::passedQuiz(1, 1) && !city::passedQuiz(1, 4));
    std::cout
        << "PASS: nickname validation, local chat bounds, CP1251, position bounds and courier eligibility\n";
}
