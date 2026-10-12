// ProsperoEden - Bundled game presets and profile-local, atomic apply/undo.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "settings_store.h"
#include <cerrno>
#include <sys/stat.h>

#ifndef EDEN_PRESET_REPORT_REPOSITORY
#define EDEN_PRESET_REPORT_REPOSITORY "blackbearreloaded/ProsperoEden"
#endif

namespace Eden::Presets {
using Json = Settings::Json;
struct Preset {
    uint64_t title;
    const char* id;
    const char* version;
    const char* evidence;
    const char* changes;
    bool experimental = true;
};
// Candidate only: a tester's subjective improvement is not a full-speed claim.
inline constexpr Preset kDatabase[] = {{
    0x0100965017338000ULL, "jamboree-base-ps5-1", "01.000.110",
    "Experimental; original PS5, firmware 4.50, Eden 1.000.110.\n"
    "Smoother play reported; exact scene and final graphics unconfirmed.\n"
    "Game update file: 2.3.0; other versions and consoles untested.",
    R"({"console_mode":"handheld","renderer":"vulkan","resolution":"1x",
        "upscaling_filter":"fsr","refresh_rate":"60","frame_gen":false,
        "performance":{"block_list":true,"async_shaders":true,"fast_gpu":false,
        "unsafe_cpu":false,"unsafe_dma":false,"reactive_flushing":true,"skip_invalidation":false}})"
}};
inline constexpr const char* kPaths[] = {
    "/console_mode", "/renderer", "/resolution", "/upscaling_filter", "/refresh_rate", "/frame_gen",
    "/performance/block_list", "/performance/async_shaders", "/performance/fast_gpu",
    "/performance/unsafe_cpu", "/performance/unsafe_dma", "/performance/reactive_flushing",
    "/performance/skip_invalidation"
};
inline const Preset* Find(uint64_t title) {
    for (const auto& preset : kDatabase) if (preset.title == title) return &preset;
    return nullptr;
}
inline bool Read(const std::string& file, Json& document) {
    struct stat st{};
    if (stat(file.c_str(), &st) != 0) {
        if (errno != ENOENT) return false;
        document = Json::object();
        return true;
    }
    std::string text;
    if (!S_ISREG(st.st_mode) || !Settings::ReadFile(file, text)) return false;
    const auto whole = Json::parse(text, nullptr, false);
    if (!whole.is_object()) return false;
    const auto profile = Settings::ProfileAt(whole);
    if (!profile.empty() && whole.contains(Json::json_pointer(profile)) &&
        !whole.at(Json::json_pointer(profile)).is_object()) return false;
    document = Settings::Load(file);
    for (const char* key : {"games", "preset_backups"})
        if (document.contains(key) && !document[key].is_object()) return false;
    return true;
}
inline bool ValidBackup(const Json& backup) {
    if (!backup.is_object() || !backup.contains("fields") || !backup["fields"].is_object() ||
        backup["fields"].size() != std::size(kPaths)) return false;
    for (const char* path : kPaths) {
        if (!backup["fields"].contains(path)) return false;
        const auto& field = backup["fields"][path];
        if (!field.is_object() || !field.contains("present") || !field["present"].is_boolean() ||
            (field["present"].get<bool>() && !field.contains("value"))) return false;
    }
    return true;
}
inline bool CanRestore(uint64_t title, const std::string& file = SettingsFile()) {
    Json document;
    const auto key = Settings::TitleKey(title);
    return title && Read(file, document) && document.contains("preset_backups") &&
        document["preset_backups"].contains(key) && ValidBackup(document["preset_backups"][key]);
}
inline bool Apply(uint64_t title, std::string_view version, const std::string& file = SettingsFile()) {
    const Preset* preset = Find(title);
    if (!preset || version != preset->version) return false;
    Json document;
    if (!Read(file, document)) return false;
    const std::string key = Settings::TitleKey(title);
    auto& game = document["games"][key];
    if (game.is_null()) game = Json::object();
    if (!game.is_object() || (game.contains("performance") && !game["performance"].is_object())) return false;
    auto& backups = document["preset_backups"];
    if (backups.is_null()) backups = Json::object();
    if (backups.contains(key)) {
        if (!ValidBackup(backups[key])) return false;
    } else {
        Json fields = Json::object();
        for (const char* path : kPaths) {
            const Json::json_pointer at(path);
            fields[path] = {{"present", game.contains(at)}};
            if (game.contains(at)) fields[path]["value"] = game.at(at);
        }
        backups[key] = {{"preset", preset->id}, {"fields", std::move(fields)}};
    }
    const Json patch = Json::parse(preset->changes);
    for (const char* path : kPaths) game[Json::json_pointer(path)] = patch.at(Json::json_pointer(path));
    document["version"] = 1;
    return Settings::Write(document, file);
}
inline bool Restore(uint64_t title, const std::string& file = SettingsFile()) {
    Json document;
    if (!Read(file, document)) return false;
    const std::string key = Settings::TitleKey(title);
    if (!document.contains("preset_backups") || !document["preset_backups"].contains(key)) return false;
    const Json backup = document["preset_backups"][key];
    if (!ValidBackup(backup)) return false;
    auto& game = document["games"][key];
    if (game.is_null()) game = Json::object();
    if (!game.is_object() || (game.contains("performance") && !game["performance"].is_object())) return false;
    for (const char* path : kPaths) {
        const auto& field = backup["fields"][path];
        if (field["present"].get<bool>()) game[Json::json_pointer(path)] = field["value"];
        else {
            const std::string text(path);
            const auto slash = text.rfind('/');
            if (slash == 0) game.erase(text.substr(1));
            else if (game.contains("performance")) game["performance"].erase(text.substr(slash + 1));
        }
    }
    if (game.contains("performance") && game["performance"].empty()) game.erase("performance");
    if (game.empty()) document["games"].erase(key);
    document["preset_backups"].erase(key);
    if (document["preset_backups"].empty()) document.erase("preset_backups");
    return Settings::Write(document, file);
}
inline std::string UrlEncode(std::string_view text) {
    static constexpr char hex[] = "0123456789ABCDEF";
    std::string result;
    for (unsigned char c : text) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') result += c;
        else { result += '%'; result += hex[c >> 4]; result += hex[c & 15]; }
    }
    return result;
}
inline std::string ReportUrl(uint64_t title, std::string_view version, const std::string& file = SettingsFile()) {
    if (!title) return {};
    // Snapshot effective settings, never the raw config or private paths/profile/account data.
    const auto prefs = PreferencesFor(title, file);
    const auto speed = LoadPerformance(title, file);
    const Json settings = {
        {"console_mode", LoadGameDocked(title, file) ? "docked" : "handheld"},
        {"renderer", prefs.backend == GraphicsBackend::Vulkan ? "vulkan" : "opengl"},
        {"resolution", kResolutionKeys[prefs.resolution]}, {"upscaling_filter", kUpscalingFilterKeys[prefs.upscaling_filter]},
        {"refresh_rate", kRefreshKeys[prefs.refresh]}, {"frame_gen", prefs.frame_gen},
        {"output_resolution", kOutputKeys[prefs.output]},
        {"detailed_logging", prefs.detailed_logging}, {"immediate_logs", prefs.immediate_logs},
        {"performance", {{"block_list", speed.block_list}, {"async_shaders", speed.async_shaders},
            {"fast_gpu", speed.fast_gpu}, {"unsafe_cpu", speed.unsafe_cpu}, {"unsafe_dma", speed.unsafe_dma},
            {"reactive_flushing", speed.reactive_flushing}, {"skip_invalidation", speed.skip_invalidation}}}
    };
    // Mobile clients may ignore issue-form field IDs; carry the same snapshot in body.
    const std::string body = "### Game\nTitle ID: " + Settings::TitleKey(title) +
        "\nName / region / update: [complete]\n\n### ProsperoEden\n" + std::string(version) +
        "\n\n### PS5\nModel / firmware / enabler / ShadowMountPlus: [complete]" +
        "\n\n### Settings snapshot\nCheck these against what you actually tested.\n```json\n" +
        settings.dump() + "\n```\n\n### Test\nScene / duration: [complete]" +
        "\nResult / native FPS / graphics / audio / restart behavior: [complete]" +
        "\n\n- [ ] I personally tested these settings and reviewed this report.";
    return std::string("https://github.com/") + EDEN_PRESET_REPORT_REPOSITORY + "/issues/new?template=compatibility_settings.yml&title=" +
        UrlEncode("[Settings report] " + Settings::TitleKey(title)) + "&game=" + UrlEncode(Settings::TitleKey(title)) +
        "&eden=" + UrlEncode(version) + "&settings=" + UrlEncode(settings.dump()) + "&body=" + UrlEncode(body);
}

}
