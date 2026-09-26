#include "parse.h"

namespace nano_edr {

bool IsBlankOrComment(const std::string* line) {
    if (line == nullptr) {
        return true;
    }

    std::size_t pos = 0;

    while (pos < line->size() &&
           ((*line)[pos] == ' ' || (*line)[pos] == '\t')) {
        ++pos;
    }

    if (pos == line->size()) {
        return true;
    }

    return (*line)[pos] == '#' || (*line)[pos] == ';';
}

bool ParseEventLine(const std::string* line, Event* out) {
    if (line == nullptr || out == nullptr) {
        return false;
    }

    *out = {};

    if (IsBlankOrComment(line)) {
        return false;
    }

    bool has_ts = false;
    bool has_type = false;
    bool has_pid = false;

    std::size_t pos = 0;

    while (pos < line->size()) {
        while (pos < line->size() &&
               ((*line)[pos] == ' ' || (*line)[pos] == '\t')) {
            ++pos;
        }

        if (pos == line->size()) {
            break;
        }

        std::size_t key_begin = pos;

        while (pos < line->size() &&
               (*line)[pos] != '=' &&
               (*line)[pos] != ' ' &&
               (*line)[pos] != '\t') {
            ++pos;
        }

        if (pos == key_begin ||
            pos == line->size() ||
            (*line)[pos] != '=') {
            return false;
        }

        std::string key = line->substr(key_begin, pos - key_begin);
        ++pos;

        std::string value;

        if (pos < line->size() && (*line)[pos] == '"') {
            ++pos;

            std::size_t value_begin = pos;

            while (pos < line->size() && (*line)[pos] != '"') {
                ++pos;
            }

            if (pos == line->size()) {
                return false;
            }

            value = line->substr(value_begin, pos - value_begin);
            ++pos;

            if (pos < line->size() &&
                (*line)[pos] != ' ' &&
                (*line)[pos] != '\t') {
                return false;
            }
        } else {
            std::size_t value_begin = pos;

            while (pos < line->size() &&
                   (*line)[pos] != ' ' &&
                   (*line)[pos] != '\t') {
                ++pos;
            }

            value = line->substr(value_begin, pos - value_begin);
        }

        if (key == "ts" && !has_ts) {
            out->ts = value;
            has_ts = true;
        } else if (key == "type" && !has_type) {
            out->type = value;
            has_type = true;
        } else if (key == "pid" && !has_pid) {
            out->pid = value;
            has_pid = true;
        } else {
            out->fields.push_back({key, value});
        }
    }

    return has_ts && has_type;
}

}  // namespace nano_edr