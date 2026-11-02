#include "ClientText.hpp"
#include "Database.hpp"
#include <SAMPCpp/Everything.hpp>
#include <unordered_map>
namespace samp = samp_cpp;
class CityGameMode;
namespace {
CityGameMode *mode = nullptr;
}
namespace {
constexpr int LoginDialog = 30000, RegisterDialog = 30001, SpawnDialog = 30002, HouseFloorDialog = 30003,
              HouseDoorDialog = 30004, HouseActionDialog = 30005, QuizDialog = 30006;
constexpr unsigned White = 0xeeeeeeff, Red = 0xff654dff, Orange = 0xff8b35ff;
using Clock = std::chrono::steady_clock;
struct Session {
    uint64_t generation = 0;
    std::string name;
    city::Profile profile;
    bool authenticated = false, pending = false, spawning = false, delivery = false;
    int expectedDialog = -1, attempts = 0;
    float deliveryX = 0, deliveryY = 0, deliveryZ = 0;
    float targetX = 0, targetY = 0, targetZ = 0;
    Clock::time_point deliveryStarted{}, connected = Clock::now(), lastMessage{}, lastCommand{}, lastSave{};
    samp::Vehicle *scooter = nullptr;
    samp::Vehicle *car = nullptr;
    city::Apartment selectedApartment;
    bool inside = false;
    city::Point outside{1958.33f, 1343.12f, 15.36f};
    int quizOrganization = 0, quizQuestion = 0, quizScore = 0, hud = -1;
};
void message(int id, const std::string &text, unsigned colour = White) {
    auto encoded = city::clientText(text);
    sampgdk_SendClientMessage(id, colour, encoded.c_str());
}
void dialog(int id, int index, int style, const char *title, const char *text, const char *positive = "Войти",
            const char *negative = "Выход") {
    auto a = city::clientText(title), b = city::clientText(text), c = city::clientText(positive),
         d = city::clientText(negative);
    sampgdk_ShowPlayerDialog(id, index, style, a.c_str(), b.c_str(), c.c_str(), d.c_str());
}
} // namespace
class CityGameMode final : public samp::IGameMode {
    city::Database database;
    std::unordered_map<int, Session> sessions;
    uint64_t sequence = 0;
    std::vector<int> actors, labels;
    static int skin(const Session &s) {
        return s.profile.courier             ? 29
               : s.profile.organization == 1 ? 280
               : s.profile.organization == 2 ? 274
                                             : 60;
    }
    bool near(int id, city::Point p, float distance = 5) {
        return sampgdk_GetPlayerInterior(id) == 0 && sampgdk_GetPlayerVirtualWorld(id) == 0 &&
               sampgdk_IsPlayerInRangeOfPoint(id, distance, p.x, p.y, p.z);
    }
    bool owns(const Session &s, city::Apartment a) {
        return std::any_of(s.profile.apartments.begin(), s.profile.apartments.end(),
                           [a](auto owned) { return city::sameApartment(a, owned); });
    }
    bool submitCustom(int id, Session &s, city::Work work) {
        work.player = id;
        work.generation = s.generation;
        work.nickname = s.name;
        work.profile = s.profile;
        if (!database.submit(std::move(work))) {
            message(id, "База занята. Повторите позже.", Red);
            return false;
        }
        s.pending = true;
        return true;
    }
    bool submit(int id, Session &s, city::Operation op, std::string secret = {}, bool employed = false) {
        city::Work work{op, id, s.generation, s.name, std::move(secret), s.profile, employed};
        if (!database.submit(std::move(work))) {
            message(id, "База занята. Повторите через несколько секунд.", Red);
            return false;
        }
        if (op != city::Operation::Position)
            s.pending = true;
        return true;
    }
    void money(int id, Session &s) {
        sampgdk_ResetPlayerMoney(id);
        sampgdk_GivePlayerMoney(id, s.profile.balance);
        if (s.hud < 0) {
            s.hud = sampgdk_CreatePlayerTextDraw(id, 495, 24, " ");
            if (s.hud == 0xffff) {
                s.hud = -1;
                return;
            }
            sampgdk_PlayerTextDrawLetterSize(id, s.hud, .26f, 1.2f);
            sampgdk_PlayerTextDrawFont(id, s.hud, 1);
            sampgdk_PlayerTextDrawColor(id, s.hud, 0xffd8c4ff);
            sampgdk_PlayerTextDrawUseBox(id, s.hud, true);
            sampgdk_PlayerTextDrawBoxColor(id, s.hud, 0x09090b99);
            sampgdk_PlayerTextDrawTextSize(id, s.hud, 625, 0);
        }
        auto value = "CITYLIVE ~r~|~w~ " + std::to_string(s.profile.balance);
        sampgdk_PlayerTextDrawSetString(id, s.hud, value.c_str());
        sampgdk_PlayerTextDrawShow(id, s.hud);
    }
    void password(int id, bool registration) {
        sessions.at(id).expectedDialog = registration ? RegisterDialog : LoginDialog;
        dialog(id, registration ? RegisterDialog : LoginDialog, 3, registration ? "Регистрация" : "Вход",
               registration ? "Придумайте пароль: от 8 до 64 символов.\nОн будет храниться в защищённом виде."
                            : "Введите пароль своего персонажа.");
    }
    void spawnChoice(int id, Session &s) {
        s.expectedDialog = SpawnDialog;
        dialog(id, SpawnDialog, 2, "Место появления",
               "Организация\nНачальное место\nМесто выхода\nПарковка\nДом", "Войти");
    }
    void floors(int id, Session &s) {
        std::string text;
        for (int i = 1; i <= city::Buildings[s.selectedApartment.building].floors; ++i)
            text += "Этаж " + std::to_string(i) + "\n";
        s.expectedDialog = HouseFloorDialog;
        dialog(id, HouseFloorDialog, 2, "Подъезд", text.c_str(), "Выбрать", "Закрыть");
    }
    void doors(int id, Session &s) {
        s.expectedDialog = HouseDoorDialog;
        dialog(id, HouseDoorDialog, 2, "Квартиры этажа", "Квартира 1\nКвартира 2\nКвартира 3\nКвартира 4",
               "Выбрать", "Закрыть");
    }
    void quiz(int id, Session &s) {
        const auto &q =
            s.quizOrganization == 1 ? city::PoliceQuiz[s.quizQuestion] : city::HospitalQuiz[s.quizQuestion];
        std::string text =
            std::string(q.question) + "\n" +
            (s.quizQuestion % 2 ? std::string(q.bad) + "\n" + q.good : std::string(q.good) + "\n" + q.bad);
        s.expectedDialog = QuizDialog;
        dialog(id, QuizDialog, 5, "Собеседование", text.c_str(), "Ответить", "Отмена");
    }
    void enterApartment(int id, Session &s) {
        auto a = s.selectedApartment;
        if (!owns(s, a) || !near(id, city::Buildings[a.building].entrance)) {
            message(id, "Вход доступен владельцу у своего подъезда.");
            return;
        }
        save(id, s);
        s.outside = city::Buildings[a.building].entrance;
        s.inside = true;
        sampgdk_SetPlayerVirtualWorld(id, 50000 + a.building * 1000 + a.floor * 10 + a.door);
        sampgdk_SetPlayerInterior(id, 1);
        sampgdk_SetPlayerPos(id, 223.04f, 1289.26f, 1082.2f);
        message(id, "Ваша квартира. /exit — выйти.");
    }
    void landmark(city::Point p, int model, const char *text) {
        if (model >= 0)
            actors.push_back(sampgdk_CreateActor(model, p.x, p.y, p.z, 90));
        auto encoded = city::clientText(text);
        labels.push_back(sampgdk_Create3DTextLabel(encoded.c_str(), Orange, p.x, p.y, p.z + 1, 25, 0, true));
    }
    void save(int id, Session &s) {
        if (!s.authenticated || s.spawning || sampgdk_GetPlayerInterior(id) != 0 ||
            sampgdk_GetPlayerVirtualWorld(id) != 0 || sampgdk_GetPlayerState(id) == 7)
            return;
        float x, y, z, angle;
        if (!sampgdk_GetPlayerPos(id, &x, &y, &z) || !sampgdk_GetPlayerFacingAngle(id, &angle) ||
            !city::validPosition(x, y, z, angle))
            return;
        s.profile.x = x;
        s.profile.y = y;
        s.profile.z = z;
        s.profile.yaw = angle;
        s.profile.hasPosition = true;
        submit(id, s, city::Operation::Position);
        s.lastSave = Clock::now();
    }
    void removeScooter(Session &s) {
        if (s.scooter) {
            map.remove(*s.scooter);
            s.scooter = nullptr;
        }
    }
    void lookup(samp::Player &p) {
        int id = p.getIndex();
        auto &s = sessions[id];
        s.generation = ++sequence;
        s.name = p.getName();
        sampgdk_TogglePlayerSpectating(id, true);
        if (!city::validName(s.name)) {
            message(id, "Используйте имя вида First_Last: имя и фамилия латиницей.", Red);
            sampgdk_Kick(id);
            return;
        }
        submit(id, s, city::Operation::Lookup);
    }
    void disconnect(samp::Player &p, samp::Player::DisconnectReason) {
        auto it = sessions.find(p.getIndex());
        if (it == sessions.end())
            return;
        save(p.getIndex(), it->second);
        removeScooter(it->second);
        if (it->second.car)
            map.remove(*it->second.car);
        if (it->second.hud >= 0)
            sampgdk_PlayerTextDrawDestroy(p.getIndex(), it->second.hud);
        sessions.erase(it);
    }
    void spawn(samp::Player &p) {
        auto it = sessions.find(p.getIndex());
        if (it == sessions.end() || !it->second.authenticated) {
            sampgdk_TogglePlayerSpectating(p.getIndex(), true);
            return;
        }
        auto &s = it->second;
        int id = p.getIndex();
        if (!s.spawning) {
            s.targetX = 1958.33f;
            s.targetY = 1343.12f;
            s.targetZ = 15.36f;
        }
        s.inside = false;
        p.setLocation({s.targetX, s.targetY, s.targetZ});
        p.setWorld(0);
        p.setInterior(0);
        p.setSkin(skin(s));
        p.setHealth(100);
        sampgdk_TogglePlayerControllable(id, true);
        money(id, s);
        s.spawning = false;
    }
    void classChoice(samp::Player &p, samp::Int32) {
        auto it = sessions.find(p.getIndex());
        if (it != sessions.end() && !it->second.authenticated)
            sampgdk_TogglePlayerSpectating(p.getIndex(), true);
    }
    void local(int id, const std::string &line, float radius, unsigned colour = White) {
        float x, y, z;
        if (!sampgdk_GetPlayerPos(id, &x, &y, &z))
            return;
        for (const auto &pair : sessions) {
            int other = pair.first;
            if (!pair.second.authenticated || pair.second.spawning)
                continue;
            if (sampgdk_GetPlayerVirtualWorld(id) != sampgdk_GetPlayerVirtualWorld(other) ||
                sampgdk_GetPlayerInterior(id) != sampgdk_GetPlayerInterior(other))
                continue;
            if (sampgdk_IsPlayerInRangeOfPoint(other, radius, x, y, z))
                // Stock-client chat is already CP1251; convert only server-authored text.
                sampgdk_SendClientMessage(other, colour, line.c_str());
        }
    }
    void text(samp::Player &p, std::string_view raw) {
        auto it = sessions.find(p.getIndex());
        if (it == sessions.end() || !it->second.authenticated || it->second.spawning)
            return;
        auto &s = it->second;
        auto now = Clock::now();
        if (now - s.lastMessage < std::chrono::milliseconds(900))
            return;
        s.lastMessage = now;
        auto clean = city::cleanChat(raw);
        if (!clean.empty())
            local(p.getIndex(), s.name + ": " + clean, 25);
    }
    void command(samp::Player &p, std::string_view raw) {
        int id = p.getIndex();
        auto it = sessions.find(id);
        if (it == sessions.end() || !it->second.authenticated || it->second.spawning)
            return;
        auto &s = it->second;
        auto now = Clock::now();
        if (now - s.lastCommand < std::chrono::milliseconds(400))
            return;
        s.lastCommand = now;
        std::string cmd(raw);
        auto split = cmd.find(' ');
        auto name = cmd.substr(0, split);
        auto tail = split == cmd.npos ? std::string{} : city::cleanChat(cmd.substr(split + 1));
        if (name == "/help") {
            message(id, "/stats /me /b /s /job /quitjob /scooter /delivery /deliver", Orange);
            message(id, "/gps /house /exit /police /hospital /car /save", Orange);
            return;
        }
        if (name == "/stats") {
            message(id, "Баланс: " + std::to_string(s.profile.balance) + " | Доставок: " +
                            std::to_string(s.profile.deliveries) + (s.profile.courier ? " | Курьер" : ""));
            message(id, "Организация: " +
                            std::string(s.profile.organization == 1   ? "Полиция"
                                        : s.profile.organization == 2 ? "Больница"
                                                                      : "Нет") +
                            " | Квартир: " + std::to_string(s.profile.apartments.size()));
            return;
        }
        if (name == "/me" || name == "/b" || name == "/s") {
            if (tail.empty())
                return;
            if (now - s.lastMessage < std::chrono::milliseconds(900))
                return;
            s.lastMessage = now;
            local(id,
                  name == "/me"  ? "* " + s.name + " " + tail
                  : name == "/b" ? "(( " + s.name + ": " + tail + " ))"
                                 : s.name + city::clientText(" кричит: ") + tail,
                  name == "/s" ? 60 : 25, name == "/me" ? 0xc58cecff : White);
            return;
        }
        if (name == "/save") {
            save(id, s);
            message(id, "Позиция поставлена в очередь сохранения.");
            return;
        }
        if (s.pending) {
            message(id, "Дождитесь ответа базы данных.", Orange);
            return;
        }
        if (name == "/exit") {
            if (!s.inside) {
                message(id, "Вы не в квартире.");
                return;
            }
            sampgdk_SetPlayerVirtualWorld(id, 0);
            sampgdk_SetPlayerInterior(id, 0);
            sampgdk_SetPlayerPos(id, s.outside.x, s.outside.y, s.outside.z);
            s.inside = false;
            return;
        }
        if (name == "/house") {
            for (size_t i = 0; i < city::Buildings.size(); ++i)
                if (near(id, city::Buildings[i].entrance, 4)) {
                    s.selectedApartment = {static_cast<int>(i), 1, 1};
                    floors(id, s);
                    return;
                }
            message(id, "Подойдите к подъезду. /gps house — ближайший дом.");
            return;
        }
        if (name == "/police" || name == "/hospital") {
            int organization = name == "/police" ? 1 : 2;
            if (!near(id, organization == 1 ? city::Police : city::Hospital)) {
                message(id, "Подойдите к сотруднику организации. Адрес: /gps police или /gps hospital.");
                return;
            }
            s.quizOrganization = organization;
            s.quizQuestion = 0;
            s.quizScore = 0;
            quiz(id, s);
            return;
        }
        if (name == "/car") {
            if (!near(id, city::Parking, 12)) {
                message(id, "Автомобиль выдаётся на парковке: /gps parking.");
                return;
            }
            if (s.car) {
                message(id, "Ваш автомобиль уже выдан. /gps car — показать его.");
                return;
            }
            auto vehicle = map.beginConstruction<samp::Vehicle>();
            vehicle->setModel(551);
            vehicle->setLocation({city::Parking.x + 6, city::Parking.y, city::Parking.z});
            vehicle->setFacingAngle(90);
            vehicle->setFirstColor(1);
            vehicle->setSecondColor(1);
            s.car = &map.finalizeConstruction(vehicle);
            message(id, "Автомобиль выдан рядом на время сеанса.");
            return;
        }
        if (name == "/job" || name == "/quitjob") {
            if (name == "/job" && !near(id, city::Courier, 8)) {
                message(id, "Подойдите к работодателю: /gps courier.");
                return;
            }
            submit(id, s, city::Operation::Employment, {}, name == "/job");
            return;
        }
        if (name == "/scooter") {
            if (!s.profile.courier) {
                message(id, "Сначала устройтесь курьером: /job.");
                return;
            }
            if (!near(id, city::Courier, 12)) {
                message(id, "Получить скутер можно у работодателя: /gps courier.");
                return;
            }
            if (s.scooter) {
                message(id, "Рабочий скутер уже выдан. Используйте /gps scooter.");
                return;
            }
            float x, y, z;
            if (!sampgdk_GetPlayerPos(id, &x, &y, &z))
                return;
            auto vehicle = map.beginConstruction<samp::Vehicle>();
            vehicle->setModel(448);
            vehicle->setLocation({x + 4, y, z + .3f});
            vehicle->setFacingAngle(90);
            vehicle->setFirstColor(3);
            vehicle->setSecondColor(1);
            s.scooter = &map.finalizeConstruction(vehicle);
            message(id, "Скутер выдан рядом. Сядьте за руль и возьмите /delivery.");
            return;
        }
        if (name == "/delivery") {
            if (!s.profile.courier || !s.scooter) {
                message(id, "Нужны работа курьера и рабочий скутер: /job, затем /scooter.");
                return;
            }
            if (s.delivery) {
                message(id, "У вас уже есть заказ. /gps delivery — показать адрес.");
                return;
            }
            static constexpr float destinations[][3] = {
                {2014.5f, 1535.0f, 10.82f}, {1901.0f, 1447.0f, 10.82f}, {2121.0f, 1399.0f, 10.82f}};
            auto index = s.profile.deliveries % 3;
            s.deliveryStarted = Clock::now();
            s.delivery = true;
            s.deliveryX = destinations[index][0];
            s.deliveryY = destinations[index][1];
            s.deliveryZ = destinations[index][2];
            sampgdk_SetPlayerCheckpoint(id, s.deliveryX, s.deliveryY, s.deliveryZ, 4);
            message(id, "Заказ отмечен. Остановитесь у метки на скутере и введите /deliver.");
            return;
        }
        if (name == "/deliver") {
            if (Clock::now() - s.deliveryStarted < std::chrono::seconds(15)) {
                message(id, "Доставка ещё не готова. Доезжайте до адреса.");
                return;
            }
            float x = 0, y = 0, z = 0, vx = 0, vy = 0, vz = 0;
            if (!sampgdk_GetPlayerPos(id, &x, &y, &z))
                return;
            bool rider = s.scooter && sampgdk_GetPlayerState(id) == 2 &&
                         sampgdk_GetPlayerVehicleID(id) == s.scooter->getHandle();
            if (rider)
                sampgdk_GetVehicleVelocity(s.scooter->getHandle(), &vx, &vy, &vz);
            float distance =
                std::sqrt((x - s.deliveryX) * (x - s.deliveryX) + (y - s.deliveryY) * (y - s.deliveryY) +
                          (z - s.deliveryZ) * (z - s.deliveryZ));
            float speed = std::sqrt(vx * vx + vy * vy + vz * vz) * 180;
            if (!city::canDeliver(s.profile.courier, s.delivery, rider, distance, speed, s.profile.balance)) {
                message(id, "Остановитесь на своём рабочем скутере у адреса доставки.");
                return;
            }
            submit(id, s, city::Operation::Reward);
            return;
        }
        if (name == "/gps") {
            if (tail == "off") {
                sampgdk_DisablePlayerCheckpoint(id);
                message(id, "Метка скрыта. Заказ сохранён.");
            } else if (tail == "scooter" && s.scooter) {
                auto pos = s.scooter->getLocation();
                sampgdk_SetPlayerCheckpoint(id, pos.x, pos.y, pos.z, 3);
            } else if (tail == "car" && s.car) {
                auto pos = s.car->getLocation();
                sampgdk_SetPlayerCheckpoint(id, pos.x, pos.y, pos.z, 3);
            } else if (tail == "police" || tail == "hospital" || tail == "parking" || tail == "courier" ||
                       tail == "house" || tail == "home") {
                city::Point point = tail == "police"     ? city::Police
                                    : tail == "hospital" ? city::Hospital
                                    : tail == "parking"  ? city::Parking
                                                         : city::Courier;
                if (tail == "home") {
                    if (s.profile.apartments.empty()) {
                        message(id, "У вас нет квартиры.");
                        return;
                    }
                    point = city::Buildings[s.profile.apartments.front().building].entrance;
                }
                if (tail == "house") {
                    float x, y, z;
                    if (!sampgdk_GetPlayerPos(id, &x, &y, &z))
                        return;
                    float best = 1e20f;
                    for (const auto &b : city::Buildings) {
                        float distance =
                            (b.entrance.x - x) * (b.entrance.x - x) + (b.entrance.y - y) * (b.entrance.y - y);
                        if (distance < best) {
                            best = distance;
                            point = b.entrance;
                        }
                    }
                }
                sampgdk_SetPlayerCheckpoint(id, point.x, point.y, point.z, 4);
                message(id, "Место отмечено на карте.");
            } else if (s.delivery && (tail.empty() || tail == "delivery"))
                sampgdk_SetPlayerCheckpoint(id, s.deliveryX, s.deliveryY, s.deliveryZ, 4);
            else
                message(id, "/gps courier | police | hospital | parking | house | home | scooter | car | "
                            "delivery | off");
            return;
        }
        message(id, "Неизвестная команда. /help — список команд.");
    }

