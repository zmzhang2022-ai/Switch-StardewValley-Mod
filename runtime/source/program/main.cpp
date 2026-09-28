/*
 * Stardew Valley Switch Native Mod / Automate Lite
 * Copyright (C) 2026 zmzhang2022-ai
 * SPDX-License-Identifier: GPL-2.0-only
 * Official repository: https://github.com/zmzhang2022-ai/Switch-StardewValley-Mod
 */
#include "lib.hpp"
#include "project_metadata.hpp"

#include "hooks/game_update.hpp"
#include "hooks/skull_cavern_elevator.hpp"
#include "hooks/wear_more_rings.hpp"
#include "uiinfo/ui_info_suite.hpp"
#include "fishing/fishing.hpp"

extern "C" void exl_main(void*, void*) {
    exl::hook::Initialize();

    Logging.Log(
        "[AutomateLite] %s | %s | Build ID %s | %s | %s",
        AutomateLite::Project::kVersion,
        AutomateLite::Project::kGameVersion,
        AutomateLite::Project::kBuildId,
        AutomateLite::Project::kCopyright,
        AutomateLite::Project::kRepository);

    AutomateLite::Fishing::Install();
    AutomateLite::Hooks::InstallGameUpdateHook();
    AutomateLite::Hooks::InstallPhase8TraceHook();
    AutomateLite::Hooks::InstallSkullCavernElevatorHooks();
    AutomateLite::Hooks::InstallWearMoreRingsHooks();
    AutomateLite::UIInfo::Install();
    Logging.Log("[AutomateLite] MonoGame Game.Tick hook installed at main+0x7F890");
}

extern "C" NORETURN void exl_exception_entry() {
    EXL_ABORT("Automate Lite exception handler called");
}
