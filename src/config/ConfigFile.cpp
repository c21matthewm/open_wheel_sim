#include "config/ConfigFile.h"

#include <charconv>
#include <cctype>
#include <fstream>
#include <sstream>

namespace sim {

bool ConfigFile::load(const std::string& path) {
    source_.clear();
    values_.clear();
    error_.clear();
    cursor_ = 0;
    loaded_ = false;

    std::ifstream input(path);
    if (!input) {
        error_ = "Could not open config file: " + path;
        return false;
    }

    std::ostringstream contents;
    contents << input.rdbuf();
    source_ = contents.str();

    skipWhitespace();
    if (!parseObject("")) {
        return false;
    }
    skipWhitespace();
    if (cursor_ != source_.size()) {
        fail("Unexpected data after the root JSON object");
        return false;
    }

    loaded_ = true;
    return true;
}

int ConfigFile::getInt(std::string_view key, int fallback) const {
    const auto found = values_.find(std::string(key));
    if (found == values_.end()) {
        return fallback;
    }
    int result = fallback;
    const auto* begin = found->second.data();
    const auto* end = begin + found->second.size();
    const auto parsed = std::from_chars(begin, end, result);
    return parsed.ec == std::errc{} && parsed.ptr == end ? result : fallback;
}

float ConfigFile::getFloat(std::string_view key, float fallback) const {
    const auto found = values_.find(std::string(key));
    if (found == values_.end()) {
        return fallback;
    }
    try {
        return std::stof(found->second);
    } catch (...) {
        return fallback;
    }
}

bool ConfigFile::getBool(std::string_view key, bool fallback) const {
    const auto found = values_.find(std::string(key));
    if (found == values_.end()) {
        return fallback;
    }
    if (found->second == "true") {
        return true;
    }
    if (found->second == "false") {
        return false;
    }
    return fallback;
}

std::string ConfigFile::getString(std::string_view key, std::string fallback) const {
    const auto found = values_.find(std::string(key));
    return found == values_.end() ? std::move(fallback) : found->second;
}

bool ConfigFile::parseObject(std::string_view prefix) {
    if (!consume('{')) {
        fail("Expected JSON object");
        return false;
    }

    skipWhitespace();
    if (cursor_ < source_.size() && source_[cursor_] == '}') {
        ++cursor_;
        return true;
    }

    while (cursor_ < source_.size()) {
        std::string name;
        if (!parseString(name)) {
            fail("Expected a quoted object key");
            return false;
        }
        skipWhitespace();
        if (!consume(':')) {
            fail("Expected ':' after object key");
            return false;
        }

        const std::string key = prefix.empty() ? name : std::string(prefix) + "." + name;
        if (!parseValue(key)) {
            return false;
        }

        skipWhitespace();
        if (consume('}')) {
            return true;
        }
        if (!consume(',')) {
            fail("Expected ',' or '}' in object");
            return false;
        }
        skipWhitespace();
    }

    fail("Unterminated JSON object");
    return false;
}

bool ConfigFile::parseValue(const std::string& key) {
    skipWhitespace();
    if (cursor_ >= source_.size()) {
        fail("Expected a JSON value");
        return false;
    }

    if (source_[cursor_] == '{') {
        return parseObject(key);
    }
    if (source_[cursor_] == '[') {
        const std::size_t arrayStart = cursor_;
        if (!skipArray()) {
            return false;
        }
        values_[key] = source_.substr(arrayStart, cursor_ - arrayStart);
        return true;
    }

    std::string value;
    if (source_[cursor_] == '"') {
        if (!parseString(value)) {
            fail("Invalid quoted string value");
            return false;
        }
    } else if (!parseScalar(value)) {
        fail("Invalid scalar value");
        return false;
    }

    values_[key] = std::move(value);
    return true;
}

bool ConfigFile::parseString(std::string& output) {
    skipWhitespace();
    if (!consume('"')) {
        return false;
    }

    output.clear();
    while (cursor_ < source_.size()) {
        const char value = source_[cursor_++];
        if (value == '"') {
            return true;
        }
        if (value == '\\') {
            if (cursor_ >= source_.size()) {
                return false;
            }
            const char escaped = source_[cursor_++];
            switch (escaped) {
                case '"':
                case '\\':
                case '/':
                    output.push_back(escaped);
                    break;
                case 'n':
                    output.push_back('\n');
                    break;
                case 'r':
                    output.push_back('\r');
                    break;
                case 't':
                    output.push_back('\t');
                    break;
                default:
                    return false;
            }
        } else {
            output.push_back(value);
        }
    }
    return false;
}

bool ConfigFile::parseScalar(std::string& output) {
    skipWhitespace();
    const std::size_t start = cursor_;
    while (cursor_ < source_.size()) {
        const char value = source_[cursor_];
        if (value == ',' || value == '}' || std::isspace(static_cast<unsigned char>(value))) {
            break;
        }
        ++cursor_;
    }
    if (cursor_ == start) {
        return false;
    }
    output = source_.substr(start, cursor_ - start);
    return true;
}

bool ConfigFile::skipArray() {
    if (!consume('[')) {
        return false;
    }

    int depth = 1;
    bool inString = false;
    bool escaped = false;
    while (cursor_ < source_.size() && depth > 0) {
        const char value = source_[cursor_++];
        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (value == '\\') {
                escaped = true;
            } else if (value == '"') {
                inString = false;
            }
            continue;
        }
        if (value == '"') {
            inString = true;
        } else if (value == '[') {
            ++depth;
        } else if (value == ']') {
            --depth;
        }
    }
    if (depth != 0) {
        fail("Unterminated JSON array");
        return false;
    }
    return true;
}

void ConfigFile::skipWhitespace() {
    while (cursor_ < source_.size() &&
           std::isspace(static_cast<unsigned char>(source_[cursor_]))) {
        ++cursor_;
    }
}

bool ConfigFile::consume(char expected) {
    skipWhitespace();
    if (cursor_ >= source_.size() || source_[cursor_] != expected) {
        return false;
    }
    ++cursor_;
    return true;
}

void ConfigFile::fail(std::string message) {
    error_ = std::move(message) + " near byte " + std::to_string(cursor_);
}

}  // namespace sim