  protected:
    void setupEvents() override {
        server.onPlayerConnect += {*this, &CityGameMode::lookup};
        server.onPlayerDisconnect += {*this, &CityGameMode::disconnect};
        server.onPlayerSpawn += {*this, &CityGameMode::spawn};
        server.onPlayerRequestClass += {*this, &CityGameMode::classChoice};
        server.onPlayerText += {*this, &CityGameMode::text};
        server.onPlayerCommandText += {*this, &CityGameMode::command};
    }
    void onServerUpdate(double delta, samp::IUpdatable::TimePoint time) override {
        IGameMode::onServerUpdate(delta, time);
        for (auto &r : database.drain()) {
            auto it = sessions.find(r.player);
            if (it == sessions.end() || it->second.generation != r.generation)
                continue;
            auto &s = it->second;
            if (r.op == city::Operation::Position)
                continue;
            s.pending = false;
            if (r.status == city::Status::Error) {
                message(r.player, "База данных недоступна. Обратитесь к администрации.", Red);
                continue;
            }
            if (r.op == city::Operation::Lookup) {
                password(r.player, r.status == city::Status::Missing);
                continue;
            }
            if (r.op == city::Operation::Login || r.op == city::Operation::Register) {
                if (r.status == city::Status::WrongPassword) {
                    if (++s.attempts >= 5) {
                        message(r.player, "Слишком много попыток входа.", Red);
                        sampgdk_Kick(r.player);
                    } else {
                        message(r.player, "Неверный пароль.", Red);
                        password(r.player, false);
                    }
                    continue;
                }
                if (r.status == city::Status::Exists) {
                    message(r.player, "Персонаж уже зарегистрирован. Введите пароль.");
                    password(r.player, false);
                    continue;
                }
                if (r.status != city::Status::Ok) {
                    password(r.player, true);
                    continue;
                }
                bool online = false;
                for (const auto &pair : sessions)
                    if (pair.first != r.player && pair.second.authenticated &&
                        pair.second.profile.id == r.profile.id)
                        online = true;
                if (online) {
                    message(r.player, "Этот персонаж уже в игре.", Red);
                    sampgdk_Kick(r.player);
                    continue;
                }
                s.profile = r.profile;
                s.authenticated = true;
                s.spawning = true;
                s.lastSave = Clock::now();
                spawnChoice(r.player, s);
                continue;
            }
            if (r.op == city::Operation::BuyApartment && r.status == city::Status::Funds) {
                message(r.player, "Недостаточно денег для покупки.", Red);
                continue;
            }
            if (r.op == city::Operation::BuyApartment && r.status == city::Status::Occupied) {
                message(r.player, "Эта квартира уже куплена.", Red);
                continue;
            }
            if (r.status != city::Status::Ok)
                continue;
            s.profile = r.profile;
            money(r.player, s);
            if (r.op == city::Operation::Employment) {
                if (!s.profile.courier) {
                    s.delivery = false;
                    sampgdk_DisablePlayerCheckpoint(r.player);
                    removeScooter(s);
                }
                sampgdk_SetPlayerSkin(r.player, skin(s));
                message(r.player, s.profile.courier ? "Вы устроились курьером. /scooter — рабочий транспорт."
                                                    : "Вы уволились.");
            }
            if (r.op == city::Operation::Reward) {
                s.delivery = false;
                sampgdk_DisablePlayerCheckpoint(r.player);
                message(r.player, "Доставка завершена. Получено 5 000. /delivery — следующий заказ.", Orange);
            }
            if (r.op == city::Operation::BuyApartment)
                message(r.player,
                        "Квартира куплена и сохранена. /house — вход у подъезда; /gps home — адрес.", Orange);
            if (r.op == city::Operation::Organization) {
                sampgdk_SetPlayerSkin(r.player, skin(s));
                message(r.player, "Собеседование пройдено. Организация и результат сохранены.", Orange);
            }
        }
        auto now = Clock::now();
        for (auto &pair : sessions) {
            auto &s = pair.second;
            if (!s.authenticated) {
                if (now - s.connected > std::chrono::seconds(120))
                    sampgdk_Kick(pair.first);
            } else if (now - s.lastSave > std::chrono::seconds(15)) {
                s.lastSave = now;
                save(pair.first, s);
            }
        }
    }

