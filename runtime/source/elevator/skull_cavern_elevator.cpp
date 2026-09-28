/* Copyright (C) 2026 zmzhang2022-ai | GPL-2.0-only | https://github.com/zmzhang2022-ai/Switch-StardewValley-Mod */
#include "elevator/skull_cavern_elevator.hpp"

#include "game/offsets.hpp"
#include "lib.hpp"

namespace AutomateLite::Elevator {
namespace {

using ManagedString = void;
using GetLocationFromNameFn = Game::GameLocation* (*)(ManagedString* name);
using GetMineLevelFn = std::int32_t (*)();
using GetDeepestMineLevelFn = std::int32_t (*)();
using FindTileFn = std::uint64_t (*)(
    Game::GameLocation* location, std::int32_t tileIndex,
    ManagedString* layer, ManagedString* tileSheet);
using SetMapTileFn = void* (*)(
    Game::GameLocation* location, std::int32_t x, std::int32_t y,
    std::int32_t tileIndex, ManagedString* layer,
    ManagedString* tileSheet, ManagedString* action,
    bool copyProperties);
using SetTilePropertyFn = void (*)(
    Game::GameLocation* location, std::int32_t x, std::int32_t y,
    ManagedString* layer, ManagedString* propertyName,
    ManagedString* propertyValue);
using GetTileSheetFn = void* (*)(void* map, ManagedString* id);
using TileSheetConstructorFn = void* (*)(
    void* instance, ManagedString* id, void* map,
    ManagedString* imageSource, std::uint64_t sheetSize,
    std::uint64_t tileSize);
using AddTileSheetFn = void (*)(void* map, void* tileSheet);
using PrepareElevatorFn = void (*)(Game::GameLocation* mine);

constexpr std::uint32_t kUpdateInterval = 30;
constexpr std::int32_t kSkullCaveLobbyX = 4;
constexpr std::int32_t kSkullCaveLobbyY = 3;
constexpr std::int32_t kLadderDownTileIndex = 115;
// The reference mod's mine.png is a 16-column copy of the game's mine sheet.
// Indices 80/96/112 are its vertically stacked upper/middle/lower elevator
// pieces (first column of rows 5/6/7).
constexpr std::int32_t kElevatorUpperTileIndex = 80;
constexpr std::int32_t kElevatorMiddleTileIndex = 96;
constexpr std::int32_t kElevatorLowerTileIndex = 112;

constexpr std::uint64_t PackSize(
    std::uint32_t width, std::uint32_t height) noexcept {
    return static_cast<std::uint64_t>(width) |
           (static_cast<std::uint64_t>(height) << 32);
}

constexpr std::uint64_t kMineSheetSize = PackSize(16, 18);
constexpr std::uint64_t kMineTileSize = PackSize(16, 16);

std::int32_t LowWord(std::uint64_t value) noexcept {
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(value));
}

std::int32_t HighWord(std::uint64_t value) noexcept {
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(value >> 32));
}

} // namespace

SkullCavernElevator& SkullCavernElevator::Instance() noexcept {
    static SkullCavernElevator instance;
    return instance;
}

void* SkullCavernElevator::ResolveManagedString(
    std::uintptr_t rootSlot) const noexcept {
    const auto slot = reinterpret_cast<void***>(
        exl::util::modules::GetTargetOffset(rootSlot));
    if (slot == nullptr || *slot == nullptr) {
        return nullptr;
    }
    return **slot;
}

Game::GameLocation* SkullCavernElevator::GetSkullCaveLobby() const noexcept {
    auto* const skullCave = static_cast<ManagedString*>(GetSkullCaveString());
    if (skullCave == nullptr) {
        return nullptr;
    }
    const auto getLocation = reinterpret_cast<GetLocationFromNameFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::Game1GetLocationFromName));
    return getLocation(skullCave);
}

void* SkullCavernElevator::GetSkullCaveString() const noexcept {
    return ResolveManagedString(StardewValley::Offsets::StringSlotSkullCave);
}

std::int32_t SkullCavernElevator::GetCurrentMineLevel() const noexcept {
    const auto getLevel = reinterpret_cast<GetMineLevelFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::Game1GetCurrentMineLevel));
    return getLevel();
}

std::int32_t SkullCavernElevator::GetDeepestMineLevel() const noexcept {
    const auto getDeepest = reinterpret_cast<GetDeepestMineLevelFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::MineShaftLowestLevelReached));
    return getDeepest();
}

