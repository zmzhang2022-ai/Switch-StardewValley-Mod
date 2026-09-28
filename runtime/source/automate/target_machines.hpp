#pragma once

#include <cstdint>

#include "game/runtime.hpp"

namespace AutomateLite::Automate {

// The Switch port is intentionally a whitelist, not a generic "all machines"
// scan. These are the legacy numeric ParentSheetIndex values used by the
// 1.6.15.3 content data. The three machines introduced with named 1.6 data
// IDs are classified through Item.getItemId; no numeric ID is guessed for
// them. Input is still gated separately by the recipe-capable whitelist in
// automate_manager.cpp, while output collection applies to every entry here.
enum class TargetMachine : std::uint8_t {
    None,
    CheesePress,
    RecyclingMachine,
    SolarPanel,
    StatueOfPerfection,
    Crystalarium,
    Keg,
    StatueOfEndlessFortune,
    Cask,
    Tapper,
    Furnace,
    StatueOfTruePerfection,
    SeedMaker,
    MayonnaiseMachine,
    LightningRod,
    HeavyTapper,
    Dehydrator,
    FishSmoker,
    HeavyFurnace,
    PreservesJar,
    GeodeCrusher,
    Count,
};

TargetMachine ClassifyTargetMachine(Game::Object* object) noexcept;
const char* TargetMachineName(TargetMachine machine) noexcept;

} // namespace AutomateLite::Automate
