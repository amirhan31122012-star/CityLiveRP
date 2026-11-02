#pragma once
#include <array>
namespace city {
struct Point {
    float x, y, z;
};
struct Building {
    const char *name;
    Point entrance;
    int floors, price;
};
inline constexpr std::array<Building, 8> Buildings{
    {{"Северный дом · подъезд 1", {2014.5f, 1535.0f, 10.82f}, 5, 350000},
     {"Северный дом · подъезд 2", {2003.5f, 1535.0f, 10.82f}, 5, 350000},
     {"Южный дом · подъезд 1", {1901.0f, 1447.0f, 10.82f}, 5, 150000},
     {"Южный дом · подъезд 2", {1912.0f, 1447.0f, 10.82f}, 5, 150000},
     {"Башня А · подъезд 1", {2121.0f, 1399.0f, 10.82f}, 6, 500000},
     {"Башня А · подъезд 2", {2132.0f, 1399.0f, 10.82f}, 6, 500000},
     {"Башня Б · подъезд 1", {1893.0f, 1596.0f, 10.82f}, 6, 500000},
     {"Башня Б · подъезд 2", {1904.0f, 1596.0f, 10.82f}, 6, 500000}}};
inline constexpr Point Police{2288.0f, 2432.0f, 10.82f}, Hospital{1607.0f, 1818.0f, 10.82f},
    Parking{1955.0f, 1340.0f, 15.36f}, Courier{1964.0f, 1343.0f, 15.36f};
struct Apartment {
    int building = 0, floor = 1, door = 1;
};
inline bool validApartment(Apartment a) {
    return a.building >= 0 && a.building < static_cast<int>(Buildings.size()) && a.floor >= 1 &&
           a.floor <= Buildings[a.building].floors && a.door >= 1 && a.door <= 4;
}
inline bool sameApartment(Apartment a, Apartment b) {
    return a.building == b.building && a.floor == b.floor && a.door == b.door;
}
struct Quiz {
    const char *question;
    const char *good;
    const char *bad;
};
inline constexpr Quiz PoliceQuiz[] = {
    {"Можно применять силу без законного основания?", "Нет, сначала законное основание", "Да, по желанию"},
    {"Первое действие при обращении гражданина?", "Представиться и выслушать", "Сразу задержать"},
    {"Доклад о происшествии должен быть…", "Точным, без выдуманных сведений", "Каким угодно"}};
inline constexpr Quiz HospitalQuiz[] = {
    {"У пострадавшего нет сознания: сначала…", "Убедиться в безопасности и вызвать помощь", "Дать воду"},
    {"Можно разглашать сведения пациента?", "Нет, нужна законная причина", "Да, всем знакомым"},
    {"Нет подготовки для сложной процедуры: что делать?", "Позвать квалифицированного сотрудника",
     "Экспериментировать"}};
inline bool passedQuiz(int organization, int score) {
    return (organization == 1 || organization == 2) && score >= 2 && score <= 3;
}
} // namespace city
