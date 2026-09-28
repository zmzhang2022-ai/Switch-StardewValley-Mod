/* Copyright (C) 2026 zmzhang2022-ai | GPL-2.0-only | https://github.com/zmzhang2022-ai/Switch-StardewValley-Mod */
#include "automate_manager.hpp"
#include "target_machines.hpp"

#include "game/runtime.hpp"
#include "lib.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace AutomateLite::Automate {
namespace {

// The original Game.Tick is called first by the hook, so vanilla machine
// updates have settled before this scan observes them.
// Keep the live location responsive, but spread background locations across
// separate main-thread frames. A full background round still covers every
// loaded root and instantiated interior; only the scheduling changes.
constexpr std::uint64_t kCurrentLocationScanIntervalTicks = 30;
constexpr std::uint64_t kBackgroundLocationScanIntervalTicks = 4;
constexpr std::uint64_t kHeartbeatIntervalTicks = 1800;
constexpr std::uint64_t kHeartbeatPhase = 1;
constexpr std::int32_t kChestParentSheetIndex = 130;
constexpr std::int32_t kBigChestParentSheetIndex = 232;
constexpr std::size_t kMaximumFloodTiles = 16384;

enum AuditBit : std::uint32_t {
    AuditCheeseSeen       = 1U << 0,
    AuditSeedSeen         = 1U << 1,
    AuditFurnaceSeen      = 1U << 2,
    AuditChestSeen        = 1U << 3,
    AuditConnectedGroup   = 1U << 4,
    AuditCheeseHeld       = 1U << 5,
    AuditCheeseReady      = 1U << 6,
    AuditSeedHeld         = 1U << 7,
    AuditSeedReady        = 1U << 8,
    AuditChestReadable    = 1U << 9,
    AuditChestHasSlots    = 1U << 10,
    AuditInputCandidate   = 1U << 11,
    AuditInputProbeMatch  = 1U << 12,
    AuditInputCommit      = 1U << 13,
    AuditChestAccepted    = 1U << 14,
    AuditOutputReset      = 1U << 15,
};

struct TileCoord final {
    std::int32_t x{};
    std::int32_t y{};
};

enum class AutomationNodeKind : std::uint8_t {
    ObjectMachine,
    Chest,
    Connector,
    FishPond,
};

struct AutomationNode final {
    void* entity{};
    TileCoord tile{};
    std::int32_t width{1};
    std::int32_t height{1};
    TargetMachine machine{TargetMachine::None};
    AutomationNodeKind kind{AutomationNodeKind::ObjectMachine};
};

bool IsMachineNode(AutomationNodeKind kind) noexcept {
    return kind == AutomationNodeKind::ObjectMachine ||
        kind == AutomationNodeKind::FishPond;
}

const char* NodeMachineName(const AutomationNode& node) noexcept {
    return node.kind == AutomationNodeKind::FishPond
        ? "FishPond"
        : TargetMachineName(node.machine);
}

struct TileIndexEntry final {
    TileCoord tile{};
    std::uint32_t node{};
};

bool IsStorageChest(Game::Object* object) noexcept {
    if (object == nullptr) {
        return false;
    }
    const Game::ObjectView view(object);
    const auto id = view.ReadParentSheetIndex();
    if (id == kChestParentSheetIndex || id == kBigChestParentSheetIndex) {
        return true;
    }

    // ParentSheetIndex is only a sprite index in 1.6 and isn't a reliable item
    // identifier for named big craftables. Support both new large-Chest IDs as
    // well as numeric ItemId fallbacks for ordinary/stone Chests.
    return view.HasItemId("130") || view.HasItemId("232") ||
        view.HasItemId("BigChest") || view.HasItemId("BigStoneChest");
}

bool SupportsInput(TargetMachine machine) noexcept {
    // These machines generate output on their own or accept no item through
    // Object.PlaceInMachine.  Keeping them in the scan is intentional (their
    // output can still be collected), but they must never be offered an
    // arbitrary Chest item.
    switch (machine) {
        case TargetMachine::CheesePress:
        case TargetMachine::RecyclingMachine:
        case TargetMachine::Crystalarium:
        case TargetMachine::Keg:
        case TargetMachine::Cask:
        case TargetMachine::Furnace:
        case TargetMachine::SeedMaker:
        case TargetMachine::MayonnaiseMachine:
        case TargetMachine::Dehydrator:
        case TargetMachine::FishSmoker:
        case TargetMachine::HeavyFurnace:
        case TargetMachine::PreservesJar:
        case TargetMachine::GeodeCrusher:
            return true;
        default:
            return false;
    }
}

bool TryGetTile(Game::TilePosition position, TileCoord* tile) noexcept {
    // Object dictionary keys are tile-aligned floats. Reject malformed values
    // before the float-to-int conversion so a bad dictionary slot cannot poison
    // the flood fill.
    if (tile == nullptr || !(position.x >= -32768.0F && position.x <= 32768.0F) ||
        !(position.y >= -32768.0F && position.y <= 32768.0F)) {
        return false;
    }
    const auto x = static_cast<std::int32_t>(position.x);
    const auto y = static_cast<std::int32_t>(position.y);
    if (position.x != static_cast<float>(x) || position.y != static_cast<float>(y)) {
        return false;
    }
    tile->x = x;
    tile->y = y;
    return true;
}

int CompareTile(TileCoord left, TileCoord right) noexcept {
    if (left.x < right.x) return -1;
    if (left.x > right.x) return 1;
    if (left.y < right.y) return -1;
    if (left.y > right.y) return 1;
    return 0;
}

std::size_t LowerBoundTile(
    const std::vector<TileIndexEntry>& index, TileCoord tile) noexcept {
    std::size_t low = 0;
    std::size_t high = index.size();
    while (low < high) {
        const auto middle = low + (high - low) / 2;
        if (CompareTile(index[middle].tile, tile) < 0) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    return low;
}

std::uint64_t EncodeTile(TileCoord tile) noexcept {
    return (static_cast<std::uint64_t>(
                static_cast<std::uint32_t>(tile.x)) << 32U) |
        static_cast<std::uint32_t>(tile.y);
}

std::size_t HashTile(std::uint64_t key) noexcept {
    // SplitMix64 finalizer: deterministic and inexpensive on ARM64.
    key ^= key >> 30U;
    key *= 0xBF58476D1CE4E5B9ULL;
    key ^= key >> 27U;
    key *= 0x94D049BB133111EBULL;
    key ^= key >> 31U;
    return static_cast<std::size_t>(key);
}

std::size_t NextPowerOfTwo(std::size_t value) noexcept {
    std::size_t result = 1;
    while (result < value && result < (kMaximumFloodTiles * 2U)) {
        result <<= 1U;
    }
    return result;
}

bool QueueTileUnique(
    std::vector<TileCoord>& queue,
    std::vector<std::uint64_t>& queuedKeys,
    std::vector<std::uint32_t>& queuedEpochs,
    std::uint32_t epoch, TileCoord tile) noexcept {
    if (queue.size() >= kMaximumFloodTiles) {
        return false;
    }

    const auto capacity = queuedKeys.size();
    if (capacity == 0 || capacity != queuedEpochs.size() ||
        (capacity & (capacity - 1U)) != 0) {
        return false;
    }

    const auto key = EncodeTile(tile);
    auto slot = HashTile(key) & (capacity - 1U);
    for (std::size_t probe = 0; probe < capacity; ++probe) {
        if (queuedEpochs[slot] != epoch) {
            queuedEpochs[slot] = epoch;
            queuedKeys[slot] = key;
            queue.push_back(tile);
            return true;
        }
        if (queuedKeys[slot] == key) {
            return false;
        }
        slot = (slot + 1U) & (capacity - 1U);
    }
    return false;
}

} // namespace

AutomateManager& AutomateManager::Instance() {
    static AutomateManager manager;
    return manager;
}

void AutomateManager::AdvanceBackgroundRoot(
    std::uint32_t rootCount) noexcept {
    m_BackgroundInteriorCursor = 0;
    m_BackgroundRootPending = true;
    ++m_BackgroundRootCursor;
    if (rootCount != 0 && m_BackgroundRootCursor >= rootCount) {
        m_BackgroundRootCursor = 0;
        m_LastInteriorLocationCount = m_BackgroundRoundInteriorCount;
        m_LastLocationCount = rootCount + m_BackgroundRoundInteriorCount;
        m_BackgroundRoundInteriorCount = 0;
    }
}

Game::GameLocation* AutomateManager::NextBackgroundLocation(
    const Game::LocationCollectionView& locations,
    Game::GameLocation* currentLocation) {
    if (!locations.readable || locations.count == 0) {
        return nullptr;
    }

    if (m_BackgroundKnownRootCount != locations.count) {
        m_BackgroundKnownRootCount = locations.count;
        m_BackgroundRootCursor = 0;
        m_BackgroundInteriorCursor = 0;
        m_BackgroundRoundInteriorCount = 0;
        m_BackgroundRootPending = true;
    }
    if (m_BackgroundRootCursor >= locations.count) {
        m_BackgroundRootCursor = 0;
        m_BackgroundInteriorCursor = 0;
        m_BackgroundRootPending = true;
    }

    // Normally one iteration returns one location. Extra iterations only skip
    // a null entry, a completed root, or currentLocation (which has its own
    // fast scan). No managed pointer is retained after this call.
    const auto maximumAttempts = locations.count * 2U + 8U;
    for (std::uint32_t attempt = 0; attempt < maximumAttempts; ++attempt) {
        auto* const root = locations.Get(m_BackgroundRootCursor);
        if (root == nullptr) {
            const bool wrapped =
                m_BackgroundRootCursor + 1U >= locations.count;
            AdvanceBackgroundRoot(locations.count);
            if (wrapped) {
                return nullptr;
            }
            continue;
        }

        ++m_InteriorLocationViews;
        const auto interiors =
            Game::GameState::GetInstancedBuildingInteriors(root);
        if (!interiors.readable) {
            ++m_InteriorLocationViewFailures;
        }

        Game::GameLocation* candidate = nullptr;
        if (m_BackgroundRootPending) {
            m_BackgroundRootPending = false;
            m_BackgroundInteriorCursor = 0;
            if (interiors.readable) {
                m_BackgroundRoundInteriorCount += interiors.count;
            }
            candidate = root;
        } else if (interiors.readable &&
                   m_BackgroundInteriorCursor < interiors.count) {
            candidate = interiors.Get(m_BackgroundInteriorCursor++);
        } else {
            const bool wrapped =
                m_BackgroundRootCursor + 1U >= locations.count;
            AdvanceBackgroundRoot(locations.count);
            if (wrapped) {
                return nullptr;
            }
            continue;
        }

        if (candidate == nullptr || candidate == currentLocation) {
            continue;
        }
        return candidate;
    }
    return nullptr;
}

void AutomateManager::Update() {
    ++m_TickCount;

    if (m_TickCount == 1) {
        Logging.Log(
            "[AutomateLite] PC-style automation active: fast currentLocation + "
            "round-robin Game1.locations/interiors, cached terrain unwrap, "
            "hashed tile flood, output then input");
#if defined(AUTOMATE_LITE_PHASE4_FATAL_PROOF)
        EXL_ABORT("Automate Lite phase 4 fatal proof: C++ Game.Tick callback reached");
#endif
    }

    const bool scanCurrent =
        (m_TickCount % kCurrentLocationScanIntervalTicks) == 0;
    const bool scanBackground =
        !scanCurrent &&
        (m_TickCount % kBackgroundLocationScanIntervalTicks) == 0;
    if (scanCurrent || scanBackground) {
        const auto player = Game::GameState::GetPlayer();
        const auto currentLocation = Game::GameState::GetCurrentLocation();
        m_LastObjectCount = 0;
        m_LastObjectsReadable = true;
        bool scannedAnyLocation = false;

        // The live map retains the original 30-tick response time. It is never
        // stored between ticks, so a compacting managed GC cannot stale a
        // native cache.
        if (scanCurrent && currentLocation != nullptr) {
            ++m_LocationScans;
            CollectReadyOutputsToChests(currentLocation, player);
            scannedAnyLocation = true;
        }

        // Background coverage is distributed: one rooted map or instantiated
        // interior per scheduler slice. The cursor stores only indices/state;
        // the candidate pointer is reacquired from game-owned roots each tick.
        if (scanBackground) {
            const auto locations = Game::GameState::GetLoadedLocations();
            ++m_LocationViews;
            m_LastLocationsReadable = locations.readable;
            m_LastLocationListNetField = locations.usedNetFieldValue;
            m_LastRootLocationCount = locations.count;
            if (m_LocationViews == 1) {
                Logging.Log(
                    "[AutomateLite] scan=current/30ticks + background/4ticks "
                    "roots=%u readable=%u locNetValue=%u current=%u player=%u",
                    locations.count, locations.readable ? 1U : 0U,
                    locations.usedNetFieldValue ? 1U : 0U,
                    currentLocation != nullptr ? 1U : 0U,
                    player != nullptr ? 1U : 0U);
            }
            if (!locations.readable || locations.count == 0) {
                ++m_LocationViewFailures;
            } else if (auto* const background =
                           NextBackgroundLocation(locations, currentLocation);
                       background != nullptr) {
                ++m_LocationScans;
                CollectReadyOutputsToChests(background, player);
                scannedAnyLocation = true;
            }
        }

        if (!scannedAnyLocation) {
            m_LastObjectsReadable = false;
        }

#if defined(AUTOMATE_LITE_PHASE8_AUDIT)
        // This variant is intentionally fatal and exists only to transport a
        // compact runtime trace through Atmosphere's crash report. Wait for a
        // recognized machine/chest group and ten scans so both output and
        // input paths have had several opportunities to run.
        if ((m_AuditMask & AuditConnectedGroup) != 0 &&
            ++m_AuditScansAfterGroup >= 10) {
            const exl::Result result = static_cast<exl::Result>(
                0xA8000000U | (m_AuditMask & 0xFFFFU));
            exl::diag::AbortImpl("", "", "", 0, &result,
                                 "Automate Lite phase8 audit");
        }
#endif
    }

    // Keep diagnostics available without formatting and writing the long
    // heartbeat on a scan frame. At 60 ticks/s this is roughly once per 30s.
    if (m_TickCount > kHeartbeatPhase &&
        (m_TickCount % kHeartbeatIntervalTicks) == kHeartbeatPhase) {
        Logging.Log(
            "[AutomateLite] heartbeat: ticks=%llu outputs=%llu inputs=%llu "
            "locations=%u roots=%u interiors=%u readable=%u locNetValue=%u "
            "locationViews=%llu locationViewFail=%llu interiorViews=%llu "
            "interiorViewFail=%llu locationScans=%llu objects=%u objectReadable=%u "
            "objectViews=%llu objectFail=%llu entries=%llu machineNodes=%llu "
            "chestNodes=%llu connectorNodes=%llu fishPondNodes=%llu "
            "fishPondOutputs=%llu terrainViews=%llu terrainFail=%llu "
            "terrainEntries=%llu flooringNodes=%llu "
            "buildingViews=%llu buildingFail=%llu groups=%llu "
            "inputGroups=%llu chestViews=%llu "
            "chestReadFail=%llu chestNetViews=%llu slots=%llu candidates=%llu "
            "autoloadAttempts=%llu autoloadSuccess=%llu "
            "autoloadCalls=%llu autoloadFail=%llu startStateFail=%llu",
            static_cast<unsigned long long>(m_TickCount),
            static_cast<unsigned long long>(m_CollectedCount),
            static_cast<unsigned long long>(m_FedCount),
            m_LastLocationCount, m_LastRootLocationCount,
            m_LastInteriorLocationCount,
            m_LastLocationsReadable ? 1U : 0U,
            m_LastLocationListNetField ? 1U : 0U,
            static_cast<unsigned long long>(m_LocationViews),
            static_cast<unsigned long long>(m_LocationViewFailures),
            static_cast<unsigned long long>(m_InteriorLocationViews),
            static_cast<unsigned long long>(m_InteriorLocationViewFailures),
            static_cast<unsigned long long>(m_LocationScans),
            m_LastObjectCount, m_LastObjectsReadable ? 1U : 0U,
            static_cast<unsigned long long>(m_ObjectViews),
            static_cast<unsigned long long>(m_ObjectViewFailures),
            static_cast<unsigned long long>(m_ObjectEntries),
            static_cast<unsigned long long>(m_MachineNodes),
            static_cast<unsigned long long>(m_ChestNodes),
            static_cast<unsigned long long>(m_ConnectorNodes),
            static_cast<unsigned long long>(m_FishPondNodes),
            static_cast<unsigned long long>(m_FishPondCollected),
            static_cast<unsigned long long>(m_TerrainFeatureViews),
            static_cast<unsigned long long>(m_TerrainFeatureViewFailures),
            static_cast<unsigned long long>(m_TerrainFeatureEntries),
            static_cast<unsigned long long>(m_FlooringNodes),
            static_cast<unsigned long long>(m_BuildingViews),
            static_cast<unsigned long long>(m_BuildingViewFailures),
            static_cast<unsigned long long>(m_ConnectedGroups),
            static_cast<unsigned long long>(m_InputGroups),
            static_cast<unsigned long long>(m_ChestViews),
            static_cast<unsigned long long>(m_ChestViewFailures),
            static_cast<unsigned long long>(m_ChestNetListViews),
            static_cast<unsigned long long>(m_ChestItemSlots),
            static_cast<unsigned long long>(m_InputCandidates),
            static_cast<unsigned long long>(m_InputProbeAttempts),
            static_cast<unsigned long long>(m_InputProbeMatches),
            static_cast<unsigned long long>(m_InputCommitAttempts),
            static_cast<unsigned long long>(m_InputCommitFailures),
            static_cast<unsigned long long>(m_InputStartStateFailures));
    }
}

void AutomateManager::CollectReadyOutputsToChests(
    Game::GameLocation* location, Game::Farmer* player) {
    if (location == nullptr) {
        return;
    }

    const auto objects = Game::GameState::GetObjects(location);
    ++m_ObjectViews;
    m_LastObjectCount += objects.count;
    m_LastObjectsReadable = m_LastObjectsReadable && objects.readable;
    if (!objects.readable) {
        ++m_ObjectViewFailures;
    } else {
        m_ObjectEntries += objects.count;
    }

    const auto buildings = Game::GameState::GetBuildings(location);
    ++m_BuildingViews;
    if (!buildings.readable) {
        ++m_BuildingViewFailures;
    }

    // PC Automate indexes terrain features as first-class entities before it
    // builds groups. main+0x00DD5F70..0x00DD6048 proves the backing arrays:
    // dictionary+0x20 is Vector2 keys and +0x28 is TerrainFeature values.
    const auto terrainFeatures = Game::GameState::GetTerrainFeatures(location);
    ++m_TerrainFeatureViews;
    if (!terrainFeatures.readable) {
        ++m_TerrainFeatureViewFailures;
    }

    std::vector<AutomationNode> nodes;
    nodes.reserve(
        static_cast<std::size_t>(objects.count) +
        static_cast<std::size_t>(buildings.count) +
        static_cast<std::size_t>(terrainFeatures.count));
    for (std::uint32_t index = 0;
         objects.readable && index < objects.count; ++index) {
        if (!objects.IsActive(index)) {
            continue;
        }

        const auto object = objects.values[index];
        const auto target = ClassifyTargetMachine(object);
        const bool chest = IsStorageChest(object);
        if (target == TargetMachine::None && !chest) {
            continue;
        }

        TileCoord tile{};
        if (!TryGetTile(objects.GetKey(index), &tile)) {
            continue;
        }
        if (!chest) {
            const auto machineSlot = static_cast<std::size_t>(target);
            if (machineSlot < kTargetMachineCount &&
                ++m_MachineSeen[machineSlot] == 1) {
                Logging.Log(
                    "[AutomateLite] detected %s at (%d,%d)",
                    TargetMachineName(target),
                    static_cast<int>(tile.x), static_cast<int>(tile.y));
            }
            if (target == TargetMachine::CheesePress) {
                m_AuditMask |= AuditCheeseSeen;
            } else if (target == TargetMachine::SeedMaker) {
                m_AuditMask |= AuditSeedSeen;
            } else if (target == TargetMachine::Furnace) {
                m_AuditMask |= AuditFurnaceSeen;
            }
        }
        nodes.push_back(AutomationNode{
            object, tile, 1, 1, target,
            chest ? AutomationNodeKind::Chest
                  : AutomationNodeKind::ObjectMachine});
        if (chest) {
            ++m_ChestNodes;
            m_AuditMask |= AuditChestSeen;
        } else {
            ++m_MachineNodes;
        }
    }

    for (std::uint32_t index = 0;
         terrainFeatures.readable && index < terrainFeatures.count; ++index) {
        if (!terrainFeatures.IsActive(index)) {
            continue;
        }
        ++m_TerrainFeatureEntries;
        auto* const feature = terrainFeatures.Get(index);
        if (feature == nullptr) {
            continue;
        }
        if (!Game::FlooringView(feature).IsWoodPath()) {
            continue;
        }
        TileCoord tile{};
        if (!TryGetTile(terrainFeatures.GetKey(index), &tile)) {
            continue;
        }
        nodes.push_back(AutomationNode{
            feature, tile, 1, 1, TargetMachine::None,
            AutomationNodeKind::Connector});
        ++m_FlooringNodes;
        if (++m_ConnectorNodes == 1) {
            Logging.Log(
                "[AutomateLite] detected Wood Path connector at (%d,%d)",
                static_cast<int>(tile.x), static_cast<int>(tile.y));
        }
    }

    // Buildings are indexed by every tile in their footprint in PC Automate.
    // Fish Pond is output-only: it participates in connected groups but is
    // never offered Chest inventory through Object.AttemptAutoLoad.
    for (std::uint32_t index = 0;
         buildings.readable && index < buildings.count; ++index) {
        auto* const building = buildings.Get(index);
        const Game::FishPondView pond(building);
        if (!pond.IsFishPond()) {
            continue;
        }
        const auto area = pond.ReadTileArea();
        if (!area.readable) {
            continue;
        }
        nodes.push_back(AutomationNode{
            building, TileCoord{area.x, area.y}, area.width, area.height,
            TargetMachine::None, AutomationNodeKind::FishPond});
        ++m_MachineNodes;
        if (++m_FishPondNodes == 1) {
            Logging.Log(
                "[AutomateLite] detected FishPond at (%d,%d) size=%dx%d",
                static_cast<int>(area.x), static_cast<int>(area.y),
                static_cast<int>(area.width), static_cast<int>(area.height));
        }
    }
    if (nodes.empty()) {
        return;
    }

    // PC Automate flood-fills tiles, not a graph of only distinct machines.
    // At each reached tile it gets every indexed entity covering that tile,
    // including the connector nodes above. This also handles Flooring under a
    // Chest or machine.
    std::vector<TileIndexEntry> tileIndex;
    std::size_t indexedTileCount = 0;
    for (const auto& node : nodes) {
        indexedTileCount += static_cast<std::size_t>(node.width) *
            static_cast<std::size_t>(node.height);
    }
    tileIndex.reserve(indexedTileCount);
    for (std::uint32_t index = 0; index < nodes.size(); ++index) {
        const auto& node = nodes[index];
        for (std::int32_t y = 0; y < node.height; ++y) {
            for (std::int32_t x = 0; x < node.width; ++x) {
                tileIndex.push_back(TileIndexEntry{
                    TileCoord{node.tile.x + x, node.tile.y + y}, index});
            }
        }
    }
    std::sort(tileIndex.begin(), tileIndex.end(), [](const TileIndexEntry& left,
                                                     const TileIndexEntry& right) {
        const auto order = CompareTile(left.tile, right.tile);
        return order == 0 ? left.node < right.node : order < 0;
    });

    std::vector<std::uint8_t> visited(nodes.size(), 0);
    std::vector<TileCoord> tileQueue;
    std::vector<std::uint32_t> machines;
    std::vector<std::uint32_t> chests;
    const auto expectedFloodTiles = std::min<std::size_t>(
        kMaximumFloodTiles, indexedTileCount * 5U + 16U);
    const auto queuedHashCapacity = NextPowerOfTwo(std::max<std::size_t>(
        32U, expectedFloodTiles * 2U));
    std::vector<std::uint64_t> queuedKeys(queuedHashCapacity, 0);
    std::vector<std::uint32_t> queuedEpochs(queuedHashCapacity, 0);
    std::uint32_t queuedEpoch = 0;
    tileQueue.reserve(expectedFloodTiles);
    machines.reserve(nodes.size());
    chests.reserve(nodes.size());

    for (std::uint32_t root = 0; root < nodes.size(); ++root) {
        if (visited[root] != 0) {
            continue;
        }

        tileQueue.clear();
        machines.clear();
        chests.clear();
        if (++queuedEpoch == 0) {
            std::fill(queuedEpochs.begin(), queuedEpochs.end(), 0);
            queuedEpoch = 1;
        }
        QueueTileUnique(
            tileQueue, queuedKeys, queuedEpochs, queuedEpoch,
            nodes[root].tile);

        for (std::size_t cursor = 0; cursor < tileQueue.size(); ++cursor) {
            const TileCoord tile = tileQueue[cursor];

            // Match LocationFloodFillIndex.GetEntities(tile): add every node
            // whose tile area covers this exact tile, including multiple
            // entities layered on the same tile.
            const auto first = LowerBoundTile(tileIndex, tile);
            for (std::size_t entry = first;
                 entry < tileIndex.size() &&
                     CompareTile(tileIndex[entry].tile, tile) == 0;
                 ++entry) {
                const auto nodeIndex = tileIndex[entry].node;
                if (visited[nodeIndex] != 0) {
                    continue;
                }
                visited[nodeIndex] = 1;
                const auto& node = nodes[nodeIndex];
                if (node.kind == AutomationNodeKind::Chest) {
                    chests.push_back(nodeIndex);
                } else if (IsMachineNode(node.kind)) {
                    machines.push_back(nodeIndex);
                }

                // A newly added entity exposes its complete TileArea and the
                // four orthogonal surrounding tiles, exactly like PC
                // MachineGroupBuilder.NewTileAreas.
                for (std::int32_t y = 0; y < node.height; ++y) {
                    for (std::int32_t x = 0; x < node.width; ++x) {
                        const TileCoord covered{
                            node.tile.x + x, node.tile.y + y};
                        QueueTileUnique(
                            tileQueue, queuedKeys, queuedEpochs, queuedEpoch,
                            covered);
                        QueueTileUnique(
                            tileQueue, queuedKeys, queuedEpochs, queuedEpoch,
                            TileCoord{covered.x - 1, covered.y});
                        QueueTileUnique(
                            tileQueue, queuedKeys, queuedEpochs, queuedEpoch,
                            TileCoord{covered.x + 1, covered.y});
                        QueueTileUnique(
                            tileQueue, queuedKeys, queuedEpochs, queuedEpoch,
                            TileCoord{covered.x, covered.y - 1});
                        QueueTileUnique(
                            tileQueue, queuedKeys, queuedEpochs, queuedEpoch,
                            TileCoord{covered.x, covered.y + 1});
                    }
                }
            }

            // If the proven backing-array snapshot is temporarily unreadable,
            // retain the v3 vanilla TryGetValue lookup as a guarded fallback.
            if (!terrainFeatures.readable) {
                bool terrainReadable = false;
                auto* const feature = Game::GameState::GetTerrainFeatureAt(
                    location,
                    Game::TilePosition{
                        static_cast<float>(tile.x), static_cast<float>(tile.y)},
                    &terrainReadable);
                if (terrainReadable && feature != nullptr &&
                    Game::FlooringView(feature).IsWoodPath()) {
                    QueueTileUnique(
                        tileQueue, queuedKeys, queuedEpochs, queuedEpoch,
                        TileCoord{tile.x - 1, tile.y});
                    QueueTileUnique(
                        tileQueue, queuedKeys, queuedEpochs, queuedEpoch,
                        TileCoord{tile.x + 1, tile.y});
                    QueueTileUnique(
                        tileQueue, queuedKeys, queuedEpochs, queuedEpoch,
                        TileCoord{tile.x, tile.y - 1});
                    QueueTileUnique(
                        tileQueue, queuedKeys, queuedEpochs, queuedEpoch,
                        TileCoord{tile.x, tile.y + 1});
                }
            }
        }

        if (machines.empty() || chests.empty()) {
            continue;
        }
        ++m_ConnectedGroups;
        m_AuditMask |= AuditConnectedGroup;

        // Phase 1: push every completed output into any connected Chest. Only
        // after Chest.addItem accepts the complete Item do we GenericReset the
        // source machine. Thus a full chest leaves output in place.
        if (Game::ChestActions::IsAvailable()) {
            for (const auto machineIndex : machines) {
                auto& machineNode = nodes[machineIndex];
                Game::ObjectOutputState objectOutput{};
                Game::Item* outputItem = nullptr;
                bool outputReady = false;
                if (machineNode.kind == AutomationNodeKind::FishPond) {
                    outputItem = Game::FishPondView(
                        reinterpret_cast<Game::Building*>(machineNode.entity))
                            .ReadOutput();
                    outputReady = outputItem != nullptr;
                } else {
                    objectOutput = Game::ObjectView(
                        reinterpret_cast<Game::Object*>(machineNode.entity))
                            .ReadOutputState();
                    outputItem = objectOutput.heldObject;
                    if (machineNode.machine == TargetMachine::CheesePress &&
                        outputItem != nullptr) {
                        m_AuditMask |= AuditCheeseHeld;
                    } else if (machineNode.machine == TargetMachine::SeedMaker &&
                               outputItem != nullptr) {
                        m_AuditMask |= AuditSeedHeld;
                    }
                    outputReady = objectOutput.HasOutput();
                    if (outputReady) {
                        if (machineNode.machine == TargetMachine::CheesePress) {
                            m_AuditMask |= AuditCheeseReady;
                        } else if (machineNode.machine == TargetMachine::SeedMaker) {
                            m_AuditMask |= AuditSeedReady;
                        }
                    }
                }
                if (!outputReady) {
                    const auto machineSlot =
                        static_cast<std::size_t>(machineNode.machine);
                    if (machineNode.kind == AutomationNodeKind::ObjectMachine &&
                        outputItem != nullptr &&
                        machineSlot < kTargetMachineCount &&
                        m_OutputStateLogs[machineSlot] < 2) {
                        ++m_OutputStateLogs[machineSlot];
                        Logging.Log(
                            "[AutomateLite] %s held output not ready: "
                            "ready=%u hasTimer=%u minutes=%d",
                            TargetMachineName(machineNode.machine),
                            objectOutput.readyForHarvest ? 1U : 0U,
                            objectOutput.hasMinutesUntilReady ? 1U : 0U,
                            static_cast<int>(objectOutput.minutesUntilReady));
                    }
                    continue;
                }

                // Preserve the collected-item sample before Chest.addItem can
                // merge or take ownership of the machine's heldObject. The
                // sample drives OutputCollected and tapper follow-up rules.
                auto* const outputSample =
                    Game::MachineActions::CloneOne(outputItem);
                if (outputSample == nullptr) {
                    Logging.Log(
                        "[AutomateLite] ERROR: couldn't clone completed %s output",
                        NodeMachineName(machineNode));
                    continue;
                }

                bool moved = false;
                for (const auto chestIndex : chests) {
                    auto* const chest = reinterpret_cast<Game::Object*>(
                        nodes[chestIndex].entity);
                    auto* const leftover = Game::ChestActions::AddItem(
                        chest, outputItem);
                    if (leftover != nullptr) {
                        continue;
                    }
                    m_AuditMask |= AuditChestAccepted;

                    const bool lifecycleComplete =
                        machineNode.kind == AutomationNodeKind::FishPond
                        ? Game::FishPondView(
                              reinterpret_cast<Game::Building*>(
                                  machineNode.entity))
                              .CompleteOutputTransfer(outputSample, player)
                        : Game::ObjectView(
                              reinterpret_cast<Game::Object*>(
                                  machineNode.entity))
                              .CompleteOutputTransfer(
                                  outputSample, player, location);
                    if (!lifecycleComplete) {
                        Logging.Log(
                            "[AutomateLite] ERROR: Chest accepted %s but output lifecycle failed",
                            NodeMachineName(machineNode));
                        return;
                    }
                    m_AuditMask |= AuditOutputReset;

                    ++m_CollectedCount;
                    if (machineNode.kind == AutomationNodeKind::FishPond) {
                        ++m_FishPondCollected;
                    }
                    Logging.Log(
                        "[AutomateLite] output %s -> connected Chest at (%d,%d), total=%llu",
                        NodeMachineName(machineNode),
                        static_cast<int>(machineNode.tile.x),
                        static_cast<int>(machineNode.tile.y),
                        static_cast<unsigned long long>(m_CollectedCount));
                    moved = true;
                    break;
                }

                if (!moved) {
                    Logging.Log(
                        "[AutomateLite] storage full; retained %s output at (%d,%d)",
                        NodeMachineName(machineNode),
                        static_cast<int>(machineNode.tile.x),
                        static_cast<int>(machineNode.tile.y));
                }
            }
        }

        // Phase 2: match DataBasedObjectMachine.SetInput from Automate 2.6.1.
        // Give each connected Chest's concrete IInventory directly to the
        // game's Object.AttemptAutoLoad(IInventory, Farmer). Vanilla selects
        // the recipe and atomically consumes the required input/fuel; there is
        // no single-item probe, manual Coal clone, or guessed stack decrement.
        if (player == nullptr || !Game::MachineActions::IsInputAvailable()) {
            continue;
        }
        ++m_InputGroups;

        for (const auto machineIndex : machines) {
            auto& machineNode = nodes[machineIndex];
            if (machineNode.kind != AutomationNodeKind::ObjectMachine ||
                !SupportsInput(machineNode.machine)) {
                continue;
            }
            auto* const machineObject = reinterpret_cast<Game::Object*>(
                machineNode.entity);
            if (!Game::ObjectView(machineObject).ReadOutputState().IsEmpty()) {
                continue;
            }

            for (const auto chestIndex : chests) {
                const auto inventory = Game::ChestActions::GetItems(
                    reinterpret_cast<Game::Object*>(nodes[chestIndex].entity));
                ++m_ChestViews;
                if (!inventory.readable) {
                    ++m_ChestViewFailures;
                    continue;
                }
                m_AuditMask |= AuditChestReadable;
                if (inventory.usedNetFieldValue) {
                    ++m_ChestNetListViews;
                }
                m_ChestItemSlots += inventory.count;
                if (inventory.count != 0) {
                    m_AuditMask |= AuditChestHasSlots;
                }

                if (inventory.count == 0 || inventory.list == nullptr) {
                    continue;
                }

                ++m_InputCandidates;
                ++m_InputProbeAttempts;
                ++m_InputCommitAttempts;
                m_AuditMask |= AuditInputCandidate;
                if (!Game::MachineActions::AttemptAutoLoad(
                        machineObject, inventory.list, player)) {
                    ++m_InputCommitFailures;
                    continue;
                }

                ++m_InputProbeMatches;
                m_AuditMask |= AuditInputProbeMatch;
                m_AuditMask |= AuditInputCommit;
                if (Game::ObjectView(machineObject).ReadOutputState().IsEmpty()) {
                    // AttemptAutoLoad owns the transaction. A true return with
                    // no heldObject is useful ABI evidence, but we must not
                    // manually alter the Chest because vanilla may have used
                    // a specialized non-heldObject processing state.
                    ++m_InputStartStateFailures;
                    Logging.Log(
                        "[AutomateLite] AttemptAutoLoad returned true for %s "
                        "without a heldObject",
                        TargetMachineName(machineNode.machine));
                }

                ++m_FedCount;
                Logging.Log(
                    "[AutomateLite] input %s <- connected Chest at (%d,%d), total=%llu",
                    TargetMachineName(machineNode.machine),
                    static_cast<int>(machineNode.tile.x),
                    static_cast<int>(machineNode.tile.y),
                    static_cast<unsigned long long>(m_FedCount));
                break;
            }
        }
    }
}

} // namespace AutomateLite::Automate
