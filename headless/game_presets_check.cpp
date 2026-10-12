// ProsperoEden - Preset preservation, profile isolation, refusal and report checks.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later
#include "game_presets.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <unistd.h>
using Json = Eden::Settings::Json;
constexpr uint64_t title = 0x0100965017338000ULL;
static std::string Read(const std::string& file) {
    std::ifstream in(file); return {std::istreambuf_iterator<char>(in), {}};
}
int main() {
    char temp[] = "/tmp/eden-preset-XXXXXX"; assert(mkdtemp(temp));
    const std::string file = std::string(temp) + "/settings.json";
    const auto key = Eden::Settings::TitleKey(title);
    Json original = {{"video", {{"renderer", "opengl"}}},
        {"diagnostics", {{"detailed_logging", true}}}, {"game_files", "/private/path"},
        {"games", {{key, {{"resolution", "0.75x"}, {"volume", 40}, {"language", "ja"},
            {"mapping", {{"a", "square"}}}, {"mods_off", {"one"}},
            {"performance", {{"fast_gpu", true}, {"unknown_future", 42}}}}},
            {"0100000000000000", {{"resolution", "4x"}}}}}};
    assert(Eden::Settings::WriteWhole(original, file));
    assert(Eden::Presets::Apply(title, "01.000.110", file));
    assert(Eden::LoadPerformance(title, file).async_shaders);
    assert(!Eden::LoadPerformance(title, file).fast_gpu);
    auto applied = Eden::Settings::Load(file);
    assert(applied["games"][key]["volume"] == 40 && applied["games"][key]["mapping"] == original["games"][key]["mapping"]);
    assert(applied["diagnostics"] == original["diagnostics"] && applied["video"] == original["video"]);
    assert(Eden::Presets::CanRestore(title, file));
    const auto backup = applied["preset_backups"];
    assert(Eden::Presets::Apply(title, "01.000.110", file));
    assert(Eden::Settings::Load(file)["preset_backups"] == backup);
    // Later unrelated settings remain; preset-owned settings revert to their prior value/inheritance.
    applied = Eden::Settings::Load(file); applied["games"][key]["volume"] = 60;
    assert(Eden::Settings::Write(applied, file));
    assert(Eden::Presets::Restore(title, file));
    original["games"][key]["volume"] = 60;
    auto restored = Eden::Settings::Load(file); restored.erase("version");
    assert(restored == original && !Eden::Presets::CanRestore(title, file));
    const auto bytes = Read(file);
    assert(!Eden::Presets::Apply(title, "01.000.120", file));
    assert(!Eden::Presets::Apply(123, "01.000.110", file));
    assert(!Eden::Presets::Restore(title, file)); assert(Read(file) == bytes);
    // Two profiles own separate snapshots and per-game settings.
    const std::string first(32, 'a'), second(32, 'b');
    original["profiles"] = {{"first", first}, {"current", second}};
    original["profile_settings"][second] = {{"games", {{key, {{"resolution", "2x"}}}}}};
    assert(Eden::Settings::WriteWhole(original, file));
    assert(Eden::Presets::Apply(title, "01.000.110", file));
    auto whole = Eden::Settings::LoadWhole(file);
    assert(whole["games"] == original["games"] && !whole.contains("preset_backups"));
    assert(whole["profile_settings"][second].contains("preset_backups"));
    assert(Eden::Presets::Restore(title, file));
    whole = Eden::Settings::LoadWhole(file);
    assert(whole["profile_settings"][second] == original["profile_settings"][second]);
    // Unknown titles still have a report, and no private paths or profile data enter it.
    const auto url = Eden::Presets::ReportUrl(title, "01.000.110", file);
    assert(url.find("https://github.com/blackbearreloaded/ProsperoEden/issues/new?") == 0);
    assert(url.find("template=compatibility_settings.yml") != std::string::npos);
    assert(url.find("&body=" + Eden::Presets::UrlEncode("### Game\nTitle ID: " + key)) != std::string::npos);
    assert(url.find(Eden::Presets::UrlEncode("- [ ] I personally tested these settings")) != std::string::npos);
    assert(url.find("private") == std::string::npos && url.find(first) == std::string::npos);
    assert(Eden::Presets::UrlEncode("A&B\n") == "A%26B%0A");
    assert(!Eden::Presets::ReportUrl(0, "01.000.110", file).size());
    // Malformed input cannot erase the user's settings or undo snapshot.
    for (const auto& bad : {std::string("{broken"), std::string("{\"games\":[]}"),
        std::string("{\"games\":{\"") + key + "\":{\"performance\":false}}}",
        std::string("{\"preset_backups\":{\"") + key + "\":{\"fields\":{}}}}"}) {
        { std::ofstream out(file); out << bad; }
        assert(!Eden::Presets::Apply(title, "01.000.110", file));
        assert(!Eden::Presets::Restore(title, file)); assert(Read(file) == bad);
    }
    // Atomic-write failure leaves the existing file untouched.
    assert(Eden::Settings::WriteWhole(Json::object(), file));
    std::filesystem::create_directory(file + ".tmp");
    const auto before = Read(file);
    assert(!Eden::Presets::Apply(title, "01.000.110", file)); assert(Read(file) == before);
    std::filesystem::remove_all(file + ".tmp");
    assert(Eden::Presets::Apply(title, "01.000.110", file));
    assert(Eden::Presets::Restore(title, file));
    assert(!Eden::Settings::Load(file).contains("games") || Eden::Settings::Load(file)["games"].empty());
    std::filesystem::remove_all(temp);
    std::puts("Preset apply/restore, refusals, profile isolation, atomic failure and redacted report PASS");
}