bool SkullCavernElevator::IsElevatorContext() const noexcept {
    if (GetCurrentMineLevel() > BaseMineLevel) {
        return true;
    }
    auto* const current = Game::GameState::GetCurrentLocation();
    return current != nullptr && current == GetSkullCaveLobby();
}

void* SkullCavernElevator::FindMineArtworkTileSheetId(
    Game::GameLocation* location) const noexcept {
    if (location == nullptr) {
        return nullptr;
    }
    auto* const map = *reinterpret_cast<void**>(
        reinterpret_cast<std::uintptr_t>(location) +
        StardewValley::Offsets::GameLocationMap);
    if (map == nullptr) {
        return nullptr;
    }

    auto* const vanillaId = static_cast<ManagedString*>(
        ResolveManagedString(StardewValley::Offsets::StringSlotMineTileSheet));
    auto* const standardMineSource = static_cast<ManagedString*>(
        ResolveManagedString(
            StardewValley::Offsets::StringSlotMineImageSource));
    if (vanillaId == nullptr || standardMineSource == nullptr) {
        return nullptr;
    }

    const auto getTileSheet = reinterpret_cast<GetTileSheetFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::MapTryGetTileSheet));
    if (getTileSheet(map, vanillaId) != nullptr) {
        return vanillaId;
    }

    // The lobby has no TileSheet whose ID is "mine". Use the already rooted
    // image-source string as a collision-resistant internal ID, and add a
    // standard mine sheet exactly as the reference mod adds its custom sheet.
    // No external PNG is needed: Maps\Mines\mine is shipped by this game.
    if (getTileSheet(map, standardMineSource) != nullptr) {
        return standardMineSource;
    }
    const auto construct = reinterpret_cast<TileSheetConstructorFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::TileSheetConstructor));
    void* const sheet = construct(
        nullptr, standardMineSource, map, standardMineSource,
        kMineSheetSize, kMineTileSize);
    if (sheet == nullptr) {
        return nullptr;
    }
    const auto addTileSheet = reinterpret_cast<AddTileSheetFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::MapAddTileSheet));
    addTileSheet(map, sheet);
    return standardMineSource;
}

bool SkullCavernElevator::PlaceElevatorTile(
    Game::GameLocation* location, std::int32_t x,
    std::int32_t y) const noexcept {
    auto* const buildings = static_cast<ManagedString*>(
        ResolveManagedString(StardewValley::Offsets::StringSlotBuildings));
    auto* const front = static_cast<ManagedString*>(
        ResolveManagedString(StardewValley::Offsets::StringSlotFront));
    auto* const mineSheet = static_cast<ManagedString*>(
        FindMineArtworkTileSheetId(location));
    auto* const action = static_cast<ManagedString*>(
        ResolveManagedString(StardewValley::Offsets::StringSlotAction));
    auto* const mineElevator = static_cast<ManagedString*>(
        ResolveManagedString(StardewValley::Offsets::StringSlotMineElevator));
    if (location == nullptr || y < 2 || buildings == nullptr ||
        front == nullptr || mineSheet == nullptr || action == nullptr ||
        mineElevator == nullptr) {
        return false;
    }

    const auto setMapTile = reinterpret_cast<SetMapTileFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::GameLocationSetMapTile));
    void* const lower = setMapTile(
        location, x, y, kElevatorLowerTileIndex,
        buildings, mineSheet, mineElevator, true);
    void* const middle = setMapTile(
        location, x, y - 1, kElevatorMiddleTileIndex,
        front, mineSheet, nullptr, true);
    void* const upper = setMapTile(
        location, x, y - 2, kElevatorUpperTileIndex,
        front, mineSheet, nullptr, true);

    // Explicitly install the action on the lower 112 tile; relying on copied
    // source properties would make interaction depend on the source map data.
    // setTileProperty is null-safe for missing map/layer/tile in this build.
    const auto setTileProperty = reinterpret_cast<SetTilePropertyFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::GameLocationSetTileProperty));
    setTileProperty(location, x, y, buildings, action, mineElevator);
    return lower != nullptr && middle != nullptr && upper != nullptr;
}