  public:
    explicit CityGameMode(samp::ServerClass &server) : IGameMode(server) {
        addPlayerClass(60, {1958.33f, 1343.12f, 15.36f}, 90);
        sampgdk_SetGameModeText("CityLive RP C++");
        landmark(city::Courier, 29, "Курьер /job /scooter");
        landmark(city::Police, 280, "Полиция /police");
        landmark(city::Hospital, 274, "Больница /hospital");
        landmark(city::Parking, -1, "Парковка /car");
        for (const auto &building : city::Buildings)
            landmark(building.entrance, -1, (std::string(building.name) + "\n/house").c_str());
    }
    ~CityGameMode() override {
        std::clog << "[CityLive] Saving and stopping C++ gamemode.\n";
        mode = nullptr;
        for (auto &pair : sessions)
            save(pair.first, pair.second);
        for (int actor : actors)
            sampgdk_DestroyActor(actor);
        for (int label : labels)
            sampgdk_Delete3DTextLabel(label);
    }
    bool dialogResponse(int id, int index, int response, int item, const char *input) {
        if (index < LoginDialog || index > QuizDialog)
            return false;
        auto it = sessions.find(id);
        if (it == sessions.end())
            return true;
        auto &s = it->second;
        if (index != s.expectedDialog)
            return true;
        if (!response) {
            s.expectedDialog = -1;
            if (index <= SpawnDialog)
                sampgdk_Kick(id);
            return true;
        }
        if (s.pending)
            return true;
        s.expectedDialog = -1;
        if (index == SpawnDialog) {
            if (!s.authenticated)
                return true;
            if (item < 0 || item > 4 || (item == 0 && !s.profile.organization) ||
                (item == 2 && !s.profile.hasPosition) || (item == 4 && s.profile.apartments.empty())) {
                message(id, "Это место пока недоступно. Выберите другое.");
                spawnChoice(id, s);
                return true;
            }
            city::Point point{1958.33f, 1343.12f, 15.36f};
            if (item == 0)
                point = s.profile.organization == 1 ? city::Police : city::Hospital;
            if (item == 2)
                point = {s.profile.x, s.profile.y, s.profile.z};
            if (item == 3)
                point = city::Parking;
            if (item == 4)
                point = city::Buildings[s.profile.apartments.front().building].entrance;
            s.targetX = point.x;
            s.targetY = point.y;
            s.targetZ = point.z;
            s.spawning = true;
            sampgdk_SetSpawnInfo(id, 0, skin(s), s.targetX, s.targetY, s.targetZ,
                                 item == 2 ? s.profile.yaw : 90, 0, 0, 0, 0, 0, 0);
            sampgdk_TogglePlayerSpectating(id, false);
            sampgdk_SpawnPlayer(id);
            message(id, "Добро пожаловать! /help — команды сервера.", Orange);
            return true;
        }
        if (index >= HouseFloorDialog) {
            if (!s.authenticated || s.spawning)
                return true;
            if (index == QuizDialog) {
                if (s.quizOrganization < 1 || s.quizOrganization > 2 || s.quizQuestion < 0 ||
                    s.quizQuestion >= 3 ||
                    !near(id, s.quizOrganization == 1 ? city::Police : city::Hospital) || item < 0 ||
                    item > 1) {
                    message(id, "Собеседование отменено. Подойдите к сотруднику.");
                    return true;
                }
                if (item == s.quizQuestion % 2)
                    ++s.quizScore;
                if (++s.quizQuestion < 3) {
                    quiz(id, s);
                    return true;
                }
                if (!city::passedQuiz(s.quizOrganization, s.quizScore)) {
                    message(id, "Нужно минимум 2 правильных ответа из 3. Можно пройти заново.");
                    return true;
                }
                city::Work work{};
                work.op = city::Operation::Organization;
                work.organization = s.quizOrganization;
                work.score = s.quizScore;
                submitCustom(id, s, std::move(work));
                return true;
            }
            if (!city::validApartment(s.selectedApartment) ||
                !near(id, city::Buildings[s.selectedApartment.building].entrance, 4)) {
                message(id, "Вы отошли от подъезда.");
                return true;
            }
            if (index == HouseFloorDialog) {
                if (item < 0 || item >= city::Buildings[s.selectedApartment.building].floors)
                    return true;
                s.selectedApartment.floor = item + 1;
                doors(id, s);
                return true;
            }
            if (index == HouseDoorDialog) {
                if (item < 0 || item > 3)
                    return true;
                s.selectedApartment.door = item + 1;
                s.expectedDialog = HouseActionDialog;
                std::string body =
                    owns(s, s.selectedApartment)
                        ? "Войти в свою квартиру"
                        : "Купить за " + std::to_string(city::Buildings[s.selectedApartment.building].price);
                dialog(id, HouseActionDialog, 2, "Квартира", body.c_str(), "Выбрать", "Закрыть");
                return true;
            }
            if (item != 0)
                return true;
            if (owns(s, s.selectedApartment)) {
                enterApartment(id, s);
                return true;
            }
            city::Work work{};
            work.op = city::Operation::BuyApartment;
            work.apartment = s.selectedApartment;
            submitCustom(id, s, std::move(work));
            return true;
        }
        if (s.authenticated)
            return true;
        size_t n = 0;
        while (n < 65 && input[n])
            ++n;
        if (n < 8 || n > 64) {
            message(id, "Пароль должен содержать от 8 до 64 символов.", Red);
            password(id, index == RegisterDialog);
            return true;
        }
        submit(id, s, index == RegisterDialog ? city::Operation::Register : city::Operation::Login,
               std::string(input, n));
        return true;
    }
};
samp::GameModeSetupResult SAMPGameModeSetup() {
    auto result = std::make_unique<CityGameMode>(*Server);
    mode = result.get();
    std::clog << "[CityLive] C++ gamemode initialized; waiting for MySQL.\n";
    return result;
}
PLUGIN_EXPORT bool PLUGIN_CALL CityFrameworkOnDialogResponse(int, int, int, int, const char *);
PLUGIN_EXPORT bool PLUGIN_CALL OnDialogResponse(int id, int index, int response, int item,
                                                const char *input) {
    if (!Server || !mode)
        return false;
    if (!input)
        input = "";
    if (mode && mode->dialogResponse(id, index, response, item, input))
        return true;
    return CityFrameworkOnDialogResponse(id, index, response, item, input);
}
