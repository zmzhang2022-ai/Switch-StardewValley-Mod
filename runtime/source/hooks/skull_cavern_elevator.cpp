/* Copyright (C) 2026 zmzhang2022-ai | GPL-2.0-only | https://github.com/zmzhang2022-ai/Switch-StardewValley-Mod */
#include "hooks/skull_cavern_elevator.hpp"

#include "elevator/skull_cavern_elevator.hpp"
#include "game/offsets.hpp"
#include "lib.hpp"

namespace AutomateLite::Hooks {
namespace {

std::uint32_t g_ConstructingSkullMenu{};
std::uint32_t g_HandlingSkullMenuClick{};
void* g_SkullMenu{};
void* g_SkullScrollReceiver{};
void* g_SkullGamePadReceiver{};
std::int32_t g_FirstVisibleFloorIndex{};
std::int32_t g_MaximumFirstFloorIndex{};
std::int32_t g_MenuRowStride{1};
std::uint32_t g_MenuButtonCount{};
bool g_ScrollHookInstalled{};
bool g_GamePadHookInstalled{};

using MenuInputFn = void (*)(void* receiver, std::int32_t value);
MenuInputFn g_OriginalReceiveScrollWheel{};
MenuInputFn g_OriginalReceiveGamePadButton{};

constexpr std::uintptr_t kListItems = 0x10;
constexpr std::uintptr_t kListCount = 0x18;
constexpr std::uintptr_t kArrayValues = 0x10;
constexpr std::uintptr_t kArrayBounds = 0x20;
constexpr std::uint32_t kMaximumReasonableMenuButtons = 128;

// Microsoft.Xna.Framework.Input.Buttons values compiled into the shipped
// MonoGame runtime. Shoulder buttons page the fixed vanilla floor-button
// viewport without stealing D-pad focus navigation from IClickableMenu.
constexpr std::int32_t kLeftShoulder = 0x100;
constexpr std::int32_t kRightShoulder = 0x200;

struct Rectangle final {
    std::int32_t x;
    std::int32_t y;
    std::int32_t width;
    std::int32_t height;
};

using Int32ToStringFn = void* (*)(std::int32_t* value);

void ResetSkullMenuState() noexcept {
    g_SkullMenu = nullptr;
    g_SkullScrollReceiver = nullptr;
    g_SkullGamePadReceiver = nullptr;
    g_FirstVisibleFloorIndex = 0;
    g_MaximumFirstFloorIndex = 0;
    g_MenuRowStride = 1;
    g_MenuButtonCount = 0;
}

bool GetFloorButtons(
    void* menu, void*** buttons, std::uint32_t* count) noexcept {
    if (menu == nullptr || buttons == nullptr || count == nullptr) {
        return false;
    }
    auto* const list = *reinterpret_cast<void**>(
        reinterpret_cast<std::uintptr_t>(menu) +
        StardewValley::Offsets::MineElevatorMenuElevators);
    if (list == nullptr) {
        return false;
    }
    const std::uint32_t size = *reinterpret_cast<std::uint32_t*>(
        reinterpret_cast<std::uintptr_t>(list) + kListCount);
    auto* const array = *reinterpret_cast<void**>(
        reinterpret_cast<std::uintptr_t>(list) + kListItems);
    if (size == 0 || size > kMaximumReasonableMenuButtons || array == nullptr) {
        return false;
    }
    auto* const bounds = *reinterpret_cast<std::uint32_t**>(
        reinterpret_cast<std::uintptr_t>(array) + kArrayBounds);
    auto** const values = *reinterpret_cast<void***>(
        reinterpret_cast<std::uintptr_t>(array) + kArrayValues);
    if (bounds == nullptr || values == nullptr || *bounds < size) {
        return false;
    }
    *buttons = values;
    *count = size;
    return true;
}

std::int32_t DetectMenuRowStride(
    void** buttons, std::uint32_t count) noexcept {
    if (buttons == nullptr || count < 2 || buttons[0] == nullptr) {
        return 1;
    }
    const auto* const first = reinterpret_cast<const Rectangle*>(
        reinterpret_cast<std::uintptr_t>(buttons[0]) +
        StardewValley::Offsets::ClickableComponentBounds);
    std::int32_t stride = 1;
    for (std::uint32_t index = 1; index < count; ++index) {
        if (buttons[index] == nullptr) {
            break;
        }
        const auto* const bounds = reinterpret_cast<const Rectangle*>(
            reinterpret_cast<std::uintptr_t>(buttons[index]) +
            StardewValley::Offsets::ClickableComponentBounds);
        if (bounds->y != first->y) {
            break;
        }
        ++stride;
    }
    return stride;
}

bool ApplyFloorWindow(void* menu) noexcept {
    void** buttons = nullptr;
    std::uint32_t count = 0;
    if (!GetFloorButtons(menu, &buttons, &count)) {
        return false;
    }

    const std::int32_t deepest =
        Elevator::SkullCavernElevator::Instance().GetDeepestMineLevel();
    const std::int32_t relative =
        deepest > Elevator::SkullCavernElevator::BaseMineLevel
            ? deepest - Elevator::SkullCavernElevator::BaseMineLevel
            : 0;
    const std::int32_t maximumFloorIndex =
        relative / Elevator::SkullCavernElevator::ElevatorStep;
    g_MaximumFirstFloorIndex = maximumFloorIndex >=
            static_cast<std::int32_t>(count - 1)
        ? maximumFloorIndex - static_cast<std::int32_t>(count - 1)
        : 0;
    if (g_FirstVisibleFloorIndex < 0) {
        g_FirstVisibleFloorIndex = 0;
    }
    if (g_FirstVisibleFloorIndex > g_MaximumFirstFloorIndex) {
        g_FirstVisibleFloorIndex = g_MaximumFirstFloorIndex;
    }
    g_MenuRowStride = DetectMenuRowStride(buttons, count);
    g_MenuButtonCount = count;

    const auto toString = reinterpret_cast<Int32ToStringFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::SystemInt32ToString));
    for (std::uint32_t index = 0; index < count; ++index) {
        auto* const button = buttons[index];
        if (button == nullptr) {
            continue;
        }
        std::int32_t relativeLevel =
            (g_FirstVisibleFloorIndex + static_cast<std::int32_t>(index)) *
            Elevator::SkullCavernElevator::ElevatorStep;
        void* const name = toString(&relativeLevel);
        if (name != nullptr) {
            *reinterpret_cast<void**>(
                reinterpret_cast<std::uintptr_t>(button) +
                StardewValley::Offsets::ClickableComponentName) = name;
        }
    }
    return true;
}

