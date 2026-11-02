#pragma once
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
namespace city {
using Config = std::map<std::string, std::string>;
inline Config loadConfig(const std::string &path) {
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Missing scriptfiles/citylive/database.ini");
    Config values;
    std::string line;
    size_t bytes = 0;
    while (std::getline(file, line)) {
        bytes += line.size();
        if (bytes > 65536)
            throw std::runtime_error("Configuration too large");
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty() || line[0] == '#' || line[0] == ';')
            continue;
        auto split = line.find('=');
        if (split == std::string::npos)
            throw std::runtime_error("Invalid configuration line");
        auto key = line.substr(0, split), value = line.substr(split + 1);
        if (!values.emplace(key, value).second)
            throw std::runtime_error("Duplicate configuration key");
    }
    for (const auto *key : {"host", "user", "password", "database"})
        if (values[key].empty())
            throw std::runtime_error(std::string("Missing database setting: ") + key);
    return values;
}
} // namespace city
