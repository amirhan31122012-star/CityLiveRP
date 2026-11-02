#pragma once
#include "Config.hpp"
#include "Rules.hpp"
#include "World.hpp"
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <iostream>
#include <locale>
#include <mutex>
#include <mysql.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <sstream>
#include <thread>
#include <vector>

namespace city {
struct Profile {
    uint64_t id = 0;
    int balance = StartingBalance;
    float x = 1958.33f, y = 1343.12f, z = 15.36f, yaw = 90;
    bool hasPosition = false, courier = false;
    unsigned deliveries = 0;
    int organization = 0, policeScore = 0, hospitalScore = 0;
    std::vector<Apartment> apartments;
};
enum class Operation { Lookup, Login, Register, Position, Employment, Reward, BuyApartment, Organization };
struct Work {
    Operation op;
    int player;
    uint64_t generation;
    std::string nickname, secret;
    Profile profile;
    bool employed = false;
    Apartment apartment;
    int organization = 0, score = 0;
};
enum class Status { Ok, Missing, Exists, WrongPassword, Occupied, Funds, Error };
struct Result {
    Operation op;
    int player;
    uint64_t generation;
    Status status = Status::Error;
    Profile profile;
};
inline std::string hex(const unsigned char *bytes, size_t count) {
    constexpr char digits[] = "0123456789abcdef";
    std::string out;
    for (size_t i = 0; i < count; ++i) {
        out += digits[bytes[i] >> 4];
        out += digits[bytes[i] & 15];
    }
    return out;
}
inline std::string derive(const std::string &secret, const std::string &salt, unsigned iterations) {
    if (iterations < 100000 || iterations > 2000000 || salt.size() != 32)
        throw std::runtime_error("Invalid password record");
    unsigned char digest[32];
    if (!PKCS5_PBKDF2_HMAC(secret.data(), static_cast<int>(secret.size()),
                           reinterpret_cast<const unsigned char *>(salt.data()),
                           static_cast<int>(salt.size()), static_cast<int>(iterations), EVP_sha256(), 32,
                           digest))
        throw std::runtime_error("Password hashing failed");
    auto out = hex(digest, 32);
    OPENSSL_cleanse(digest, sizeof(digest));
    return out;
}
class Database {
    std::mutex mutex;
    std::condition_variable changed;
    std::deque<Work> waiting;
    std::deque<Result> finished;
    bool stopping = false;
    MYSQL *connection = nullptr;
    std::thread worker;
    std::string quote(const std::string &text) {
        std::string escaped(text.size() * 2 + 1, '\0');
        auto n = mysql_real_escape_string(connection, escaped.data(), text.data(),
                                          static_cast<unsigned long>(text.size()));
        escaped.resize(n);
        return "'" + escaped + "'";
    }
    void execute(const std::string &query) {
        if (mysql_real_query(connection, query.data(), query.size()))
            throw std::runtime_error("Database operation failed");
    }
    static std::string number(double value) {
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out.precision(10);
        out << value;
        return out.str();
    }
    bool read(const std::string &nickname, Profile &p, std::string &salt, std::string &hash,
              unsigned &iterations) {
        execute("SELECT "
                "id,password_salt,password_hash,password_iterations,balance,x,y,z,yaw,has_position,courier,"
                "deliveries,organization,police_score,hospital_score FROM citylive_accounts WHERE nickname=" +
                quote(nickname) + " LIMIT 1");
        MYSQL_RES *result = mysql_store_result(connection);
        if (!result)
            throw std::runtime_error("Database read failed");
        auto row = mysql_fetch_row(result);
        if (!row) {
            mysql_free_result(result);
            return false;
        }
        try {
            for (int i = 0; i < 15; ++i)
                if (!row[i])
                    throw std::runtime_error("Invalid account record");
            p.id = std::stoull(row[0]);
            salt = row[1];
            hash = row[2];
            iterations = std::stoul(row[3]);
            p.balance = std::stoi(row[4]);
            p.x = std::stof(row[5]);
            p.y = std::stof(row[6]);
            p.z = std::stof(row[7]);
            p.yaw = std::stof(row[8]);
            p.hasPosition = std::stoi(row[9]) != 0;
            p.courier = std::stoi(row[10]) != 0;
            p.deliveries = std::stoul(row[11]);
            p.organization = std::stoi(row[12]);
            p.policeScore = std::stoi(row[13]);
            p.hospitalScore = std::stoi(row[14]);
            if (p.balance < 0 || p.organization < 0 || p.organization > 2 || p.policeScore < 0 ||
                p.policeScore > 3 || p.hospitalScore < 0 || p.hospitalScore > 3 ||
                !validPosition(p.x, p.y, p.z, p.yaw))
                throw std::runtime_error("Invalid account state");
        } catch (...) {
            mysql_free_result(result);
            throw;
        }
        mysql_free_result(result);
        execute("SELECT building,floor,door FROM citylive_apartments WHERE owner_id=" + std::to_string(p.id) +
                " ORDER BY building,floor,door");
        result = mysql_store_result(connection);
        if (!result)
            throw std::runtime_error("Apartment read failed");
        p.apartments.clear();
        try {
            while (auto row = mysql_fetch_row(result)) {
                if (!row[0] || !row[1] || !row[2])
                    throw std::runtime_error("Invalid apartment record");
                Apartment a{std::stoi(row[0]), std::stoi(row[1]), std::stoi(row[2])};
                if (!validApartment(a))
                    throw std::runtime_error("Invalid apartment record");
                p.apartments.push_back(a);
            }
        } catch (...) {
            mysql_free_result(result);
            throw;
        }
        mysql_free_result(result);
        return true;
    }
    Result process(const Work &w) {
        Result r{w.op, w.player, w.generation, Status::Error, {}};
        if (!connection)
            return r;
        if (w.op == Operation::Position) {
            if (!w.profile.id || !validPosition(w.profile.x, w.profile.y, w.profile.z, w.profile.yaw))
                return r;
            execute("UPDATE citylive_accounts SET x=" + number(w.profile.x) + ",y=" + number(w.profile.y) +
                    ",z=" + number(w.profile.z) + ",yaw=" + number(w.profile.yaw) +
                    ",has_position=1 WHERE id=" + std::to_string(w.profile.id));
            r.status = Status::Ok;
            return r;
        }
        if (!validName(w.nickname))
            return r;
        std::string salt, hash;
        unsigned iterations = 0;
        bool exists = read(w.nickname, r.profile, salt, hash, iterations);
        if (w.op == Operation::Lookup) {
            r.status = exists ? Status::Exists : Status::Missing;
            return r;
        }
        if (w.op == Operation::Register) {
            if (exists) {
                r.status = Status::Exists;
                return r;
            }
            if (w.secret.size() < 8 || w.secret.size() > 64)
                return r;
            unsigned char random[16];
            if (RAND_bytes(random, 16) != 1)
                return r;
            salt = hex(random, 16);
            iterations = 600000;
            hash = derive(w.secret, salt, iterations);
            execute("INSERT INTO citylive_accounts(nickname,password_salt,password_hash,password_iterations) "
                    "VALUES(" +
                    quote(w.nickname) + "," + quote(salt) + "," + quote(hash) + ",600000)");
            exists = read(w.nickname, r.profile, salt, hash, iterations);
            r.status = exists ? Status::Ok : Status::Error;
            return r;
        }
        if (!exists) {
            r.status = Status::Missing;
            return r;
        }
        if (w.op == Operation::Login) {
            if (w.secret.size() < 8 || w.secret.size() > 64) {
                r.status = Status::WrongPassword;
                return r;
            }
            auto supplied = derive(w.secret, salt, iterations);
            r.status = hash.size() == supplied.size() &&
                               CRYPTO_memcmp(hash.data(), supplied.data(), hash.size()) == 0
                           ? Status::Ok
                           : Status::WrongPassword;
            OPENSSL_cleanse(supplied.data(), supplied.size());
            return r;
        }
        if (r.profile.id != w.profile.id)
            return r;
        if (w.op == Operation::BuyApartment) {
            if (!validApartment(w.apartment))
                return r;
            const auto a = w.apartment;
            const int cost = Buildings[a.building].price;
            execute("START TRANSACTION");
            try {
                execute("SELECT balance FROM citylive_accounts WHERE id=" + std::to_string(r.profile.id) +
                        " FOR UPDATE");
                MYSQL_RES *balanceResult = mysql_store_result(connection);
                if (!balanceResult)
                    throw std::runtime_error("Balance lock failed");
                auto balanceRow = mysql_fetch_row(balanceResult);
                int current = -1;
                try {
                    if (balanceRow && balanceRow[0])
                        current = std::stoi(balanceRow[0]);
                } catch (...) {
                    mysql_free_result(balanceResult);
                    throw;
                }
                mysql_free_result(balanceResult);
                if (current < cost) {
                    mysql_rollback(connection);
                    r.status = Status::Funds;
                    return r;
                }
                const std::string insert =
                    "INSERT INTO citylive_apartments(building,floor,door,owner_id) VALUES(" +
                    std::to_string(a.building) + "," + std::to_string(a.floor) + "," +
                    std::to_string(a.door) + "," + std::to_string(r.profile.id) + ")";
                if (mysql_real_query(connection, insert.data(), insert.size())) {
                    auto error = mysql_errno(connection);
                    mysql_rollback(connection);
                    if (error == 1062) {
                        r.status = Status::Occupied;
                        return r;
                    }
                    throw std::runtime_error("Apartment purchase failed");
                }
                execute("UPDATE citylive_accounts SET balance=balance-" + std::to_string(cost) +
                        " WHERE id=" + std::to_string(r.profile.id) +
                        " AND balance>=" + std::to_string(cost));
                if (mysql_affected_rows(connection) != 1)
                    throw std::runtime_error("Purchase balance changed");
                if (mysql_commit(connection) != 0)
                    throw std::runtime_error("Purchase commit failed");
                r.profile.balance = current - cost;
                r.profile.apartments.push_back(a);
                r.status = Status::Ok;
                return r;
            } catch (...) {
                mysql_rollback(connection);
                throw;
            }
        }
        if (w.op == Operation::Organization) {
            if (!passedQuiz(w.organization, w.score))
                return r;
            const std::string field = w.organization == 1 ? "police_score" : "hospital_score";
            execute("UPDATE citylive_accounts SET organization=" + std::to_string(w.organization) + "," +
                    field + "=" + std::to_string(w.score) + " WHERE id=" + std::to_string(r.profile.id));
            r.profile.organization = w.organization;
            if (w.organization == 1)
                r.profile.policeScore = w.score;
            else
                r.profile.hospitalScore = w.score;
            r.status = Status::Ok;
            return r;
        }
        if (w.op == Operation::Employment) {
            execute("UPDATE citylive_accounts SET courier=" + std::to_string(w.employed ? 1 : 0) +
                    " WHERE id=" + std::to_string(r.profile.id));
            r.profile.courier = w.employed;
        } else if (w.op == Operation::Reward) {
            if (!r.profile.courier || r.profile.balance > 2147483647 - DeliveryReward ||
                r.profile.deliveries >= 4294967295u)
                return r;
            execute("UPDATE citylive_accounts SET balance=balance+5000,deliveries=deliveries+1 WHERE id=" +
                    std::to_string(r.profile.id) +
                    " AND courier=1 AND balance<=2147478647 AND deliveries<4294967295");
            if (mysql_affected_rows(connection) != 1)
                return r;
            r.profile.balance += DeliveryReward;
            ++r.profile.deliveries;
        } else
            return r;
        r.status = Status::Ok;
        return r;
    }
    void run() {
        static std::once_flag initialized;
        std::call_once(initialized, [] { mysql_library_init(0, nullptr, nullptr); });
        mysql_thread_init();
        try {
            auto c = loadConfig("scriptfiles/citylive/database.ini");
            connection = mysql_init(nullptr);
            if (!connection)
                throw std::runtime_error("Database initialization failed");
            unsigned timeout = 5;
            mysql_options(connection, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
            mysql_options(connection, MYSQL_OPT_READ_TIMEOUT, &timeout);
            mysql_options(connection, MYSQL_OPT_WRITE_TIMEOUT, &timeout);
            if (c["host"] != "127.0.0.1" && c["host"] != "localhost") {
                my_bool enforce = 1, verify = 1;
                mysql_options(connection, MYSQL_OPT_SSL_ENFORCE, &enforce);
                mysql_options(connection, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, &verify);
                if (c["tls_ca"].empty())
                    throw std::runtime_error("Remote database requires tls_ca");
                mysql_ssl_set(connection, nullptr, nullptr, c["tls_ca"].c_str(), nullptr, nullptr);
            } else {
                // Local socket/loopback is trusted; MariaDB may offer a self-signed TLS cert.
                my_bool verify = 0;
                mysql_options(connection, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, &verify);
            }
            unsigned port = c.count("port") ? std::stoul(c["port"]) : 3306;
            if (port == 0 || port > 65535)
                throw std::runtime_error("Invalid database port");
            if (!mysql_real_connect(connection, c["host"].c_str(), c["user"].c_str(), c["password"].c_str(),
                                    c["database"].c_str(), port, nullptr, 0))
                throw std::runtime_error("Database connection failed, code " +
                                         std::to_string(mysql_errno(connection)));
            if (mysql_set_character_set(connection, "ascii") != 0 || mysql_autocommit(connection, 1) != 0)
                throw std::runtime_error("Database session setup failed");
            std::clog << "[CityLive] MySQL connected.\n";
        } catch (const std::exception &e) {
            if (connection)
                mysql_close(connection);
            connection = nullptr;
            std::clog << "[CityLive] MySQL unavailable: " << e.what()
                      << ". Check database.ini, TLS and schema.sql. Login disabled.\n";
        }
        for (;;) {
            Work w;
            {
                std::unique_lock<std::mutex> lock(mutex);
                changed.wait(lock, [this] { return stopping || !waiting.empty(); });
                if (waiting.empty() && stopping)
                    break;
                w = std::move(waiting.front());
                waiting.pop_front();
            }
            Result r{w.op, w.player, w.generation, Status::Error, {}};
            try {
                r = process(w);
            } catch (const std::exception &) {
                std::clog << "[CityLive] Database request failed; no success was issued.\n";
            }
            if (!w.secret.empty())
                OPENSSL_cleanse(w.secret.data(), w.secret.size());
            {
                std::lock_guard<std::mutex> lock(mutex);
                finished.push_back(std::move(r));
            }
        }
        if (connection)
            mysql_close(connection);
        mysql_thread_end();
        std::clog << "[CityLive] Database worker stopped.\n";
    }

  public:
    Database() : worker([this] { run(); }) {}
    ~Database() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            stopping = true;
        }
        changed.notify_one();
        worker.join();
    }
    bool submit(Work w) {
        std::lock_guard<std::mutex> lock(mutex);
        if (stopping || waiting.size() + finished.size() >= 1024) {
            if (!w.secret.empty())
                OPENSSL_cleanse(w.secret.data(), w.secret.size());
            return false;
        }
        waiting.push_back(std::move(w));
        changed.notify_one();
        return true;
    }
    std::vector<Result> drain() {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<Result> values;
        while (!finished.empty()) {
            values.push_back(std::move(finished.front()));
            finished.pop_front();
        }
        return values;
    }
};
} // namespace city
