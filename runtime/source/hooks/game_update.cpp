/* Copyright (C) 2026 zmzhang2022-ai | GPL-2.0-only | https://github.com/zmzhang2022-ai/Switch-StardewValley-Mod */
#include "game_update.hpp"

#include "automate/automate_manager.hpp"
#include "elevator/skull_cavern_elevator.hpp"
#include "game/offsets.hpp"
#include "game/runtime.hpp"
#include "lib.hpp"
#include "fishing/fishing.hpp"
#include "fastanimations/fastanimations.hpp"

namespace AutomateLite::Hooks {

HOOK_DEFINE_TRAMPOLINE(GameTickHook) {
    static void Callback(void* game) {
        Fishing::BeforeTick();
        Orig(game);
        FastAnimations::Update();
        Fishing::Update();
        Automate::AutomateManager::Instance().Update();
        Elevator::SkullCavernElevator::Instance().Update();
    }
};

#if defined(AUTOMATE_LITE_PHASE8_TRACE) && defined(AUTOMATE_LITE_ENABLE_UNSAFE_PLAYER_COLLECTION)

// This probe remains opt-in together with the unsafe action call. It must not
// be installed in normal or trace-only builds until the managed invoker ABI is
// confirmed on the exact hardware/build.
HOOK_DEFINE_TRAMPOLINE(ObjectCheckForActionTraceHook) {
    static bool Callback(void* object, const Game::CheckForActionArgs* args) {
        const bool justChecking =
            args != nullptr && args->justChecking != nullptr && *args->justChecking != 0;
        Game::ObjectOutputState before{};
        if (!justChecking) {
            before = Game::ObjectView(static_cast<Game::Object*>(object)).ReadOutputState();
            Logging.Log(
                "[AutomateLite] phase8 trace: checkForAction enter "
                "machine=%llX who=%llX held=%llX minutes=%d",
                static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(object)),
                static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(
                    args == nullptr ? nullptr : args->who)),
                static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(before.heldObject)),
                before.minutesUntilReady);
        }

        const bool result = Orig(object, args);

        if (!justChecking) {
            const auto after =
                Game::ObjectView(static_cast<Game::Object*>(object)).ReadOutputState();
            Logging.Log(
                "[AutomateLite] phase8 trace: checkForAction exit result=%u "
                "held=%llX->%llX minutes=%d->%d",
                result ? 1U : 0U,
                static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(before.heldObject)),
                static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(after.heldObject)),
                before.minutesUntilReady,
                after.minutesUntilReady);
        }
        return result;
    }
};

#endif

void InstallGameUpdateHook() {
    GameTickHook::InstallAtOffset(StardewValley::Offsets::MonoGameTick);
}

void InstallPhase8TraceHook() {
#if defined(AUTOMATE_LITE_PHASE8_TRACE) && defined(AUTOMATE_LITE_ENABLE_UNSAFE_PLAYER_COLLECTION)
    ObjectCheckForActionTraceHook::InstallAtOffset(
        StardewValley::Offsets::ObjectCheckForActionInvoker);
    Logging.Log(
        "[AutomateLite] phase8 trace hook installed at main+0x74BEBC0 "
        "(explicit unsafe ABI probe)");
#else
    Logging.Log(
        "[AutomateLite] checkForAction trace disabled; managed invoker ABI is not "
        "hardware-verified");
#endif
}

} // namespace AutomateLite::Hooks
