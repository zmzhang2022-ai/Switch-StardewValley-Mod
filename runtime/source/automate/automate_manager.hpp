#pragma once

#include <cstddef>
#include <cstdint>

#include "game/runtime.hpp"
#include "target_machines.hpp"

namespace AutomateLite::Automate {

class AutomateManager final {
public:
    static AutomateManager& Instance();
    void Update();

    std::uint64_t TickCount() const noexcept { return m_TickCount; }

private:
    static constexpr std::size_t kTargetMachineCount =
        static_cast<std::size_t>(TargetMachine::Count);

    void CollectReadyOutputsToChests(Game::GameLocation* location, Game::Farmer* player);
    Game::GameLocation* NextBackgroundLocation(
        const Game::LocationCollectionView& locations,
        Game::GameLocation* currentLocation);
    void AdvanceBackgroundRoot(std::uint32_t rootCount) noexcept;

    std::uint64_t m_TickCount{};
    std::uint64_t m_CollectedCount{};
    std::uint64_t m_FedCount{};
    std::uint64_t m_LocationViews{};
    std::uint64_t m_LocationViewFailures{};
    std::uint64_t m_InteriorLocationViews{};
    std::uint64_t m_InteriorLocationViewFailures{};
    std::uint64_t m_LocationScans{};
    std::uint64_t m_ObjectViews{};
    std::uint64_t m_ObjectViewFailures{};
    std::uint64_t m_ObjectEntries{};
    std::uint64_t m_MachineNodes{};
    std::uint64_t m_ChestNodes{};
    std::uint64_t m_ConnectorNodes{};
    std::uint64_t m_FishPondNodes{};
    std::uint64_t m_FishPondCollected{};
    std::uint64_t m_TerrainFeatureViews{};
    std::uint64_t m_TerrainFeatureViewFailures{};
    std::uint64_t m_TerrainFeatureEntries{};
    std::uint64_t m_FlooringNodes{};
    std::uint64_t m_BuildingViews{};
    std::uint64_t m_BuildingViewFailures{};
    std::uint64_t m_ConnectedGroups{};
    std::uint32_t m_LastLocationCount{};
    std::uint32_t m_LastRootLocationCount{};
    std::uint32_t m_LastInteriorLocationCount{};
    std::uint32_t m_LastObjectCount{};
    bool m_LastLocationsReadable{};
    bool m_LastLocationListNetField{};
    bool m_LastObjectsReadable{};

    std::uint64_t m_InputGroups{};
    std::uint64_t m_ChestViews{};
    std::uint64_t m_ChestViewFailures{};
    std::uint64_t m_ChestNetListViews{};
    std::uint64_t m_ChestItemSlots{};
    std::uint64_t m_InputCandidates{};
    std::uint64_t m_InputProbeAttempts{};
    std::uint64_t m_InputProbeMatches{};
    std::uint64_t m_InputCommitAttempts{};
    std::uint64_t m_InputCommitFailures{};
    std::uint64_t m_InputStartStateFailures{};
    std::uint64_t m_MachineSeen[kTargetMachineCount]{};
    std::uint8_t m_OutputStateLogs[kTargetMachineCount]{};
    std::uint8_t m_FlooringItemLogs{};
    std::uint32_t m_AuditMask{};
    std::uint32_t m_AuditScansAfterGroup{};
    std::uint32_t m_BackgroundRootCursor{};
    std::uint32_t m_BackgroundInteriorCursor{};
    std::uint32_t m_BackgroundKnownRootCount{};
    std::uint32_t m_BackgroundRoundInteriorCount{};
    bool m_BackgroundRootPending{true};
};

} // namespace AutomateLite::Automate