bool ScrollFloorWindow(void* menu, bool down) noexcept {
    if (menu == nullptr || menu != g_SkullMenu ||
        g_MaximumFirstFloorIndex <= 0) {
        return false;
    }
    const std::int32_t oldIndex = g_FirstVisibleFloorIndex;
    if (down) {
        g_FirstVisibleFloorIndex += g_MenuRowStride;
        if (g_FirstVisibleFloorIndex > g_MaximumFirstFloorIndex) {
            g_FirstVisibleFloorIndex = g_MaximumFirstFloorIndex;
        }
    } else {
        g_FirstVisibleFloorIndex -= g_MenuRowStride;
        if (g_FirstVisibleFloorIndex < 0) {
            g_FirstVisibleFloorIndex = 0;
        }
    }
    if (g_FirstVisibleFloorIndex == oldIndex) {
        return false;
    }
    if (!ApplyFloorWindow(menu)) {
        g_FirstVisibleFloorIndex = oldIndex;
        return false;
    }
    Logging.Log(
        "[SkullElevator] menu scroll first=%d last=%d stride=%d",
        g_FirstVisibleFloorIndex *
            Elevator::SkullCavernElevator::ElevatorStep,
        (g_FirstVisibleFloorIndex +
         static_cast<std::int32_t>(g_MenuButtonCount - 1)) *
            Elevator::SkullCavernElevator::ElevatorStep,
        g_MenuRowStride);
    return true;
}