bool SkullCavernElevator::InstallLobbyElevator(
    Game::GameLocation* lobby) noexcept {
    auto* const buildings = static_cast<ManagedString*>(
        ResolveManagedString(StardewValley::Offsets::StringSlotBuildings));
    auto* const back = static_cast<ManagedString*>(
        ResolveManagedString(StardewValley::Offsets::StringSlotBack));
    auto* const action = static_cast<ManagedString*>(
        ResolveManagedString(StardewValley::Offsets::StringSlotAction));
    auto* const mineElevator = static_cast<ManagedString*>(
        ResolveManagedString(StardewValley::Offsets::StringSlotMineElevator));
    if (lobby == nullptr || buildings == nullptr || back == nullptr ||
        action == nullptr || mineElevator == nullptr) {
        return false;
    }
    auto* const map = *reinterpret_cast<void**>(
        reinterpret_cast<std::uintptr_t>(lobby) +
        StardewValley::Offsets::GameLocationMap);
    if (map == nullptr) {
        return false;
    }

    // Find or add the standard shipped mine TileSheet without taking the
    // verified throwing GetTileSheet("mine") path, then place the reference
    // mod's 80/96/112 artwork stack.
    const bool visual = PlaceElevatorTile(
        lobby, kSkullCaveLobbyX, kSkullCaveLobbyY);
    const auto setTileProperty = reinterpret_cast<SetTilePropertyFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::GameLocationSetTileProperty));
    setTileProperty(
        lobby, kSkullCaveLobbyX, kSkullCaveLobbyY,
        buildings, action, mineElevator);
    setTileProperty(
        lobby, kSkullCaveLobbyX, kSkullCaveLobbyY,
        back, action, mineElevator);
    Logging.Log(
        "[SkullElevator] lobby installed at (4,3), visual=%u",
        visual ? 1U : 0U);
    return visual;
}

bool SkullCavernElevator::IsElevatorFloor(
    std::int32_t mineLevel) const noexcept {
    if (mineLevel <= BaseMineLevel) {
        return false;
    }
    if (mineLevel == BaseMineLevel + 1) {
        return true;
    }
    return ((mineLevel - BaseMineLevel) % ElevatorStep) == 0;
}

void SkullCavernElevator::InstallMineElevator(
    Game::GameLocation* mine, std::int32_t mineLevel) noexcept {
    auto* const buildings = static_cast<ManagedString*>(
        ResolveManagedString(StardewValley::Offsets::StringSlotBuildings));
    if (mine == nullptr || buildings == nullptr) {
        return;
    }

    // Match the PC mod's findLadder result: the lower elevator tile is on the
    // ladder's row, one tile to its right. A null tile-sheet filter searches
    // every tile sheet, exactly like the C# loop.
    const auto findTile = reinterpret_cast<FindTileFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::GameLocationFindTile));
    const std::uint64_t ladder = findTile(
        mine, kLadderDownTileIndex, buildings, nullptr);
    const std::int32_t ladderX = LowWord(ladder);
    const std::int32_t ladderY = HighWord(ladder);
    if (ladderX < 0 || ladderY < 0) {
        Logging.Log(
            "[SkullElevator] level=%d ladder tile 115 not found",
            mineLevel);
        return;
    }

    const bool visual = PlaceElevatorTile(mine, ladderX + 1, ladderY);
    const auto prepare = reinterpret_cast<PrepareElevatorFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::MineShaftPrepareElevator));
    prepare(mine);
    Logging.Log(
        "[SkullElevator] level=%d installed at (%d,%d), visual=%u",
        mineLevel, ladderX + 1, ladderY, visual ? 1U : 0U);
}

void SkullCavernElevator::Update() noexcept {
    if (++m_Throttle < kUpdateInterval) {
        return;
    }
    m_Throttle = 0;

    auto* const lobby = GetSkullCaveLobby();
    if (lobby != m_LastLobby) {
        m_LastLobby = lobby;
        m_LobbyInstalled = false;
    }
    if (lobby != nullptr && !m_LobbyInstalled) {
        // A location object may exist before its map/tile sheets are ready.
        // Mark it installed only after setMapTile returns the live tile so a
        // premature save-load tick cannot permanently suppress the elevator.
        m_LobbyInstalled = InstallLobbyElevator(lobby);
    }

    const std::int32_t mineLevel = GetCurrentMineLevel();
    if (!IsElevatorFloor(mineLevel)) {
        // MineShaft instances/maps can be recycled while descending. Always
        // invalidate the placement cache after leaving a qualifying floor.
        m_LastMine = nullptr;
        m_LastMineLevel = 0;
        return;
    }

    auto* const current = Game::GameState::GetCurrentLocation();
    if (current == nullptr ||
        (current == m_LastMine && mineLevel == m_LastMineLevel)) {
        return;
    }
    InstallMineElevator(current, mineLevel);
    m_LastMine = current;
    m_LastMineLevel = mineLevel;
}

} // namespace AutomateLite::Elevator
