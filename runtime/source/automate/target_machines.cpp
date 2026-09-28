/* Copyright (C) 2026 zmzhang2022-ai | GPL-2.0-only | https://github.com/zmzhang2022-ai/Switch-StardewValley-Mod */
#include "target_machines.hpp"

namespace AutomateLite::Automate {
namespace {

struct IdEntry final {
    std::int32_t parentSheetIndex;
    TargetMachine machine;
};

// Vanilla 1.6 big-craftable IDs. Keep this list local and explicit so an
// unrelated machine can never enter the future Chest transaction by accident.
constexpr IdEntry kTargetIds[] = {
    {9,   TargetMachine::LightningRod},
    {12,  TargetMachine::Keg},
    {13,  TargetMachine::Furnace},
    {15,  TargetMachine::PreservesJar},
    {16,  TargetMachine::CheesePress},
    {20,  TargetMachine::RecyclingMachine},
    {21,  TargetMachine::Crystalarium},
    {24,  TargetMachine::MayonnaiseMachine},
    {25,  TargetMachine::SeedMaker},
    {105, TargetMachine::Tapper},
    {160, TargetMachine::StatueOfPerfection},
    {163, TargetMachine::Cask},
    {164, TargetMachine::StatueOfEndlessFortune},
    {182, TargetMachine::GeodeCrusher},
    {231, TargetMachine::SolarPanel},
    {264, TargetMachine::HeavyTapper},
    {280, TargetMachine::StatueOfTruePerfection},
};

} // namespace

TargetMachine ClassifyTargetMachine(Game::Object* object) noexcept {
    if (object == nullptr) {
        return TargetMachine::None;
    }

    const Game::ObjectView view(object);
    const auto parentSheetIndex = view.ReadParentSheetIndex();
    for (const auto& entry : kTargetIds) {
        if (entry.parentSheetIndex == parentSheetIndex) {
            return entry.machine;
        }
    }
    // Legacy machines normally retain numeric ParentSheetIndex values. Keep
    // an ItemId fallback for the two machines reported missing on hardware so
    // a data-driven/named 1.6 object cannot silently disappear from the scan.
    // These checks run only after all numeric IDs failed.
    if (view.HasItemId("16") || view.HasItemId("CheesePress")) {
        return TargetMachine::CheesePress;
    }
    if (view.HasItemId("25") || view.HasItemId("SeedMaker")) {
        return TargetMachine::SeedMaker;
    }
    if (view.HasItemId("15") || view.HasItemId("PreservesJar")) {
        return TargetMachine::PreservesJar;
    }
    if (view.HasItemId("182") || view.HasItemId("GeodeCrusher")) {
        return TargetMachine::GeodeCrusher;
    }
    if (view.HasItemId("Dehydrator")) return TargetMachine::Dehydrator;
    if (view.HasItemId("FishSmoker")) return TargetMachine::FishSmoker;
    if (view.HasItemId("HeavyFurnace")) return TargetMachine::HeavyFurnace;
    return TargetMachine::None;
}

const char* TargetMachineName(TargetMachine machine) noexcept {
    switch (machine) {
        case TargetMachine::CheesePress: return "Cheese Press";
        case TargetMachine::RecyclingMachine: return "Recycling Machine";
        case TargetMachine::SolarPanel: return "Solar Panel";
        case TargetMachine::StatueOfPerfection: return "Statue of Perfection";
        case TargetMachine::Crystalarium: return "Crystalarium";
        case TargetMachine::Keg: return "Keg";
        case TargetMachine::StatueOfEndlessFortune: return "Statue of Endless Fortune";
        case TargetMachine::Cask: return "Cask";
        case TargetMachine::Tapper: return "Tapper";
        case TargetMachine::Furnace: return "Furnace";
        case TargetMachine::StatueOfTruePerfection: return "Statue of True Perfection";
        case TargetMachine::SeedMaker: return "Seed Maker";
        case TargetMachine::MayonnaiseMachine: return "Mayonnaise Machine";
        case TargetMachine::LightningRod: return "Lightning Rod";
        case TargetMachine::HeavyTapper: return "Heavy Tapper";
        case TargetMachine::Dehydrator: return "Dehydrator";
        case TargetMachine::FishSmoker: return "Fish Smoker";
        case TargetMachine::HeavyFurnace: return "Heavy Furnace";
        case TargetMachine::PreservesJar: return "Preserves Jar";
        case TargetMachine::GeodeCrusher: return "Geode Crusher";
        default: return "None";
    }
}

} // namespace AutomateLite::Automate
