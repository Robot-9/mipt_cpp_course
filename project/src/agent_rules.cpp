#include "agent_rules.h"

#include <string>

#include "fields.h"

namespace nano_edr {
namespace {

bool EndsWith(const std::string& text, const std::string& suffix) {
    return text.size() >= suffix.size() &&
           text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool BaseNameEquals(const std::string& path, const std::string& name) {
    const std::size_t slash = path.find_last_of('\\');
    const std::size_t begin = slash == std::string::npos ? 0 : slash + 1;

    return path.size() - begin == name.size() &&
           path.compare(begin, name.size(), name) == 0;
}

bool IsFileMutation(const Event& event) {
    return event.type == "file_create" || event.type == "file_write" ||
           event.type == "file_move";
}

const std::string* FileMutationPath(const Event& event) {
    if (event.type == "file_move") {
        return FindField(event, "to");
    }

    return FindField(event, "path");
}

bool ScriptHostFromTemp(const Event& event) {
    if (!IsProcessStart(event)) {
        return false;
    }

    const std::string image = NormalizePath(GetRequiredField(event, "image"));

    if (!BaseNameEquals(image, "wscript.exe") &&
        !BaseNameEquals(image, "cscript.exe")) {
        return false;
    }

    const std::string* cmdline = FindField(event, "cmdline");

    if (cmdline == nullptr) {
        return false;
    }

    const std::string command = NormalizePath(*cmdline);

    return command.find("\\appdata\\local\\temp\\") != std::string::npos ||
           command.find("\\windows\\temp\\") != std::string::npos;
}

bool LolbinDownload(const Event& event) {
    if (!IsProcessStart(event)) {
        return false;
    }

    const std::string image = NormalizePath(GetRequiredField(event, "image"));

    if (!BaseNameEquals(image, "certutil.exe") &&
        !BaseNameEquals(image, "bitsadmin.exe")) {
        return false;
    }

    const std::string* cmdline = FindField(event, "cmdline");

    if (cmdline == nullptr) {
        return false;
    }

    const std::string command = NormalizePath(*cmdline);

    return command.find("urlcache") != std::string::npos ||
           command.find("transfer") != std::string::npos ||
           command.find("http:") != std::string::npos ||
           command.find("https:") != std::string::npos;
}

bool HiddenPowershell(const Event& event) {
    if (!IsProcessStart(event)) {
        return false;
    }

    const std::string image = NormalizePath(GetRequiredField(event, "image"));

    if (!BaseNameEquals(image, "powershell.exe") &&
        !BaseNameEquals(image, "pwsh.exe")) {
        return false;
    }

    const std::string* cmdline = FindField(event, "cmdline");

    if (cmdline == nullptr) {
        return false;
    }

    const std::string command = NormalizePath(*cmdline);

    return command.find("-w hidden") != std::string::npos ||
           command.find("-windowstyle hidden") != std::string::npos ||
           command.find("-enc") != std::string::npos ||
           command.find("-encodedcommand") != std::string::npos;
}

bool AutostartWrite(const Event& event) {
    if (!IsFileMutation(event)) {
        return false;
    }

    const std::string* path = FileMutationPath(event);

    if (path == nullptr) {
        return false;
    }

    return NormalizePath(*path).find("\\start menu\\programs\\startup\\") !=
           std::string::npos;
}

bool RansomExtension(const Event& event) {
    if (!IsFileMutation(event)) {
        return false;
    }

    const std::string* path = FileMutationPath(event);

    if (path == nullptr) {
        return false;
    }

    return EndsWith(NormalizePath(*path), ".locked");
}

constexpr Rule kRules[] = {
    {"script_host_from_temp", ScriptHostFromTemp, Severity::kHigh},
    {"lolbin_download", LolbinDownload, Severity::kHigh},
    {"hidden_powershell", HiddenPowershell, Severity::kMedium},
    {"autostart_write", AutostartWrite, Severity::kHigh},
    {"ransom_extension", RansomExtension, Severity::kCritical},
};

}  // namespace

const Rule* AgentRules() {
    return kRules;
}

size_t AgentRuleCount() {
    return sizeof(kRules) / sizeof(kRules[0]);
}

}  // namespace nano_edr
