#pragma once

#include <cstdint>

#include "game/offsets.hpp"

namespace AutomateLite::Game {

// Opaque Brute/AOT game objects. Their managed layouts are version-locked;
// do not replace these with guessed C++ class definitions.
using Object = void;
using Farmer = void;
using Item = void;
using GameLocation = void;
using TerrainFeature = void;
using Building = void;

struct TilePosition final {
    float x{};
    float y{};
};

struct ObjectOutputState final {
    Item* heldObject{};
    std::int32_t minutesUntilReady{};
    bool readyForHarvest{};
    bool hasMinutesUntilReady{};

    bool HasOutput() const noexcept {
        // This is DataBasedObjectMachine.GetState's exact Done condition.
        // Timer and interaction-probe fallbacks can collect a machine early.
        return heldObject != nullptr && readyForHarvest;
    }

    bool IsEmpty() const noexcept {
        return heldObject == nullptr;
    }
};

// A non-owning snapshot of the actual IList<Item> returned by
// Chest.GetItemsForPlayer. Count/get_Item use the same Brute/AOT interface
// dispatch as the game, so no managed List layout is guessed.
struct ItemCollectionView final {
    void* list{};
    std::uint32_t count{};
    bool getterReturnedList{};
    bool readable{};
    bool usedNetFieldValue{};

    Item* Get(std::uint32_t index) const noexcept;
};

// A non-owning snapshot of a game-owned location collection. The root view is
// Game1.locations; interior views come from the original
// GameLocation.GetInstancedBuildingInteriors method. Automation never loads or
// creates a location merely to process it.
struct LocationCollectionView final {
    GameLocation* const* locations{};
    std::uint32_t count{};
    bool readable{};
    bool usedNetFieldValue{};

    GameLocation* Get(std::uint32_t index) const noexcept {
        return locations != nullptr && index < count ? locations[index] : nullptr;
    }
};

// A non-owning snapshot of a location's Object dictionary backing arrays. The
// dictionary capacity can exceed its logical item count, so null values mark
// unused/deleted slots. The data stays valid only in the current game tick.
struct ObjectCollectionView final {
    const TilePosition* keys{};
    Object* const* values{};
    const std::uint64_t* entryMetadata{};
    std::uint32_t count{};
    bool readable{};

    bool IsActive(std::uint32_t index) const noexcept {
        // Do not depend on a guessed entry-metadata layout. On this build the
        // values backing array is authoritative for live NetDictionary pairs.
        return index < count && values != nullptr && values[index] != nullptr;
    }

    TilePosition GetKey(std::uint32_t index) const noexcept {
        return keys != nullptr && index < count ? keys[index] : TilePosition{};
    }
};

// The private dictionary stores network-field wrappers. Get() reproduces the
// NetVector2Dictionary virtual unwrap used by vanilla and returns the public
// TerrainFeature value exposed through location.terrainFeatures.Pairs.
struct TerrainFeatureCollectionView final {
    void* owner{};
    const TilePosition* keys{};
    void* const* fields{};
    std::uintptr_t unwrapFunction{};
    std::uint8_t unwrapThisAdjustment{};
    std::uint32_t count{};
    bool readable{};

    bool IsActive(std::uint32_t index) const noexcept {
        return index < count && fields != nullptr && fields[index] != nullptr;
    }

    TilePosition GetKey(std::uint32_t index) const noexcept {
        return keys != nullptr && index < count ? keys[index] : TilePosition{};
    }

    TerrainFeature* Get(std::uint32_t index) const noexcept;
};

// Non-owning view over GameLocation.buildings. The shipped AOT method at
// main+0x0149FFB0 enumerates the concrete List<Building> at collection+0x48;
// this snapshot exposes that same list without creating or loading buildings.
struct BuildingCollectionView final {
    Building* const* buildings{};
    std::uint32_t count{};
    bool readable{};

