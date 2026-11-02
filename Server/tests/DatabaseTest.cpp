#include "Database.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
city::Result request(city::Database &db, city::Work work) {
    assert(db.submit(std::move(work)));
    for (int i = 0; i < 1500; ++i) {
        auto r = db.drain();
        if (!r.empty()) {
            assert(r.size() == 1);
            return r[0];
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    throw std::runtime_error("DB test timed out");
}
int main() {
    using namespace city;
    Database db;
    Work w{Operation::Lookup, 4, 99, "Test_Alpha", {}, {}, false};
    assert(request(db, w).status == Status::Missing);
    w.op = Operation::Register;
    w.secret = "correct-test-password";
    auto registered = request(db, w);
    assert(registered.status == Status::Ok);
    assert(registered.profile.balance == StartingBalance && registered.profile.id > 0);
    assert(registered.player == 4 && registered.generation == 99);
    assert(request(db, w).status == Status::Exists);
    w.op = Operation::Login;
    w.secret = "wrong-password";
    assert(request(db, w).status == Status::WrongPassword);
    w.secret = "correct-test-password";
    auto login = request(db, w);
    assert(login.status == Status::Ok && login.profile.id == registered.profile.id);
    w.profile = login.profile;
    w.secret.clear();
    w.op = Operation::Reward;
    assert(request(db, w).status == Status::Error);
    w.op = Operation::Employment;
    w.employed = true;
    auto hired = request(db, w);
    assert(hired.status == Status::Ok && hired.profile.courier);
    w.profile = hired.profile;
    w.op = Operation::Reward;
    auto paid = request(db, w);
    assert(paid.status == Status::Ok && paid.profile.balance == 25000 && paid.profile.deliveries == 1);
    w.profile = paid.profile;
    w.op = Operation::Position;
    w.profile.x = 1999;
    w.profile.y = 1400;
    w.profile.z = 11;
    w.profile.yaw = 45;
    assert(request(db, w).status == Status::Ok);
    w.op = Operation::Login;
    w.secret = "correct-test-password";
    auto saved = request(db, w);
    assert(saved.status == Status::Ok && saved.profile.balance == 25000 && saved.profile.x == 1999 &&
           saved.profile.hasPosition);
    w.op = Operation::Position;
    w.secret.clear();
    w.profile.x = std::numeric_limits<float>::quiet_NaN();
    assert(request(db, w).status == Status::Error);
    w.profile = saved.profile;
    w.op = Operation::Organization;
    w.organization = 1;
    w.score = 1;
    assert(request(db, w).status == Status::Error);
    w.score = 2;
    auto joined = request(db, w);
    assert(joined.status == Status::Ok && joined.profile.organization == 1 &&
           joined.profile.policeScore == 2);
    w.profile = joined.profile;
    w.op = Operation::BuyApartment;
    w.apartment = {2, 1, 1};
    assert(request(db, w).status == Status::Funds);
    w.apartment = {2, 6, 1};
    assert(request(db, w).status == Status::Error);
    w.op = Operation::Reward;
    for (int i = 0; i < 28; ++i) {
        auto reward = request(db, w);
        assert(reward.status == Status::Ok);
        w.profile = reward.profile;
    }
    Work second{Operation::Register, 5, 100, "Test_Beta", "correct-test-password", {}, false};
    auto other = request(db, second);
    assert(other.status == Status::Ok);
    second.profile = other.profile;
    second.secret.clear();
    second.op = Operation::Employment;
    second.employed = true;
    assert(request(db, second).status == Status::Ok);
    second.op = Operation::Reward;
    for (int i = 0; i < 29; ++i) {
        auto reward = request(db, second);
        assert(reward.status == Status::Ok);
        second.profile = reward.profile;
    }
    const int previous = w.profile.balance, otherPrevious = second.profile.balance;
    w.op = Operation::BuyApartment;
    w.apartment = {2, 1, 1};
    second.op = Operation::BuyApartment;
    second.apartment = w.apartment;
    assert(db.submit(w) && db.submit(second));
    std::vector<Result> purchases;
    for (int i = 0; i < 1000 && purchases.size() < 2; ++i) {
        for (auto &r : db.drain())
            purchases.push_back(r);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    assert(purchases.size() == 2 && purchases[0].status == Status::Ok &&
           purchases[1].status == Status::Occupied);
    assert(purchases[0].profile.balance == previous - 150000 && purchases[0].profile.apartments.size() == 1);
    second.op = Operation::Login;
    second.secret = "correct-test-password";
    auto unchanged = request(db, second);
    assert(unchanged.status == Status::Ok && unchanged.profile.balance == otherPrevious &&
           unchanged.profile.apartments.empty());
    w.op = Operation::Login;
    w.secret = "correct-test-password";
    auto homeowner = request(db, w);
    assert(homeowner.status == Status::Ok && homeowner.profile.balance == previous - 150000 &&
           homeowner.profile.apartments.size() == 1 && homeowner.profile.organization == 1 &&
           homeowner.profile.policeScore == 2);
    w.secret.clear();
    w.op = Operation::BuyApartment;
    w.apartment = {2, 1, 2};
    assert(request(db, w).status == Status::Funds);
    w.op = Operation::Lookup;
    w.nickname = "Test_' OR 1=1";
    assert(request(db, w).status == Status::Error);
    // All accepted tasks must retain a result even when the consumer is slow.
    unsigned accepted = 0, received = 0;
    for (int i = 0; i < 2500; ++i)
        if (db.submit(w))
            ++accepted;
    assert(accepted <= 1025);
    for (int i = 0; i < 1000 && received < accepted; ++i) {
        received += db.drain().size();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    assert(received == accepted);
    std::cout << "PASS: real MariaDB auth, payout, position persistence, organizations, atomic apartment "
                 "purchase, competing buyers, insufficient funds and queue backpressure\n";
}
