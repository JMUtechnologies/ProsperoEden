#!/usr/bin/env bash
# ProsperoEden - Build an isolated PS5 settings-preset check; does not deploy.
# Copyright (C) 2026 BlackBearReloaded
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$root"
: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to the official PS5 Payload SDK folder}"
: "${NLOHMANN_INCLUDE:?Set NLOHMANN_INCLUDE to the include root containing nlohmann/json.hpp}"
mkdir -p build/preset-console
"$PS5_PAYLOAD_SDK/bin/prospero-clang++" -std=c++20 -O1 -g -Wall -Wextra -Werror \
    -Iheadless -I"$NLOHMANN_INCLUDE" headless/game_presets_console_check.cpp \
    -o build/preset-console/preset-test.elf
