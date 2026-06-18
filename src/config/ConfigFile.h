#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace sim {

class ConfigFile {
public:
    bool load(const std::string& path);

    [[nodiscard]] bool isLoaded() const { return loaded_; }
    [[nodiscard]] const std::string& error() const { return error_; }
    [[nodiscard]] int getInt(std::string_view key, int fallback) const;
    [[nodiscard]] float getFloat(std::string_view key, float fallback) const;
    [[nodiscard]] bool getBool(std::string_view key, bool fallback) const;
    [[nodiscard]] std::string getString(std::string_view key, std::string fallback) const;

private:
    bool parseObject(std::string_view prefix);
    bool parseValue(const std::string& key);
    bool parseString(std::string& output);
    bool parseScalar(std::string& output);
    bool skipArray();
    void skipWhitespace();
    bool consume(char expected);
    void fail(std::string message);

    std::string source_;
    std::size_t cursor_ = 0;
    std::unordered_map<std::string, std::string> values_;
    std::string error_;
    bool loaded_ = false;
};

}  // namespace sim
