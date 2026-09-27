#include <charconv>
#include <exception>
#include <fstream>
#include <print>
#include <string>
#include <system_error>
#include <vector>

#include "agent_rules.h"
#include "event_list.h"
#include "parse.h"
#include "rules.h"

namespace {

struct Options {
    std::string path;
    bool quiet = false;
    std::size_t window_size = 64;
};

struct Statistics {
    long long event_count = 0;
    long long detection_count = 0;
    std::vector<std::string> types;
    std::vector<long long> type_counts;
};

bool ParseArguments(int argc, char** argv, Options* options) {
    if (options == nullptr) {
        return false;
    }

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--quiet") {
            options->quiet = true;
            continue;
        }

        if (arg == "--window-size") {
            if (i + 1 >= argc) {
                std::print(stderr, "не указано значение --window-size\n");
                return false;
            }

            const std::string value = argv[++i];
            const auto result = std::from_chars(
                value.data(),
                value.data() + value.size(),
                options->window_size);

            if (result.ec != std::errc{} ||
                result.ptr != value.data() + value.size()) {
                std::print(stderr, "неверный размер окна: {}\n", value);
                return false;
            }

            continue;
        }

        if (!arg.empty() && arg[0] == '-') {
            std::print(stderr, "неизвестный аргумент: {}\n", arg);
            return false;
        }

        if (!options->path.empty()) {
            std::print(stderr, "лишний аргумент: {}\n", arg);
            return false;
        }

        options->path = arg;
    }

    if (options->path.empty()) {
        std::print(
            stderr,
            "использование: nano-edr <журнал.log> [--quiet] "
            "[--window-size N]\n");
        return false;
    }

    return true;
}

void PrintContext(const nano_edr::EventList& list,
                  const nano_edr::EventNode* previous_tail) {
    if (previous_tail != nullptr) {
        std::print(
            "[CTX] -2: ts={} type={} pid={}\n",
            previous_tail->event.ts,
            previous_tail->event.type,
            previous_tail->event.pid);
    }

    if (list.tail != nullptr) {
        std::print(
            "[CTX] -1: ts={} type={} pid={}\n",
            list.tail->event.ts,
            list.tail->event.type,
            list.tail->event.pid);
    }
}

void PushEvent(nano_edr::EventList* list,
               nano_edr::EventNode** previous_tail,
               const nano_edr::Event* event) {
    nano_edr::EventNode* old_tail = list->tail;

    nano_edr::ListPushBack(list, event);

    *previous_tail = list->size >= 2 ? old_tail : nullptr;
}

void CountEventType(const std::string& type, Statistics* statistics) {
    std::size_t type_index = 0;

    while (type_index < statistics->types.size() &&
           statistics->types[type_index] != type) {
        ++type_index;
    }

    if (type_index == statistics->types.size()) {
        statistics->types.push_back(type);
        statistics->type_counts.push_back(1);
    } else {
        ++statistics->type_counts[type_index];
    }
}

void PrintSummary(const Statistics& statistics) {
    std::print(
        "событий {}, детектов {}\n",
        statistics.event_count,
        statistics.detection_count);

    for (std::size_t i = 0; i < statistics.types.size(); ++i) {
        std::print("{}: {}\n", statistics.types[i], statistics.type_counts[i]);
    }
}

void ProcessLog(std::ifstream* log, const Options& options) {
    nano_edr::EventList window;
    window.capacity = options.window_size;
    nano_edr::EventNode* previous_tail = nullptr;

    Statistics statistics;
    std::string line;

    while (std::getline(*log, line)) {
        if (nano_edr::IsBlankOrComment(&line)) {
            continue;
        }

        nano_edr::Event event;

        if (!nano_edr::ParseEventLine(&line, &event)) {
            continue;
        }

        ++statistics.event_count;
        CountEventType(event.type, &statistics);

        const std::size_t detections = nano_edr::CheckRules(
            event,
            nano_edr::AgentRules(),
            nano_edr::AgentRuleCount());

        statistics.detection_count += static_cast<long long>(detections);

        if (detections != 0 && !options.quiet) {
            PrintContext(window, previous_tail);
        }

        PushEvent(&window, &previous_tail, &event);
    }

    if (!options.quiet) {
        PrintSummary(statistics);
    }
}

int Run(int argc, char** argv) {
    Options options;

    if (!ParseArguments(argc, argv, &options)) {
        return 2;
    }

    std::ifstream log(options.path);

    if (!log) {
        std::print(stderr, "не удалось открыть журнал: {}\n", options.path);
        return 2;
    }

    ProcessLog(&log, options);
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return Run(argc, argv);
    } catch (const std::exception& error) {
        std::print(stderr, "ошибка: {}\n", error.what());
        return 1;
    }
}
