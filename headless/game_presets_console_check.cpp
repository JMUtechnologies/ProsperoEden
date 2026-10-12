// ProsperoEden - Isolated PS5 preset preservation and recovery checks.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later
#include "game_presets.h"
#include <cstdio>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
using Json = Eden::Settings::Json;
const std::string root = "/data/prosperoeden/.preset-test-22b625d-20261011";
const std::string file = root + "/settings.json";
constexpr uint64_t title = 0x0100965017338000ULL;
void require(bool ok, const char *text)
{
    if (!ok)
        throw std::runtime_error(text);
}
std::string read()
{
    std::string s;
    require(Eden::Settings::ReadFile(file, s), "read fixture");
    return s;
}
int main()
{
    struct stat st{};
    if (stat(root.c_str(), &st) == 0)
        return 2;
    if (mkdir(root.c_str(), 0777) != 0)
        return 3;
    unsigned passed = 0;
    try
    {
        const auto key = Eden::Settings::TitleKey(title);
        Json original = {{"video", {{"renderer", "opengl"}}},
                         {"diagnostics", {{"detailed_logging", true}}},
                         {"games",
                          {{key,
                            {{"volume", 40},
                             {"resolution", "0.75x"},
                             {"rng_seed", 1234},
                             {"mapping", {{"a", "square"}}},
                             {"mods_off", {"one"}},
                             {"performance", {{"fast_gpu", true}, {"future", 42}}}}}}}};
        require(Eden::Settings::WriteWhole(original, file), "seed");
        require(Eden::Presets::Apply(title, "01.000.110", file), "apply");
        require(Eden::LoadPerformance(title, file).async_shaders &&
                    !Eden::LoadPerformance(title, file).fast_gpu,
                "settings applied");
        auto after = Eden::Settings::Load(file);
        require(after["games"][key]["mapping"] == original["games"][key]["mapping"] &&
                    after["games"][key]["rng_seed"] == 1234,
                "unrelated fields");
        require(after["video"] == original["video"] &&
                    after["diagnostics"] == original["diagnostics"],
                "globals preserved");
        ++passed;
        const auto snapshot = after["preset_backups"];
        require(Eden::Presets::Apply(title, "01.000.110", file), "reapply");
        require(Eden::Settings::Load(file)["preset_backups"] == snapshot,
                "original snapshot retained");
        ++passed;
        after = Eden::Settings::Load(file);
        after["games"][key]["volume"] = 60;
        require(Eden::Settings::Write(after, file), "later edit");
        require(Eden::Presets::Restore(title, file), "restore");
        original["games"][key]["volume"] = 60;
        after = Eden::Settings::Load(file);
        after.erase("version");
        require(after == original, "restore preserves unrelated edits and inheritance");
        ++passed;
        auto bytes = read();
        require(!Eden::Presets::Apply(title, "01.000.120", file) &&
                    !Eden::Presets::Apply(123, "01.000.110", file) &&
                    !Eden::Presets::Restore(title, file),
                "unknown version/title/undo refusal");
        require(bytes == read(), "refusal changes no bytes");
        ++passed;
        const std::string first(32, 'a'), second(32, 'b');
        original["profiles"] = {{"first", first}, {"current", second}};
        original["profile_settings"][second] = {{"games", {{key, {{"resolution", "2x"}}}}}};
        require(Eden::Settings::WriteWhole(original, file), "profile seed");
        require(Eden::Presets::Apply(title, "01.000.110", file), "profile apply");
        auto whole = Eden::Settings::LoadWhole(file);
        require(whole["games"] == original["games"] && !whole.contains("preset_backups"),
                "first profile isolated");
        require(Eden::Presets::Restore(title, file), "profile restore");
        require(Eden::Settings::LoadWhole(file)["profile_settings"][second] ==
                    original["profile_settings"][second],
                "profile round trip");
        ++passed;
        const auto url = Eden::Presets::ReportUrl(title, "01.000.110", file);
        require(url.find("template=compatibility_settings.yml") != url.npos &&
                    url.find(second) == url.npos && url.find("mapping") == url.npos,
                "allowlisted report URL");
        ++passed;
        require(Eden::Settings::WriteFile(file, "{broken"), "malformed fixture");
        bytes = read();
        require(!Eden::Presets::Apply(title, "01.000.110", file) &&
                    !Eden::Presets::Restore(title, file) && read() == bytes,
                "malformed refusal");
        ++passed;
        require(Eden::Settings::WriteWhole(Json::object(), file), "write-failure seed");
        require(mkdir((file + ".tmp").c_str(), 0777) == 0, "write-failure setup");
        bytes = read();
        require(!Eden::Presets::Apply(title, "01.000.110", file) && read() == bytes,
                "atomic failure preservation");
        require(rmdir((file + ".tmp").c_str()) == 0, "clear fixture obstacle");
        ++passed;
        require(Eden::Presets::Apply(title, "01.000.110", file) &&
                    Eden::Presets::Restore(title, file),
                "missing per-title settings round trip");
        require(!Eden::Settings::Load(file).contains("games") ||
                    Eden::Settings::Load(file)["games"].empty(),
                "missing fields restored");
        ++passed;
        require(Eden::Settings::WriteFile(
                    root + "/result.txt",
                    "PASS: " + std::to_string(passed) +
                        " isolated PS5 preset scenarios.\nProduction branch commit: 22b625d\nReal "
                        "configuration, saves and game files were not accessed by this test.\n"),
                "result");
        return 0;
    }
    catch (const std::exception &e)
    {
        Eden::Settings::WriteFile(root + "/result.txt", "FAIL after " + std::to_string(passed) +
                                                            " scenarios: " + e.what() + "\n");
        return 1;
    }
}