void InstallLiveMenuInputHooks(void* menu) noexcept;
void ReceiveScrollWheelCallback(
    void* receiver, std::int32_t delta) noexcept;
void ReceiveGamePadButtonCallback(
    void* receiver, std::int32_t button) noexcept;

class ScopedCounter final {
public:
    explicit ScopedCounter(std::uint32_t& value) noexcept : m_Value(value) {
        ++m_Value;
    }
    ~ScopedCounter() {
        --m_Value;
    }

    ScopedCounter(const ScopedCounter&) = delete;
    ScopedCounter& operator=(const ScopedCounter&) = delete;

private:
    std::uint32_t& m_Value;
};

} // namespace

HOOK_DEFINE_TRAMPOLINE(MineShaftLowestLevelReachedHook) {
    static std::int32_t Callback() {
        const std::int32_t actual = Orig();
        if (g_ConstructingSkullMenu == 0) {
            return actual;
        }
        if (actual <= Elevator::SkullCavernElevator::BaseMineLevel) {
            return 0;
        }
        const std::int32_t relative =
            actual - Elevator::SkullCavernElevator::BaseMineLevel;
        // The original constructor still applies min(value, 120), intentionally
        // producing a bounded native button viewport. The scroll hooks rewrite
        // that viewport's names/targets instead of overflowing the screen.
        return relative;
    }
};

HOOK_DEFINE_TRAMPOLINE(MineElevatorMenuConstructorHook) {
    static void* Callback(void* menu) {
        const bool skull =
            Elevator::SkullCavernElevator::Instance().IsElevatorContext();
        if (!skull) {
            void* const result = Orig(menu);
            ResetSkullMenuState();
            return result;
        }

        void* result = nullptr;
        {
            ScopedCounter scope(g_ConstructingSkullMenu);
            result = Orig(menu);
        }
        g_SkullMenu = result;
        g_FirstVisibleFloorIndex = 0;
        g_MaximumFirstFloorIndex = 0;
        g_MenuRowStride = 1;
        g_MenuButtonCount = 0;
        ApplyFloorWindow(result);
        InstallLiveMenuInputHooks(result);
        const std::int32_t deepest =
            Elevator::SkullCavernElevator::Instance().GetDeepestMineLevel();
        Logging.Log(
            "[SkullElevator] menu opened deepest=%d relative=%d "
            "maxFirst=%d rowStride=%d",
            deepest,
            deepest > Elevator::SkullCavernElevator::BaseMineLevel
                ? deepest - Elevator::SkullCavernElevator::BaseMineLevel
                : 0,
            g_MaximumFirstFloorIndex *
                Elevator::SkullCavernElevator::ElevatorStep,
            g_MenuRowStride);
        return result;
    }
};

HOOK_DEFINE_TRAMPOLINE(Game1ExitActiveMenuHook) {
    static void Callback() {
        Orig();
        ResetSkullMenuState();
    }
};

