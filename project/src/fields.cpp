#include "fields.h"

#include <charconv>
#include <cctype>
#include <stdexcept>
#include <system_error>

namespace nano_edr {
namespace {

char ToLowerAscii(char value) {
    return static_cast<char>(
        std::tolower(static_cast<unsigned char>(value)));
}

std::string LowerAscii(const std::string& text) {
    std::string result;
    result.reserve(text.size());

    for (char value : text) {
        result.push_back(ToLowerAscii(value));
    }

    return result;
}

bool EndsWith(const std::string& text, const std::string& suffix) {
    return text.size() >= suffix.size() &&
           text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

}  // namespace

const std::string* FindField(const Event& event, const std::string& key) {
    for (const Field& field : event.fields) {
        if (field.key == key) {
            return &field.value;
        }
    }

    return nullptr;
}

const std::string& GetRequiredField(const Event& event,
                                    const std::string& key) {
    const std::string* value = FindField(event, key);

    if (value == nullptr) {
        throw std::invalid_argument("required field is missing: " + key);
    }

    return *value;
}

bool GetIntField(const Event& event, const std::string& key, uint64_t* out) {
    if (out == nullptr) {
        return false;
    }

    const std::string* value = FindField(event, key);

    if (value == nullptr || value->empty()) {
        return false;
    }

    uint64_t parsed = 0;
    const char* begin = value->data();
    const char* end = begin + value->size();
    const auto result = std::from_chars(begin, end, parsed);

    if (result.ec != std::errc{} || result.ptr != end) {
        return false;
    }

    *out = parsed;
    return true;
}

uint64_t GetIntField(const Event& event,
                     const std::string& key,
                     uint64_t fallback) {
    uint64_t value = 0;

    if (!GetIntField(event, key, &value)) {
        return fallback;
    }

    return value;
}

bool IsProcessStart(const Event& event) {
    return event.type == "process_start";
}

bool IsFileWrite(const Event& event) {
    return event.type == "file_write";
}

bool IsNetConnect(const Event& event) {
    return event.type == "net_connect";
}

bool PathEndsWith(const Event& event, const std::string& suffix) {
    const std::string* path = FindField(event, "path");

    if (path == nullptr) {
        return false;
    }

    return EndsWith(LowerAscii(*path), LowerAscii(suffix));
}

bool CommandLineContains(const Event& event, const std::string& needle) {
    const std::string* cmdline = FindField(event, "cmdline");

    if (cmdline == nullptr) {
        return false;
    }

    return LowerAscii(*cmdline).find(LowerAscii(needle)) != std::string::npos;
}

std::string NormalizePath(const std::string& path) {
    std::string normalized;
    normalized.reserve(path.size());

    for (char value : path) {
        char current = ToLowerAscii(value);

        if (current == '/') {
            current = '\\';
        }

        if (current == '\\' && !normalized.empty() &&
            normalized.back() == '\\') {
            continue;
        }

        normalized.push_back(current);
    }

    constexpr const char* kTemp = "%temp%";
    constexpr const char* kTmp = "%tmp%";
    constexpr const char* kTempPath = "\\appdata\\local\\temp";

    std::string result;
    result.reserve(normalized.size());

    std::size_t pos = 0;

    while (pos < normalized.size()) {
        if (normalized.compare(pos, 6, kTemp) == 0) {
            result += kTempPath;
            pos += 6;
        } else if (normalized.compare(pos, 5, kTmp) == 0) {
            result += kTempPath;
            pos += 5;
        } else {
            result.push_back(normalized[pos]);
            ++pos;
        }
    }

    return result;
}

}  // namespace nano_edr
