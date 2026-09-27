#include "rules.h"

#include <print>

namespace nano_edr {

const char* SeverityName(Severity severity) {
    switch (severity) {
        case Severity::kLow:
            return "low";
        case Severity::kMedium:
            return "medium";
        case Severity::kHigh:
            return "high";
        case Severity::kCritical:
            return "critical";
    }

    return "?";
}

size_t CheckRules(const Event& event, const Rule* rules, size_t rule_count) {
    if (rules == nullptr) {
        return 0;
    }

    size_t detection_count = 0;

    for (size_t i = 0; i < rule_count; ++i) {
        const Rule& rule = rules[i];

        if (rule.check == nullptr || !rule.check(event)) {
            continue;
        }

        ++detection_count;

        const char* id = rule.id == nullptr ? "?" : rule.id;
        std::print(
            "[DETECT] {}  {}  ts={} pid={}\n",
            SeverityName(rule.severity),
            id,
            event.ts,
            event.pid);
    }

    return detection_count;
}

}  // namespace nano_edr