    Building* Get(std::uint32_t index) const noexcept {
        return readable && buildings != nullptr && index < count
            ? buildings[index]
            : nullptr;
    }
};

struct BuildingTileArea final {
    std::int32_t x{};
    std::int32_t y{};
    std::int32_t width{};
    std::int32_t height{};
    bool readable{};
};

class FlooringView final {
public:
    explicit FlooringView(TerrainFeature* feature) noexcept : m_Feature(feature) {}

    bool IsFlooring() const noexcept;
    bool IsWoodPath() const noexcept;
    bool CopyItemId(char* output, std::uint32_t capacity) const noexcept;

private:
    TerrainFeature* m_Feature{};
};

class FishPondView final {
public:
    explicit FishPondView(Building* building) noexcept : m_Building(building) {}

    bool IsFishPond() const noexcept;
    bool IsUnderConstruction() const noexcept;
    BuildingTileArea ReadTileArea() const noexcept;
    Item* ReadOutput() const noexcept;

    // Called only after Chest.addItem accepts the complete output.  Clears the
    // original NetRef with its virtual setter, then awards the same Fishing XP
    // formula used by vanilla/PC Automate to the supplied Farmer.
    bool CompleteOutputTransfer(Item* outputSample, Farmer* who) const noexcept;

private:
    Building* m_Building{};
};

// Read-only view of the NetRef/NetBool fields used by the machine paths.
class ObjectView final {
public:
    explicit ObjectView(Object* object) noexcept : m_Object(object) {}

    ObjectOutputState ReadOutputState() const noexcept;
    std::int32_t ReadParentSheetIndex() const noexcept;
    bool HasItemId(const char* expected) const noexcept;

    // Apply the data-based Object output lifecycle after a Chest accepts the
    // complete output. This clears the vanilla NetFields, resets the item
    // index, asks MachineDataUtility for a queued OnHarvest output (Seed Maker
    // and other multi-output rules), and refreshes a tapper's tree product.
    bool CompleteOutputTransfer(
        Item* collectedItem, Farmer* who, GameLocation* location) const noexcept;

private:
    Object* m_Object{};
};

class GameState final {
public:
    static Farmer* GetPlayer() noexcept;
    static GameLocation* GetCurrentLocation() noexcept;
    static LocationCollectionView GetLoadedLocations() noexcept;
    static LocationCollectionView GetInstancedBuildingInteriors(
        GameLocation* location) noexcept;
    static ObjectCollectionView GetObjects(GameLocation* location) noexcept;
    static TerrainFeatureCollectionView GetTerrainFeatures(
        GameLocation* location) noexcept;
    // Query the live NetVector2Dictionary through the same concrete
    // TryGetValue body used by Object's vanilla tapper-output path. `readable`
    // distinguishes an empty tile from an invalid location/dictionary view.
    static TerrainFeature* GetTerrainFeatureAt(
        GameLocation* location, TilePosition tile,
        bool* readable = nullptr) noexcept;
    static BuildingCollectionView GetBuildings(GameLocation* location) noexcept;
};

// ABI-only argument block retained for the separately gated historical trace
// hook. Normal automation never calls checkForAction to collect output.
struct CheckForActionArgs final {
    Farmer* who{};
    const std::uint8_t* justChecking{};
};

class ChestActions final {
public:
    using AddItemFn = Item* (*)(Object* chest, Item** item);

    static bool IsAvailable() noexcept;
    static Item* AddItem(Object* chest, Item* item) noexcept;
    static ItemCollectionView GetItems(Object* chest) noexcept;
};

class MachineActions final {
public:
    // Create the stable one-item sample used by the game's OnCollected rule
    // and tapper follow-up before Chest.addItem mutates/owns the output stack.
    static Item* CloneOne(Item* item) noexcept;

    // Calls Object.AttemptAutoLoad(IInventory, Farmer) through the concrete
    // machine's virtual slot. The game selects recipes and consumes all
    // required ingredients (including fuel) from the supplied Chest inventory.
    static bool IsInputAvailable() noexcept;
    static bool AttemptAutoLoad(
        Object* machine, void* inventory, Farmer* who) noexcept;

};

} // namespace AutomateLite::Game
