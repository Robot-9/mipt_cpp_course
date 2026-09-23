#include <charconv>
#include <cstdio>
#include <fstream>
#include <print>
#include <string>
#include <system_error>
#include <vector>

#include "event_list.h"
#include "parse.h"

namespace {

void PrintContext(const nano_edr::EventList* list) {
    std::size_t count = list->size < 2 ? list->size : 2;
    std::size_t skip = list->size - count;

    const nano_edr::EventNode* node = list->head;

    for (std::size_t i = 0; i < skip; ++i) {
        node = node->next;
    }

    int offset = -static_cast<int>(count);

    while (node != nullptr) {
        std::print(
            "[CTX] {}: ts={} type={} pid={}\n",
            offset,
            node->event.ts,
            node->event.type,
            node->event.pid
        );

        ++offset;
        node = node->next;
    }
}

}

int main(int argc, char** argv) {
    std::string path;
    bool quiet = false;
    std::size_t window_size = 64;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--quiet") {
            quiet = true;
            continue;
        }

        if (arg == "--window-size") {
            if (i + 1 >= argc) {
                std::print(stderr, "не указано значение --window-size\n");
                return 2;
            }

            std::string value = argv[++i];

            auto result = std::from_chars(
                value.data(),
                value.data() + value.size(),
                window_size
            );

            if (result.ec != std::errc{} ||
                result.ptr != value.data() + value.size()) {
                std::print(stderr, "неверный размер окна: {}\n", value);
                return 2;
            }

            continue;
        }

        if (!arg.empty() && arg[0] == '-') {
            std::print(stderr, "неизвестный аргумент: {}\n", arg);
            return 2;
        }

        if (!path.empty()) {
            std::print(stderr, "лишний аргумент: {}\n", arg);
            return 2;
        }

        path = arg;
    }

    if (path.empty()) {
        std::print(
            stderr,
            "использование: nano-edr <журнал.log> [--quiet] [--window-size N]\n"
        );
        return 2;
    }

    std::ifstream log(path);

    if (!log) {
        std::print(stderr, "не удалось открыть журнал: {}\n", path);
        return 2;
    }

    nano_edr::EventList window;
    window.capacity = window_size;

    const std::string signs[] = {
        "wscript.exe",
        ".locked",
        "certutil.exe",
        "\\Startup\\"
    };

    long long line_number = 0;
    long long event_count = 0;
    long long detection_count = 0;

    std::vector<std::string> types;
    std::vector<long long> type_counts;

    std::string line;

    while (std::getline(log, line)) {
        ++line_number;

        if (nano_edr::IsBlankOrComment(&line)) {
            continue;
        }

        nano_edr::Event event;

        if (!nano_edr::ParseEventLine(&line, &event)) {
            continue;
        }

        ++event_count;

        std::size_t type_index = 0;

        while (type_index < types.size() &&
               types[type_index] != event.type) {
            ++type_index;
        }

        if (type_index == types.size()) {
            types.push_back(event.type);
            type_counts.push_back(1);
        } else {
            ++type_counts[type_index];
        }

        bool detected = false;

        for (const std::string& sign : signs) {
            if (line.find(sign) == std::string::npos) {
                continue;
            }

            std::print(
                "[DETECT] строка {}, признак {}: {}\n",
                line_number,
                sign,
                line
            );

            detected = true;
            ++detection_count;
        }

        if (detected && !quiet) {
            PrintContext(&window);
        }

        nano_edr::ListPushBack(&window, &event);
    }

    if (!quiet) {
        std::print(
            "событий {}, детектов {}\n",
            event_count,
            detection_count
        );

        for (std::size_t i = 0; i < types.size(); ++i) {
            std::print("{}: {}\n", types[i], type_counts[i]);
        }
    }

    return 0;
}