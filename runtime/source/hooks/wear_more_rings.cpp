/* Copyright (C) 2026 zmzhang2022-ai | GPL-2.0-only | https://github.com/zmzhang2022-ai/Switch-StardewValley-Mod */
#include "hooks/wear_more_rings.hpp"

#include "game/offsets.hpp"
#include "lib.hpp"
#include "rings/wear_more_rings.hpp"
#include "uiinfo/ui_info_suite.hpp"

namespace AutomateLite::Hooks {
namespace {

HOOK_DEFINE_TRAMPOLINE(InventoryPageConstructorHook) {
    static void* Callback(
        void* page, std::int32_t x, std::int32_t y,
        std::int32_t width, std::int32_t height) {
        void* const result = Orig(page, x, y, width, height);
        if (!Rings::WearMoreRings::AttachInventoryPage(result)) {
            Logging.Log("[WearMoreRings] ERROR: InventoryPage attach failed");
        }
        return result;
    }
};

HOOK_DEFINE_TRAMPOLINE(InventoryPageReceiveLeftClickHook) {
    static void Callback(
        void* page, std::int32_t x, std::int32_t y, bool playSound) {
        if (Rings::WearMoreRings::HandleClick(page, x, y)) {
            return;
        }
        Orig(page, x, y, playSound);
    }
};

HOOK_DEFINE_TRAMPOLINE(InventoryPagePerformHoverActionHook) {
    static void Callback(void* page, std::int32_t x, std::int32_t y) {
        const bool attached =
            Rings::WearMoreRings::IsAttachedInventoryPage(page);
        if (attached) {
            Rings::WearMoreRings::SetNamesHidden(true);
        }
        Orig(page, x, y);
        if (attached) {
            Rings::WearMoreRings::SetNamesHidden(false);
            Rings::WearMoreRings::HandleHover(page, x, y);
        }
    }
};

HOOK_DEFINE_TRAMPOLINE(InventoryPageDrawHook) {
    static void Callback(void* page, void* spriteBatch) {
        const bool attached =
            Rings::WearMoreRings::IsAttachedInventoryPage(page);
        if (attached) {
            Rings::WearMoreRings::SynchronizeItems();
            Rings::WearMoreRings::SetNamesHidden(true);
        }
        Orig(page, spriteBatch);
        if (attached) {
            Rings::WearMoreRings::SetNamesHidden(false);
            Rings::WearMoreRings::Draw(page, spriteBatch);
        }
    }
};

} // namespace

void InstallWearMoreRingsHooks() {
    InventoryPageConstructorHook::InstallAtOffset(
        StardewValley::Offsets::InventoryPageConstructor);
    InventoryPageReceiveLeftClickHook::InstallAtOffset(
        StardewValley::Offsets::InventoryPageReceiveLeftClick);
    InventoryPagePerformHoverActionHook::InstallAtOffset(
        StardewValley::Offsets::InventoryPagePerformHoverAction);
    InventoryPageDrawHook::InstallAtOffset(
        StardewValley::Offsets::InventoryPageDraw);
    Logging.Log(
        "[WearMoreRings] fixed four-slot hooks installed for Build ID "
        "A5C617C14A7F3F6620B3BC8136965A4822D32B9C");
}

} // namespace AutomateLite::Hooks