namespace {

void ReceiveScrollWheelCallback(
    void* receiver, std::int32_t delta) noexcept {
    if (receiver == g_SkullScrollReceiver && g_SkullMenu != nullptr) {
        if (delta < 0 && ScrollFloorWindow(g_SkullMenu, true)) {
            return;
        }
        if (delta > 0 && ScrollFloorWindow(g_SkullMenu, false)) {
            return;
        }
    }
    if (g_OriginalReceiveScrollWheel != nullptr) {
        g_OriginalReceiveScrollWheel(receiver, delta);
    }
}

void ReceiveGamePadButtonCallback(
    void* receiver, std::int32_t button) noexcept {
    if (receiver == g_SkullGamePadReceiver && g_SkullMenu != nullptr) {
        if (button == kLeftShoulder) {
            ScrollFloorWindow(g_SkullMenu, false);
            return;
        }
        if (button == kRightShoulder) {
            ScrollFloorWindow(g_SkullMenu, true);
            return;
        }
    }
    if (g_OriginalReceiveGamePadButton != nullptr) {
        g_OriginalReceiveGamePadButton(receiver, button);
    }
}

bool IsMainTextPointer(std::uintptr_t pointer) noexcept {
    const std::uintptr_t start = exl::util::modules::GetTargetStart();
    return pointer >= start &&
           pointer - start < StardewValley::Offsets::MainTextSize;
}

bool ReplaceMethodPointer(
    std::uintptr_t slot, std::uintptr_t expected,
    std::uintptr_t replacement) noexcept {
    if (slot == 0 || expected == 0 || replacement == 0 ||
        *reinterpret_cast<std::uintptr_t*>(slot) != expected) {
        return false;
    }
    exl::util::RwPages writable(slot, sizeof(std::uintptr_t));
    *reinterpret_cast<std::uintptr_t*>(writable.GetRw()) = replacement;
    writable.Flush();
    return *reinterpret_cast<std::uintptr_t*>(slot) == replacement;
}

void InstallLiveMenuInputHooks(void* menu) noexcept {
    if (menu == nullptr) {
        return;
    }
    auto* const objectTable = *reinterpret_cast<void**>(menu);
    if (objectTable == nullptr) {
        return;
    }
    auto* const methods = *reinterpret_cast<void**>(
        reinterpret_cast<std::uintptr_t>(objectTable) + 8);
    if (methods == nullptr) {
        return;
    }

    const auto scrollTarget = *reinterpret_cast<std::uintptr_t*>(
        reinterpret_cast<std::uintptr_t>(methods) +
        StardewValley::Offsets::MenuReceiveScrollWheelSlot);
    const auto scrollAdjust = *reinterpret_cast<std::uint8_t*>(
        reinterpret_cast<std::uintptr_t>(methods) +
        StardewValley::Offsets::MenuReceiveScrollWheelAdjust);
    g_SkullScrollReceiver = reinterpret_cast<void*>(
        reinterpret_cast<std::uintptr_t>(menu) + scrollAdjust);
    if (!g_ScrollHookInstalled && IsMainTextPointer(scrollTarget)) {
        const std::uintptr_t slot =
            reinterpret_cast<std::uintptr_t>(methods) +
            StardewValley::Offsets::MenuReceiveScrollWheelSlot;
        g_OriginalReceiveScrollWheel =
            reinterpret_cast<MenuInputFn>(scrollTarget);
        if (ReplaceMethodPointer(
                slot, scrollTarget,
                reinterpret_cast<std::uintptr_t>(
                    &ReceiveScrollWheelCallback))) {
            g_ScrollHookInstalled = true;
            Logging.Log(
                "[SkullElevator] menu scroll slot patched, "
                "original=main+0x%llX adjust=%u",
                static_cast<unsigned long long>(
                    scrollTarget - exl::util::modules::GetTargetStart()),
                static_cast<unsigned>(scrollAdjust));
        } else {
            g_OriginalReceiveScrollWheel = nullptr;
        }
    }

    const auto gamePadTarget = *reinterpret_cast<std::uintptr_t*>(
        reinterpret_cast<std::uintptr_t>(methods) +
        StardewValley::Offsets::MenuReceiveGamePadButtonSlot);
    const auto gamePadAdjust = *reinterpret_cast<std::uint8_t*>(
        reinterpret_cast<std::uintptr_t>(methods) +
        StardewValley::Offsets::MenuReceiveGamePadButtonAdjust);
    g_SkullGamePadReceiver = reinterpret_cast<void*>(
        reinterpret_cast<std::uintptr_t>(menu) + gamePadAdjust);
    if (!g_GamePadHookInstalled && IsMainTextPointer(gamePadTarget)) {
        const std::uintptr_t slot =
            reinterpret_cast<std::uintptr_t>(methods) +
            StardewValley::Offsets::MenuReceiveGamePadButtonSlot;
        g_OriginalReceiveGamePadButton =
            reinterpret_cast<MenuInputFn>(gamePadTarget);
        if (ReplaceMethodPointer(
                slot, gamePadTarget,
                reinterpret_cast<std::uintptr_t>(
                    &ReceiveGamePadButtonCallback))) {
            g_GamePadHookInstalled = true;
            Logging.Log(
                "[SkullElevator] menu gamepad slot patched, "
                "original=main+0x%llX adjust=%u",
                static_cast<unsigned long long>(
                    gamePadTarget - exl::util::modules::GetTargetStart()),
                static_cast<unsigned>(gamePadAdjust));
        } else {
            g_OriginalReceiveGamePadButton = nullptr;
        }
    }
}

} // namespace

HOOK_DEFINE_TRAMPOLINE(Game1EnterMineHook) {
    static void Callback(std::int32_t targetLevel, void* forceDescending) {
        if (g_HandlingSkullMenuClick != 0 && targetLevel > 0) {
            const std::int32_t relative = targetLevel;
            targetLevel += Elevator::SkullCavernElevator::BaseMineLevel;
            Logging.Log(
                "[SkullElevator] travel relative=%d target=%d",
                relative, targetLevel);
        }
        Orig(targetLevel, forceDescending);
    }
};

HOOK_DEFINE_TRAMPOLINE(Game1WarpFarmerHook) {
    static void Callback(
        void* locationName, std::int32_t x, std::int32_t y,
        std::int32_t facingDirection, bool isStructure) {
        if (g_HandlingSkullMenuClick != 0) {
            void* const skullCave =
                Elevator::SkullCavernElevator::Instance()
                    .GetSkullCaveString();
            if (skullCave != nullptr) {
                locationName = skullCave;
                x = 3;
                y = 4;
                Logging.Log("[SkullElevator] return to SkullCave (3,4)");
            }
        }
        Orig(locationName, x, y, facingDirection, isStructure);
    }
};

HOOK_DEFINE_TRAMPOLINE(MineElevatorMenuReceiveLeftClickHook) {
    static void Callback(
        void* menu, std::int32_t x, std::int32_t y, bool playSound) {
        if (menu == nullptr || menu != g_SkullMenu) {
            Orig(menu, x, y, playSound);
            return;
        }

        // Match the reference menu's touchable up/down control strip. The
        // floor buttons and their drawing remain the vanilla implementation;
        // this narrow strip only changes the visible button window.
        const auto* const menuBounds = reinterpret_cast<const Rectangle*>(
            reinterpret_cast<std::uintptr_t>(menu) + 0x28);
        const std::int32_t stripLeft =
            menuBounds->x + menuBounds->width + 8;
        if (x >= stripLeft && x < stripLeft + 64 &&
            y >= menuBounds->y && y < menuBounds->y + menuBounds->height) {
            const bool down = y >= menuBounds->y + menuBounds->height / 2;
            if (ScrollFloorWindow(menu, down)) {
                return;
            }
        }
        ScopedCounter scope(g_HandlingSkullMenuClick);
        Orig(menu, x, y, playSound);
    }
};

void InstallSkullCavernElevatorHooks() {
    MineShaftLowestLevelReachedHook::InstallAtOffset(
        StardewValley::Offsets::MineShaftLowestLevelReached);
    MineElevatorMenuConstructorHook::InstallAtOffset(
        StardewValley::Offsets::MineElevatorMenuConstructor);
    MineElevatorMenuReceiveLeftClickHook::InstallAtOffset(
        StardewValley::Offsets::MineElevatorMenuReceiveLeftClick);
    Game1EnterMineHook::InstallAtOffset(
        StardewValley::Offsets::Game1EnterMine);
    Game1WarpFarmerHook::InstallAtOffset(
        StardewValley::Offsets::Game1WarpFarmer);
    Game1ExitActiveMenuHook::InstallAtOffset(
        StardewValley::Offsets::Game1ExitActiveMenu);
    Logging.Log(
        "[SkullElevator] hooks installed for Build ID "
        "A5C617C14A7F3F6620B3BC8136965A4822D32B9C");
}

} // namespace AutomateLite::Hooks
