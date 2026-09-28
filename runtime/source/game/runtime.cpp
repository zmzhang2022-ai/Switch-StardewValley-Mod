#include "game/runtime.hpp"

#include "lib.hpp"

#include <utility>

namespace AutomateLite::Game {
namespace {

constexpr std::uintptr_t kObjectReadyForHarvestRef = 0x118;
constexpr std::uintptr_t kObjectVtable = 0x0;
constexpr std::uintptr_t kNetDictionaryDictionary = 0x20;
constexpr std::uintptr_t kDictionaryEntryMetadata = 0x18;
constexpr std::uintptr_t kDictionaryKeys = 0x20;
constexpr std::uintptr_t kDictionaryValues = 0x28;
constexpr std::uintptr_t kBruteArrayData = 0x10;
constexpr std::uintptr_t kBruteArrayBounds = 0x20;
constexpr std::uintptr_t kNetFieldSetter = 0x230;
constexpr std::uintptr_t kNetFieldThisAdjustment = 0x238;
constexpr std::uint32_t kMaximumObjectCount = 4096;
constexpr std::uintptr_t kListItemsArray = 0x10;
constexpr std::uintptr_t kListSize = 0x18;
constexpr std::uintptr_t kNetFieldValue = StardewValley::Offsets::NetRefValue;
constexpr std::uintptr_t kListProbeEnd = 0x80;

// Objects returned by the Switch managed heap are in the 0x17../0x18..
// address range seen in the crash dumps.  Reject null, tiny, and unaligned
// values before following any guessed managed-layout offset.  This is a
// guardrail, not a substitute for proving a new layout in Ghidra/runtime
// trace; an unknown layout is reported as an unreadable Chest instead of
// dereferenced.
bool IsLikelyManagedPointer(std::uintptr_t address) noexcept {
    return address >= 0x100000000ULL && (address & 0x7U) == 0;
}

bool IsReadableAddressRange(std::uintptr_t address, std::uintptr_t size) noexcept {
    if (address == 0 || size == 0 ||
        address > (std::numeric_limits<std::uintptr_t>::max() - size)) {
        return false;
    }

    MemoryInfo memory{};
    u32 pageInfo{};
    if (R_FAILED(svcQueryMemory(&memory, &pageInfo, address)) ||
        memory.type == MemType_Unmapped || (memory.perm & Perm_R) == 0) {
        return false;
    }

    const auto memoryEnd = memory.addr + memory.size;
    return memoryEnd >= memory.addr && address >= memory.addr &&
           address + size <= memoryEnd;
}

bool IsReadableRange(std::uintptr_t address, std::uintptr_t size) noexcept {
    return IsLikelyManagedPointer(address) &&
           IsReadableAddressRange(address, size);
}

using GetPlayerFn = Farmer* (*)();
using GetLocationsFn = void* (*)();
using GetCurrentLocationFn = GameLocation* (*)();
using GetInstancedBuildingInteriorsFn = void* (*)(GameLocation* location);
using GetLocationObjectsFn = void* (*)(GameLocation* location);
using NetFieldSetFn = void (*)(void* field, std::uintptr_t value);
// Keep the two-register call shape used by the generated wrapper.  On this
// exact offset the wrapper recomputes the current player's ID itself before
// tail-calling the list implementation; X1 is still supplied explicitly so
// this call cannot depend on a stale register if the wrapper is replaced by a
// direct overload in a later build.
using GetChestItemsFn = void* (*)(Object* chest, std::int64_t playerId);
using AttemptAutoLoadVirtualFn = bool (*)(
    Object* machine, void* inventory, Farmer* who);
using GetOneFn = Item* (*)(Item* item);
using GetMachineDataFn = void* (*)(Object* machine);
using ResetParentSheetIndexFn = void (*)(Item* item);
using TryGetMachineOutputRuleFn = bool (*)(
    Object* machine, void* machineData, std::int32_t trigger,
    Item* inputItem, Farmer* who, GameLocation* location,
    void** outputRule, void** outputId, void** outputItem, void** outputData);
using OutputMachineVirtualFn = bool (*)(
    Object* machine, void* machineData, void* outputRule,
    Item* lastInputItem, Farmer* who, GameLocation* location,
    bool probe, bool overrideOutput);
using IsTapperVirtualFn = bool (*)(Object* machine);
using TerrainFeatureTryGetValueFn = bool (*)(
    void* dictionary, TilePosition tile, void** terrainFeature);
using TerrainFeatureUnwrapFn = TerrainFeature* (*)(
    void* netDictionary, void* field);
using UpdateTapperProductFn = void (*)(
    void* tree, Object* tapper, Item* previousOutput, bool probe);
using InterfaceMetadataFactoryFn = void* (*)();
using GetFlooringDataFn = void* (*)(TerrainFeature* flooring);
using IsInstanceFn = void* (*)(void* object, void* type);
using SellToStorePriceFn = std::int32_t (*)(Item* item, std::int32_t specificPlayerId);
using GainExperienceFn = void (*)(Farmer* who, std::int32_t skill, std::int32_t amount);

struct InterfaceMethod final {
    std::uintptr_t function{};
    std::uint8_t thisAdjustment{};
};

using ResolveInterfaceMethodFn = InterfaceMethod* (*)(
    void* object, void* interfaceMetadata, std::int32_t slot);
using ICollectionCountFn = std::int32_t (*)(void* collection);
using IListGetItemFn = Item* (*)(void* list, std::int32_t index);

void* g_ICollectionItemMetadata{};
void* g_IListItemMetadata{};

struct VirtualMethod final {
    std::uintptr_t function{};
    std::uint8_t thisAdjustment{};
};

bool TryResolveVirtualMethod(
    void* object, std::uintptr_t functionSlot,
    std::uintptr_t adjustmentSlot, VirtualMethod* result) noexcept {
    if (object == nullptr || result == nullptr) {
        return false;
    }

    const auto objectAddress = reinterpret_cast<std::uintptr_t>(object);
    if (!IsReadableRange(objectAddress, sizeof(std::uintptr_t))) {
        return false;
    }
    const auto vtable = *reinterpret_cast<const std::uintptr_t*>(objectAddress);
    if (vtable == 0 ||
        !IsReadableAddressRange(vtable, 2 * sizeof(std::uintptr_t))) {
        return false;
    }
    const auto methods = *reinterpret_cast<const std::uintptr_t*>(
        vtable + sizeof(std::uintptr_t));
    const auto requiredEnd =
        (functionSlot > adjustmentSlot ? functionSlot : adjustmentSlot) +
        sizeof(std::uintptr_t);
    if (methods == 0 || !IsReadableAddressRange(methods, requiredEnd)) {
        return false;
    }

    result->function = *reinterpret_cast<const std::uintptr_t*>(
        methods + functionSlot);
    result->thisAdjustment = *reinterpret_cast<const std::uint8_t*>(
        methods + adjustmentSlot);
    return result->function != 0;
}

TerrainFeature* UnwrapTerrainFeatureField(
    void* owner, void* field) noexcept {
    if (owner == nullptr || field == nullptr) {
        return nullptr;
    }
    VirtualMethod method{};
    if (!TryResolveVirtualMethod(
            owner,
            StardewValley::Offsets::TerrainFeatureUnwrapVirtualSlot,
            StardewValley::Offsets::TerrainFeatureUnwrapThisAdjustment,
            &method)) {
        return nullptr;
    }
    const auto unwrap = reinterpret_cast<TerrainFeatureUnwrapFn>(
        method.function);
    return unwrap(
        reinterpret_cast<void*>(
            reinterpret_cast<std::uintptr_t>(owner) + method.thisAdjustment),
        field);
}

bool ManagedStringEqualsAscii(void* string, const char* expected) noexcept {
    if (string == nullptr || expected == nullptr) {
        return false;
    }

    const auto stringAddress = reinterpret_cast<std::uintptr_t>(string);
    // Brute strings observed in this build use one of these two header sizes.
    // Require the complete candidate character range to be readable before
    // comparing it, so a malformed NetString cannot become a connector.
    for (const std::uintptr_t lengthOffset : {0x8U, 0x10U}) {
        if (!IsReadableRange(stringAddress, lengthOffset + sizeof(std::int32_t))) {
            continue;
        }
        const auto length = *reinterpret_cast<const std::int32_t*>(
            stringAddress + lengthOffset);
        if (length < 0 || length > 64) {
            continue;
        }
        const auto charsOffset = lengthOffset + sizeof(std::int32_t);
        const auto charsSize = static_cast<std::uintptr_t>(length) * 2U;
        if (!IsReadableRange(stringAddress, charsOffset + charsSize)) {
            continue;
        }

        std::uint32_t index = 0;
        for (; expected[index] != '\0' &&
               index < static_cast<std::uint32_t>(length); ++index) {
            const auto character = *reinterpret_cast<const std::uint16_t*>(
                stringAddress + charsOffset +
                static_cast<std::uintptr_t>(index) * 2U);
            if (character != static_cast<std::uint8_t>(expected[index])) {
                break;
            }
        }
        if (expected[index] == '\0' &&
            index == static_cast<std::uint32_t>(length)) {
            return true;
        }
    }
    return false;
}

bool CopyManagedStringAscii(
    void* string, char* output, std::uint32_t capacity) noexcept {
    if (string == nullptr || output == nullptr || capacity == 0) {
        return false;
    }
    output[0] = '\0';

    const auto stringAddress = reinterpret_cast<std::uintptr_t>(string);
    for (const std::uintptr_t lengthOffset : {0x8U, 0x10U}) {
        if (!IsReadableRange(stringAddress,
                lengthOffset + sizeof(std::int32_t))) {
            continue;
        }
        const auto length = *reinterpret_cast<const std::int32_t*>(
            stringAddress + lengthOffset);
        if (length < 0 ||
            static_cast<std::uint32_t>(length) >= capacity || length > 64) {
            continue;
        }
        const auto charsOffset = lengthOffset + sizeof(std::int32_t);
        if (!IsReadableRange(
                stringAddress,
                charsOffset + static_cast<std::uintptr_t>(length) * 2U)) {
            continue;
        }
        bool ascii = true;
        for (std::int32_t index = 0; index < length; ++index) {
            const auto character = *reinterpret_cast<const std::uint16_t*>(
                stringAddress + charsOffset +
                static_cast<std::uintptr_t>(index) * 2U);
            if (character > 0x7FU) {
                ascii = false;
                break;
            }
            output[index] = static_cast<char>(character);
        }
        if (ascii) {
            output[length] = '\0';
            return true;
        }
    }
    output[0] = '\0';
    return false;
}

bool TryReadNetInt(
    void* object, std::uintptr_t fieldOffset, std::int32_t* value) noexcept {
    if (object == nullptr || value == nullptr) {
        return false;
    }
    const auto objectAddress = reinterpret_cast<std::uintptr_t>(object);
    if (!IsReadableRange(objectAddress, fieldOffset + sizeof(std::uintptr_t))) {
        return false;
    }
    const auto field = *reinterpret_cast<const std::uintptr_t*>(
        objectAddress + fieldOffset);
    if (!IsReadableRange(field,
            StardewValley::Offsets::NetRefValue + sizeof(std::int32_t))) {
        return false;
    }
    *value = *reinterpret_cast<const std::int32_t*>(
        field + StardewValley::Offsets::NetRefValue);
    return true;
}

void* GetInterfaceMetadata(
    std::uintptr_t factoryOffset, void** cache) noexcept {
    if (cache == nullptr) {
        return nullptr;
    }
    if (*cache == nullptr) {
        const auto factory = reinterpret_cast<InterfaceMetadataFactoryFn>(
            exl::util::modules::GetTargetOffset(factoryOffset));
        *cache = factory();
    }
    return *cache;
}

InterfaceMethod* ResolveInterfaceMethod(
    void* object, void* metadata, std::int32_t slot) noexcept {
    if (object == nullptr || metadata == nullptr ||
        !StardewValley::Offsets::IsKnown(
            StardewValley::Offsets::ResolveInterfaceMethod)) {
        return nullptr;
    }

    const auto resolve = reinterpret_cast<ResolveInterfaceMethodFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::ResolveInterfaceMethod));
    auto* const method = resolve(object, metadata, slot);
    return method != nullptr && method->function != 0 ? method : nullptr;
}

bool TryGetICollectionCount(void* collection, std::uint32_t* count) noexcept {
    if (collection == nullptr || count == nullptr ||
        !IsReadableRange(reinterpret_cast<std::uintptr_t>(collection), 0x10)) {
        return false;
    }

    void* const metadata = GetInterfaceMetadata(
        StardewValley::Offsets::ICollectionItemMetadataFactory,
        &g_ICollectionItemMetadata);
    auto* const method = ResolveInterfaceMethod(collection, metadata, 0);
    if (method == nullptr) {
        return false;
    }

    const auto getCount = reinterpret_cast<ICollectionCountFn>(method->function);
    const auto adjustedThis = reinterpret_cast<void*>(
        reinterpret_cast<std::uintptr_t>(collection) + method->thisAdjustment);
    const auto logicalCount = getCount(adjustedThis);
    if (logicalCount < 0 ||
        static_cast<std::uint32_t>(logicalCount) > kMaximumObjectCount) {
        return false;
    }
    *count = static_cast<std::uint32_t>(logicalCount);
    return true;
}

Item* TryGetIListItem(void* list, std::uint32_t index) noexcept {
    if (list == nullptr || index > 0x7FFFFFFFU) {
        return nullptr;
    }

    void* const metadata = GetInterfaceMetadata(
        StardewValley::Offsets::IListItemMetadataFactory,
        &g_IListItemMetadata);
    auto* const method = ResolveInterfaceMethod(list, metadata, 0);
    if (method == nullptr) {
        return nullptr;
    }

    const auto getItem = reinterpret_cast<IListGetItemFn>(method->function);
    const auto adjustedThis = reinterpret_cast<void*>(
        reinterpret_cast<std::uintptr_t>(list) + method->thisAdjustment);
    return getItem(adjustedThis, static_cast<std::int32_t>(index));
}

bool TryReadArray(void* array, const void** data, std::uint32_t* count) noexcept {
    if (array == nullptr || data == nullptr || count == nullptr) {
        return false;
    }

    const auto arrayAddress = reinterpret_cast<std::uintptr_t>(array);
    if (!IsReadableRange(arrayAddress, kBruteArrayBounds + sizeof(std::uint32_t))) {
        return false;
    }
    const auto values = *reinterpret_cast<const void* const*>(arrayAddress + kBruteArrayData);
    const auto bounds = *reinterpret_cast<const std::uintptr_t*>(arrayAddress + kBruteArrayBounds);
    if (!IsLikelyManagedPointer(reinterpret_cast<std::uintptr_t>(values)) ||
        !IsReadableRange(bounds, sizeof(std::int32_t))) {
        return false;
    }

    const auto length = *reinterpret_cast<const std::int32_t*>(bounds);
    if (length < 0 || static_cast<std::uint32_t>(length) > kMaximumObjectCount) {
        return false;
    }

    *data = values;
    *count = static_cast<std::uint32_t>(length);
    return true;
}

bool TryReadDirectList(
    std::uintptr_t listAddress, const void** data, std::uint32_t* count) noexcept {
    if (data == nullptr || count == nullptr ||
        !IsReadableRange(listAddress, kListProbeEnd)) {
        return false;
    }

    // Brute emits both direct List<T> objects and NetList/IList wrappers in
    // this build.  The normal List layout is (_items,+0x10), (_size,+0x18),
    // but a wrapper can add one or more object fields before those members.
    // Probe only a small set of ABI layouts and accept one only when its
    // backing array and capacity are internally consistent.
    constexpr std::pair<std::uintptr_t, std::uintptr_t> kLayouts[] = {
        {0x10, 0x18}, {0x18, 0x20}, {0x20, 0x28}, {0x28, 0x30},
    };
    bool sawEmptyList = false;
    for (const auto [itemsOffset, sizeOffset] : kLayouts) {
        const auto logicalSize = *reinterpret_cast<const std::int32_t*>(
            listAddress + sizeOffset);
        const auto itemsArray = *reinterpret_cast<void* const*>(
            listAddress + itemsOffset);
        if (logicalSize < 0 ||
            static_cast<std::uint32_t>(logicalSize) > kMaximumObjectCount) {
            continue;
        }

        // A freshly constructed managed List<T> may use a null/empty backing
        // array. This is a valid empty inventory, not evidence that the
        // returned object is a NetField.
        if (logicalSize == 0 && itemsArray == nullptr) {
            sawEmptyList = true;
            continue;
        }

        std::uint32_t capacity = 0;
        const void* values = nullptr;
        if (!TryReadArray(itemsArray, &values, &capacity) ||
            static_cast<std::uint32_t>(logicalSize) > capacity) {
            continue;
        }

        *data = values;
        *count = static_cast<std::uint32_t>(logicalSize);
        return true;
    }
    if (sawEmptyList) {
        *data = nullptr;
        *count = 0;
        return true;
    }
    return false;
}

bool TryReadNestedList(
    std::uintptr_t wrapperAddress, const void** data, std::uint32_t* count,
    bool* nested) noexcept {
    if (data == nullptr || count == nullptr || nested == nullptr ||
        !IsReadableRange(wrapperAddress, kListProbeEnd)) {
        return false;
    }

    if (TryReadDirectList(wrapperAddress, data, count)) {
        *nested = false;
        return true;
    }

    // NetList/IList wrappers keep their concrete List<T> in a managed object
    // field. Scan only aligned pointer fields in the object header region;
    // every candidate is passed through TryReadDirectList, so an arbitrary
    // value such as the old 0x62006e0078 cannot be dereferenced.
    for (std::uintptr_t offset = 0x10; offset < kListProbeEnd; offset += 0x8) {
        const auto candidate = *reinterpret_cast<const std::uintptr_t*>(
            wrapperAddress + offset);
        if (!IsLikelyManagedPointer(candidate) || candidate == wrapperAddress ||
            !IsReadableRange(candidate, kListProbeEnd)) {
            continue;
        }
        if (TryReadDirectList(candidate, data, count)) {
            *nested = true;
            return true;
        }
    }
    return false;
}

// Some generated wrappers return a List<T>, while others return a NetField
// whose Value is that List<T>. Prefer the direct List view, then use Value only
// as a fallback; this avoids reinterpreting valid direct getters unnecessarily.
bool TryReadListOrNetField(
    void* listOrNetField, const void** data, std::uint32_t* count,
    bool* usedNetFieldValue) noexcept {
    if (listOrNetField == nullptr || data == nullptr || count == nullptr ||
        usedNetFieldValue == nullptr) {
        return false;
    }

    const auto address = reinterpret_cast<std::uintptr_t>(listOrNetField);
    if (!IsReadableRange(address, kNetFieldValue + sizeof(std::uintptr_t))) {
        return false;
    }
    bool nested = false;
    if (TryReadNestedList(address, data, count, &nested)) {
        *usedNetFieldValue = false;
        return true;
    }

    // Game1.locations uses a confirmed NetField.Value fallback. Keep it for
    // that generic getter, but guard both the field and the nested list. Chest
    // inventory never reaches this path.
    const auto value = *reinterpret_cast<void* const*>(address + kNetFieldValue);
    if (!IsLikelyManagedPointer(reinterpret_cast<std::uintptr_t>(value)) ||
        value == listOrNetField ||
        !TryReadNestedList(reinterpret_cast<std::uintptr_t>(value), data, count,
                           &nested)) {
        return false;
    }

    *usedNetFieldValue = true;
    return true;
}

bool SetNetFieldValue(void* field, std::uintptr_t value) noexcept {
    if (field == nullptr) {
        return false;
    }

    const auto fieldAddress = reinterpret_cast<std::uintptr_t>(field);
    const auto vtable = *reinterpret_cast<const std::uintptr_t*>(fieldAddress + kObjectVtable);
    if (vtable == 0) {
        return false;
    }
    const auto methods = *reinterpret_cast<const std::uintptr_t*>(vtable + sizeof(std::uintptr_t));
    if (methods == 0) {
        return false;
    }

    const auto fnAddress = *reinterpret_cast<const std::uintptr_t*>(methods + kNetFieldSetter);
    const auto thisAdjustment = *reinterpret_cast<const std::uint8_t*>(methods + kNetFieldThisAdjustment);
    if (fnAddress == 0) {
        return false;
    }

    const auto fn = reinterpret_cast<NetFieldSetFn>(fnAddress);
    fn(reinterpret_cast<void*>(fieldAddress + thisAdjustment), value);
    return true;
}

} // namespace

Item* ItemCollectionView::Get(std::uint32_t index) const noexcept {
    return readable && list != nullptr && index < count
        ? TryGetIListItem(list, index)
        : nullptr;
}

TerrainFeature* TerrainFeatureCollectionView::Get(
    std::uint32_t index) const noexcept {
    if (!readable || owner == nullptr || fields == nullptr || index >= count ||
        fields[index] == nullptr || unwrapFunction == 0) {
        return nullptr;
    }
    const auto unwrap = reinterpret_cast<TerrainFeatureUnwrapFn>(
        unwrapFunction);
    return unwrap(
        reinterpret_cast<void*>(
            reinterpret_cast<std::uintptr_t>(owner) + unwrapThisAdjustment),
        fields[index]);
}

bool FlooringView::IsFlooring() const noexcept {
    if (m_Feature == nullptr) {
        return false;
    }

    // Flooring::.ctor allocates `this` with the metadata held by this exact
    // game-owned rooted storage (main+0x01A536D8..0x01A53740). Read it on every
    // probe instead of copying the value into native BSS. This preserves the
    // same level of indirection as the original AOT code and avoids retaining
    // an untracked runtime pointer across a compacting GC.
    auto** const storage = *reinterpret_cast<void***>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::FlooringMetadataStorageSlot));
    if (storage == nullptr || *storage == nullptr) {
        return false;
    }
    void* const flooringMetadata = *storage;
    const auto isInstance = reinterpret_cast<IsInstanceFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::IsInstanceOfType));
    return isInstance(m_Feature, flooringMetadata) != nullptr;
}

bool FlooringView::CopyItemId(
    char* output, std::uint32_t capacity) const noexcept {
    if (output == nullptr || capacity == 0) {
        return false;
    }
    output[0] = '\0';
    if (!IsFlooring()) {
        return false;
    }

    // Match PC Automate's `floor.GetData()?.ItemId` connector test. The
    // whichFloor value is a FloorsAndPaths data key, not necessarily the
    // placed item's ID, so comparing it to a legacy key such as "0" is not a
    // stable Wood Path test on 1.6 data.
    const auto getData = reinterpret_cast<GetFlooringDataFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::FlooringGetDataBody));
    void* const data = getData(m_Feature);
    const auto dataAddress = reinterpret_cast<std::uintptr_t>(data);
    if (!IsReadableRange(
            dataAddress,
            StardewValley::Offsets::FlooringDataItemId +
                sizeof(std::uintptr_t))) {
        return false;
    }
    auto* const itemId = reinterpret_cast<void*>(
        *reinterpret_cast<const std::uintptr_t*>(
            dataAddress + StardewValley::Offsets::FlooringDataItemId));
    return CopyManagedStringAscii(itemId, output, capacity);
}

bool FlooringView::IsWoodPath() const noexcept {
    // Match PC Automate exactly after the owning NetVector2Dictionary has
    // unwrapped its raw NetRef<TerrainFeature> field: `feature is Flooring`,
    // then `floor.GetData()?.ItemId`. Do not synthesize "405" through
    // main+0x002025E0: that body is the instance Int32.ToString method and its
    // X0 is an Int32 address, not a numeric value.
    if (!IsFlooring()) {
        return false;
    }

    // Match PC Automate exactly: floor.GetData()?.ItemId, followed by the
    // connector item-ID test. GetData resolves whichFloor through the game's
    // current Data/FloorsAndPaths dictionary, so no internal key is cached.
    const auto getData = reinterpret_cast<GetFlooringDataFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::FlooringGetDataBody));
    void* const data = getData(m_Feature);
    const auto dataAddress = reinterpret_cast<std::uintptr_t>(data);
    if (!IsReadableRange(
            dataAddress,
            StardewValley::Offsets::FlooringDataItemId +
                sizeof(std::uintptr_t))) {
        return false;
    }
    void* const itemId = reinterpret_cast<void*>(
        *reinterpret_cast<const std::uintptr_t*>(
            dataAddress + StardewValley::Offsets::FlooringDataItemId));
    if (itemId == nullptr) {
        return false;
    }

    // FloorsAndPathsData.ItemId is a managed String. Compare its UTF-16
    // contents in-place while the GetData result is live; don't allocate or
    // cache a managed comparison String in untracked native storage.
    return ManagedStringEqualsAscii(itemId, "405") ||
        ManagedStringEqualsAscii(itemId, "(O)405");
}

bool FishPondView::IsFishPond() const noexcept {
    VirtualMethod method{};
    return m_Building != nullptr &&
        TryResolveVirtualMethod(
            m_Building,
            StardewValley::Offsets::BuildingDoActionVirtualSlot,
            StardewValley::Offsets::BuildingDoActionThisAdjustment,
            &method) &&
        method.function == exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::FishPondDoActionBody);
}

bool FishPondView::IsUnderConstruction() const noexcept {
    std::int32_t days = 0;
    return !TryReadNetInt(
               m_Building,
               StardewValley::Offsets::BuildingDaysOfConstructionLeftRef,
               &days) ||
        days > 0;
}

BuildingTileArea FishPondView::ReadTileArea() const noexcept {
    BuildingTileArea area{};
    if (!IsFishPond() ||
        !TryReadNetInt(m_Building, StardewValley::Offsets::BuildingTileXRef,
                       &area.x) ||
        !TryReadNetInt(m_Building, StardewValley::Offsets::BuildingTileYRef,
                       &area.y) ||
        !TryReadNetInt(m_Building,
                       StardewValley::Offsets::BuildingTilesWideRef,
                       &area.width) ||
        !TryReadNetInt(m_Building,
                       StardewValley::Offsets::BuildingTilesHighRef,
                       &area.height)) {
        return area;
    }

    // Fish Ponds are 5x5 in vanilla.  Keep a wider but finite bound for map
    // variants and reject corrupt NetInts before they reach tile indexing.
    if (area.x < -32768 || area.x > 32768 ||
        area.y < -32768 || area.y > 32768 ||
        area.width <= 0 || area.width > 32 ||
        area.height <= 0 || area.height > 32) {
        return area;
    }
    area.readable = true;
    return area;
}

Item* FishPondView::ReadOutput() const noexcept {
    if (m_Building == nullptr || IsUnderConstruction()) {
        return nullptr;
    }
    const auto buildingAddress = reinterpret_cast<std::uintptr_t>(m_Building);
    if (!IsReadableRange(
            buildingAddress,
            StardewValley::Offsets::FishPondOutputRef +
                sizeof(std::uintptr_t))) {
        return nullptr;
    }
    const auto outputField = *reinterpret_cast<const std::uintptr_t*>(
        buildingAddress + StardewValley::Offsets::FishPondOutputRef);
    if (!IsReadableRange(
            outputField,
            StardewValley::Offsets::NetRefValue + sizeof(std::uintptr_t))) {
        return nullptr;
    }
    return reinterpret_cast<Item*>(
        *reinterpret_cast<const std::uintptr_t*>(
            outputField + StardewValley::Offsets::NetRefValue));
}

bool FishPondView::CompleteOutputTransfer(
    Item* outputSample, Farmer* who) const noexcept {
    if (m_Building == nullptr) {
        return false;
    }
    const auto buildingAddress = reinterpret_cast<std::uintptr_t>(m_Building);
    if (!IsReadableRange(
            buildingAddress,
            StardewValley::Offsets::FishPondOutputRef +
                sizeof(std::uintptr_t))) {
        return false;
    }
    auto* const outputField = *reinterpret_cast<void* const*>(
        buildingAddress + StardewValley::Offsets::FishPondOutputRef);
    if (outputField == nullptr || !SetNetFieldValue(outputField, 0)) {
        return false;
    }

    // PC Automate awards the pond owner the same harvest XP as a manual
    // collection.  The native PoC supplies Game1.player here; owner mapping is
    // deliberately not guessed.  Failure to resolve an optional XP component
    // must not restore an output which the Chest already owns.
    if (outputSample == nullptr || who == nullptr) {
        return true;
    }
    VirtualMethod priceMethod{};
    VirtualMethod experienceMethod{};
    if (!TryResolveVirtualMethod(
            outputSample,
            StardewValley::Offsets::ItemSellToStorePriceVirtualSlot,
            StardewValley::Offsets::ItemSellToStorePriceThisAdjustment,
            &priceMethod) ||
        !TryResolveVirtualMethod(
            who,
            StardewValley::Offsets::FarmerGainExperienceVirtualSlot,
            StardewValley::Offsets::FarmerGainExperienceThisAdjustment,
            &experienceMethod)) {
        return true;
    }

    const auto baseSlot = exl::util::modules::GetTargetOffset(
        StardewValley::Offsets::FishPondHarvestBaseExpStorageSlot);
    const auto multiplierSlot = exl::util::modules::GetTargetOffset(
        StardewValley::Offsets::FishPondHarvestOutputExpMultiplierStorageSlot);
    if (!IsReadableAddressRange(baseSlot, sizeof(std::uintptr_t)) ||
        !IsReadableAddressRange(multiplierSlot, sizeof(std::uintptr_t))) {
        return true;
    }
    const auto baseStorage = *reinterpret_cast<const std::uintptr_t*>(baseSlot);
    const auto multiplierStorage =
        *reinterpret_cast<const std::uintptr_t*>(multiplierSlot);
    if (!IsReadableAddressRange(baseStorage, sizeof(std::int32_t)) ||
        !IsReadableAddressRange(multiplierStorage, sizeof(float))) {
        return true;
    }

    const auto sellToStorePrice = reinterpret_cast<SellToStorePriceFn>(
        priceMethod.function);
    const auto price = sellToStorePrice(
        reinterpret_cast<Item*>(
            reinterpret_cast<std::uintptr_t>(outputSample) +
            priceMethod.thisAdjustment),
        -1);
    const auto multiplier = *reinterpret_cast<const float*>(multiplierStorage);
    const auto baseExperience =
        *reinterpret_cast<const std::int32_t*>(baseStorage);
    const auto outputExperience =
        static_cast<std::int32_t>(static_cast<float>(price) * multiplier);
    const auto gainExperience = reinterpret_cast<GainExperienceFn>(
        experienceMethod.function);
    gainExperience(
        reinterpret_cast<Farmer*>(
            reinterpret_cast<std::uintptr_t>(who) +
            experienceMethod.thisAdjustment),
        1, baseExperience + outputExperience);
    return true;
}

bool ObjectView::HasItemId(const char* expected) const noexcept {
    if (m_Object == nullptr || expected == nullptr ||
        !StardewValley::Offsets::IsKnown(StardewValley::Offsets::ItemGetItemIdBody)) {
        return false;
    }

    using GetItemIdFn = void* (*)(Item* item);
    const auto fn = reinterpret_cast<GetItemIdFn>(
        exl::util::modules::GetTargetOffset(StardewValley::Offsets::ItemGetItemIdBody));
    return ManagedStringEqualsAscii(
        fn(reinterpret_cast<Item*>(m_Object)), expected);
}

std::int32_t ObjectView::ReadParentSheetIndex() const noexcept {
    if (m_Object == nullptr ||
        !StardewValley::Offsets::IsKnown(StardewValley::Offsets::ItemGetParentSheetIndexBody)) {
        return -1;
    }

    using GetParentSheetIndexFn = std::int32_t (*)(Item* item);
    const auto fn = reinterpret_cast<GetParentSheetIndexFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::ItemGetParentSheetIndexBody));
    return fn(reinterpret_cast<Item*>(m_Object));
}

ObjectOutputState ObjectView::ReadOutputState() const noexcept {
    ObjectOutputState state{};
    if (m_Object == nullptr) {
        return state;
    }

    const auto objectAddress = reinterpret_cast<std::uintptr_t>(m_Object);
    const auto heldRef = *reinterpret_cast<std::uintptr_t*>(
        objectAddress + StardewValley::Offsets::ObjectHeldObjectRef);
    if (heldRef != 0) {
        state.heldObject = reinterpret_cast<Item*>(
            *reinterpret_cast<std::uintptr_t*>(heldRef + StardewValley::Offsets::NetRefValue));
    }

    const auto minutesRef = *reinterpret_cast<std::uintptr_t*>(
        objectAddress + StardewValley::Offsets::ObjectMinutesUntilReadyRef);
    if (minutesRef != 0) {
        state.hasMinutesUntilReady = true;
        state.minutesUntilReady = *reinterpret_cast<std::int32_t*>(
            minutesRef + StardewValley::Offsets::NetRefValue);
    }

    const auto readyRef = *reinterpret_cast<std::uintptr_t*>(objectAddress + kObjectReadyForHarvestRef);
    if (readyRef != 0) {
        state.readyForHarvest =
            *reinterpret_cast<const std::uint8_t*>(readyRef + StardewValley::Offsets::NetRefValue) != 0;
    }

    return state;
}

bool ObjectView::CompleteOutputTransfer(
    Item* collectedItem, Farmer* who, GameLocation* location) const noexcept {
    (void)who;
    if (m_Object == nullptr || collectedItem == nullptr || location == nullptr) {
        return false;
    }

    const auto objectAddress = reinterpret_cast<std::uintptr_t>(m_Object);

    // Vanilla resolves MachineData before it mutates any Object NetFields and
    // carries that same pointer through the trigger-2 follow-up. The caller
    // made collectedItem with Item.getOne before Chest.addItem, so this sample
    // is independent of any merge/ownership changes in the destination.
    Item* const outputSample = collectedItem;
    const auto getMachineData = reinterpret_cast<GetMachineDataFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::ObjectGetMachineDataBody));
    void* const machineData = getMachineData(m_Object);

    const auto readyField = *reinterpret_cast<void* const*>(
        objectAddress + kObjectReadyForHarvestRef);
    const auto heldField = *reinterpret_cast<void* const*>(
        objectAddress + StardewValley::Offsets::ObjectHeldObjectRef);
    const auto showNextIndexField = *reinterpret_cast<void* const*>(
        objectAddress + StardewValley::Offsets::ObjectShowNextIndexRef);

    // Match the common data-based collection block at
    // main+0x0193789C..0x01937900.  A full Chest never reaches this function,
    // so the machine keeps its output until storage has accepted it.
    if (heldField != nullptr && !SetNetFieldValue(heldField, 0)) {
        return false;
    }
    if (readyField != nullptr && !SetNetFieldValue(readyField, 0)) {
        return false;
    }
    if (showNextIndexField != nullptr && !SetNetFieldValue(showNextIndexField, 0)) {
        return false;
    }

    const auto resetParentSheetIndex = reinterpret_cast<ResetParentSheetIndexFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::ItemResetParentSheetIndexBody));
    resetParentSheetIndex(reinterpret_cast<Item*>(m_Object));

    if (outputSample == nullptr || machineData == nullptr) {
        return true;
    }

    // This is the exact trigger-2 call shape used by vanilla at
    // main+0x01937944..0x0193795C.  PC Automate also passes a null Farmer for
    // this rule lookup/output generation; player experience is separate from
    // machine-state generation.
    void* outputRule = nullptr;
    void* outputId = nullptr;
    void* outputItem = nullptr;
    void* outputData = nullptr;
    const auto tryGetOutputRule = reinterpret_cast<TryGetMachineOutputRuleFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::TryGetMachineOutputRuleBody));
    if (tryGetOutputRule(
            m_Object, machineData, 2, outputSample, nullptr, location,
            &outputRule, &outputId, &outputItem, &outputData) &&
        outputRule != nullptr) {
        VirtualMethod outputMethod{};
        if (!TryResolveVirtualMethod(
                m_Object,
                StardewValley::Offsets::ObjectOutputMachineVirtualSlot,
                StardewValley::Offsets::ObjectOutputMachineThisAdjustment,
                &outputMethod)) {
            return false;
        }
        Item* lastInputItem = nullptr;
        const auto lastInputField = *reinterpret_cast<const std::uintptr_t*>(
            objectAddress + StardewValley::Offsets::ObjectLastInputItemRef);
        if (lastInputField != 0) {
            lastInputItem = reinterpret_cast<Item*>(
                *reinterpret_cast<const std::uintptr_t*>(
                    lastInputField + StardewValley::Offsets::NetRefValue));
        }
        const auto outputMachine = reinterpret_cast<OutputMachineVirtualFn>(
            outputMethod.function);
        outputMachine(
            reinterpret_cast<Object*>(objectAddress + outputMethod.thisAdjustment),
            machineData, outputRule, lastInputItem, nullptr, location,
            false, false);
    }

    // Vanilla refreshes the source Tree after collecting from both normal and
    // heavy tappers.  All pointers/offsets below come from the same collection
    // block at main+0x0193799C..0x01937A40.
    VirtualMethod tapperMethod{};
    if (TryResolveVirtualMethod(
            m_Object,
            StardewValley::Offsets::ObjectIsTapperVirtualSlot,
            StardewValley::Offsets::ObjectIsTapperThisAdjustment,
            &tapperMethod)) {
        const auto isTapper = reinterpret_cast<IsTapperVirtualFn>(
            tapperMethod.function);
        if (isTapper(reinterpret_cast<Object*>(
                objectAddress + tapperMethod.thisAdjustment))) {
            const auto tileField = *reinterpret_cast<const std::uintptr_t*>(
                objectAddress + StardewValley::Offsets::ObjectTileLocationRef);
            const auto terrainFeatures = *reinterpret_cast<const std::uintptr_t*>(
                reinterpret_cast<std::uintptr_t>(location) +
                StardewValley::Offsets::GameLocationTerrainFeaturesRef);
            if (tileField != 0 && terrainFeatures != 0) {
                const TilePosition tile = *reinterpret_cast<const TilePosition*>(
                    tileField + StardewValley::Offsets::NetRefValue);
                void* treeField = nullptr;
                void* const dictionary = *reinterpret_cast<void* const*>(
                    terrainFeatures +
                    StardewValley::Offsets::TerrainFeatureDictionaryValue);
                const auto tryGetTerrainFeature =
                    reinterpret_cast<TerrainFeatureTryGetValueFn>(
                        exl::util::modules::GetTargetOffset(
                            StardewValley::Offsets::TerrainFeatureTryGetValueBody));
                if (dictionary != nullptr &&
                    tryGetTerrainFeature(dictionary, tile, &treeField) &&
                    treeField != nullptr) {
                    auto* const tree = UnwrapTerrainFeatureField(
                        reinterpret_cast<void*>(terrainFeatures), treeField);
                    if (tree == nullptr) {
                        return true;
                    }
                    const auto updateTapper = reinterpret_cast<UpdateTapperProductFn>(
                        exl::util::modules::GetTargetOffset(
                            StardewValley::Offsets::TreeUpdateTapperProductBody));
                    updateTapper(tree, m_Object, outputSample, false);
                }
            }
        }
    }
    return true;
}

Farmer* GameState::GetPlayer() noexcept {
    const auto fn = reinterpret_cast<GetPlayerFn>(
        exl::util::modules::GetTargetOffset(StardewValley::Offsets::Game1GetPlayer));
    return fn();
}

GameLocation* GameState::GetCurrentLocation() noexcept {
    const auto fn = reinterpret_cast<GetCurrentLocationFn>(
        exl::util::modules::GetTargetOffset(StardewValley::Offsets::Game1GetCurrentLocation));
    return fn();
}

LocationCollectionView GameState::GetLoadedLocations() noexcept {
    LocationCollectionView result{};
    if (!StardewValley::Offsets::IsKnown(StardewValley::Offsets::Game1GetLocations)) {
        return result;
    }

    const auto getLocations = reinterpret_cast<GetLocationsFn>(
        exl::util::modules::GetTargetOffset(StardewValley::Offsets::Game1GetLocations));
    const auto listOrNetField = getLocations();
    const void* data = nullptr;
    bool usedNetFieldValue = false;
    if (!TryReadListOrNetField(listOrNetField, &data, &result.count, &usedNetFieldValue)) {
        return result;
    }

    result.locations = reinterpret_cast<GameLocation* const*>(data);
    result.readable = true;
    result.usedNetFieldValue = usedNetFieldValue;
    return result;
}

LocationCollectionView GameState::GetInstancedBuildingInteriors(
    GameLocation* location) noexcept {
    LocationCollectionView result{};
    if (location == nullptr ||
        !StardewValley::Offsets::IsKnown(
            StardewValley::Offsets::
                GameLocationGetInstancedBuildingInteriorsBody)) {
        return result;
    }

    const auto getInteriors =
        reinterpret_cast<GetInstancedBuildingInteriorsFn>(
            exl::util::modules::GetTargetOffset(
                StardewValley::Offsets::
                    GameLocationGetInstancedBuildingInteriorsBody));
    void* const collection = getInteriors(location);
    if (collection == nullptr) {
        return result;
    }

    const void* data = nullptr;
    bool usedNetFieldValue = false;
    if (TryReadListOrNetField(
            collection, &data, &result.count, &usedNetFieldValue)) {
        result.locations = reinterpret_cast<GameLocation* const*>(data);
        result.readable = true;
        result.usedNetFieldValue = usedNetFieldValue;
        return result;
    }

    // The vanilla method returns a static empty GameLocation[] when a root has
    // no instanced interiors. Accept a direct managed array as well as the
    // non-empty List<GameLocation> path above.
    if (TryReadArray(collection, &data, &result.count)) {
        result.locations = reinterpret_cast<GameLocation* const*>(data);
        result.readable = true;
    }
    return result;
}

ObjectCollectionView GameState::GetObjects(GameLocation* location) noexcept {
    ObjectCollectionView result{};
    if (location == nullptr) {
        return result;
    }

    const auto getObjects = reinterpret_cast<GetLocationObjectsFn>(
        exl::util::modules::GetTargetOffset(StardewValley::Offsets::GameLocationGetObjects));
    const auto netDictionary = getObjects(location);
    if (netDictionary == nullptr) {
        return result;
    }

    const auto dictionary = *reinterpret_cast<const std::uintptr_t*>(
        reinterpret_cast<std::uintptr_t>(netDictionary) + kNetDictionaryDictionary);
    if (dictionary == 0) {
        return result;
    }

    const auto keysArray = *reinterpret_cast<void* const*>(dictionary + kDictionaryKeys);
    const auto valuesArray = *reinterpret_cast<void* const*>(dictionary + kDictionaryValues);
    const auto entriesArray = *reinterpret_cast<void* const*>(dictionary + kDictionaryEntryMetadata);
    const void* keysData = nullptr;
    const void* valuesData = nullptr;
    const void* entriesData = nullptr;
    std::uint32_t keysCount = 0;
    std::uint32_t valuesCount = 0;
    std::uint32_t entriesCount = 0;
    if (!TryReadArray(const_cast<void*>(keysArray), &keysData, &keysCount) ||
        !TryReadArray(valuesArray, &valuesData, &valuesCount) ||
        keysCount != valuesCount) {
        return result;
    }

    // Metadata differs between AOT wrappers and is not used for liveness. It
    // remains exposed only for diagnostics if it has the same capacity.
    const bool hasEntries =
        TryReadArray(entriesArray, &entriesData, &entriesCount) &&
        entriesCount == valuesCount;

    result.keys = reinterpret_cast<const TilePosition*>(keysData);
    result.values = reinterpret_cast<Object* const*>(valuesData);
    result.entryMetadata = hasEntries
        ? reinterpret_cast<const std::uint64_t*>(entriesData)
        : nullptr;
    result.count = valuesCount;
    result.readable = true;
    return result;
}

TerrainFeatureCollectionView GameState::GetTerrainFeatures(
    GameLocation* location) noexcept {
    TerrainFeatureCollectionView result{};
    if (location == nullptr) {
        return result;
    }

    const auto locationAddress = reinterpret_cast<std::uintptr_t>(location);
    if (!IsReadableRange(
            locationAddress,
            StardewValley::Offsets::GameLocationTerrainFeaturesRef +
                sizeof(std::uintptr_t))) {
        return result;
    }
    const auto terrainFeatures = *reinterpret_cast<const std::uintptr_t*>(
        locationAddress +
        StardewValley::Offsets::GameLocationTerrainFeaturesRef);
    if (!IsReadableRange(
            terrainFeatures,
            StardewValley::Offsets::TerrainFeatureDictionaryValue +
                sizeof(std::uintptr_t))) {
        return result;
    }
    const auto dictionary = *reinterpret_cast<const std::uintptr_t*>(
        terrainFeatures +
        StardewValley::Offsets::TerrainFeatureDictionaryValue);
    if (!IsReadableRange(
            dictionary, kDictionaryValues + sizeof(std::uintptr_t))) {
        return result;
    }

    const auto keysArray = *reinterpret_cast<void* const*>(
        dictionary + kDictionaryKeys);
    const auto valuesArray = *reinterpret_cast<void* const*>(
        dictionary + kDictionaryValues);
    const void* keysData = nullptr;
    const void* valuesData = nullptr;
    std::uint32_t keysCount = 0;
    std::uint32_t valuesCount = 0;
    if (!TryReadArray(const_cast<void*>(keysArray), &keysData, &keysCount) ||
        !TryReadArray(valuesArray, &valuesData, &valuesCount) ||
        keysCount != valuesCount) {
        return result;
    }

    // The unwrap virtual method belongs to the collection owner and is
    // identical for every value in this snapshot. Resolve and validate it once
    // per location scan instead of issuing three svcQueryMemory calls for every
    // terrain feature.
    VirtualMethod unwrapMethod{};
    if (!TryResolveVirtualMethod(
            reinterpret_cast<void*>(terrainFeatures),
            StardewValley::Offsets::TerrainFeatureUnwrapVirtualSlot,
            StardewValley::Offsets::TerrainFeatureUnwrapThisAdjustment,
            &unwrapMethod)) {
        return result;
    }

    result.owner = reinterpret_cast<void*>(terrainFeatures);
    result.keys = reinterpret_cast<const TilePosition*>(keysData);
    result.fields = reinterpret_cast<void* const*>(valuesData);
    result.unwrapFunction = unwrapMethod.function;
    result.unwrapThisAdjustment = unwrapMethod.thisAdjustment;
    result.count = valuesCount;
    result.readable = true;
    return result;
}

TerrainFeature* GameState::GetTerrainFeatureAt(
    GameLocation* location, TilePosition tile, bool* readable) noexcept {
    if (readable != nullptr) {
        *readable = false;
    }
    if (location == nullptr) {
        return nullptr;
    }

    const auto locationAddress = reinterpret_cast<std::uintptr_t>(location);
    if (!IsReadableRange(
            locationAddress,
            StardewValley::Offsets::GameLocationTerrainFeaturesRef +
                sizeof(std::uintptr_t))) {
        return nullptr;
    }
    const auto terrainFeatures = *reinterpret_cast<const std::uintptr_t*>(
        locationAddress +
        StardewValley::Offsets::GameLocationTerrainFeaturesRef);
    if (!IsReadableRange(
            terrainFeatures,
            StardewValley::Offsets::TerrainFeatureDictionaryValue +
                sizeof(std::uintptr_t))) {
        return nullptr;
    }
    void* const dictionary = *reinterpret_cast<void* const*>(
        terrainFeatures +
        StardewValley::Offsets::TerrainFeatureDictionaryValue);
    if (!IsReadableRange(
            reinterpret_cast<std::uintptr_t>(dictionary),
            kDictionaryValues + sizeof(std::uintptr_t))) {
        return nullptr;
    }

    if (readable != nullptr) {
        *readable = true;
    }
    void* field = nullptr;
    const auto tryGet = reinterpret_cast<TerrainFeatureTryGetValueFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::TerrainFeatureTryGetValueBody));
    if (!tryGet(dictionary, tile, &field) || field == nullptr) {
        return nullptr;
    }
    return UnwrapTerrainFeatureField(
        reinterpret_cast<void*>(terrainFeatures), field);
}

BuildingCollectionView GameState::GetBuildings(
    GameLocation* location) noexcept {
    BuildingCollectionView result{};
    if (location == nullptr) {
        return result;
    }

    const auto locationAddress = reinterpret_cast<std::uintptr_t>(location);
    if (!IsReadableRange(
            locationAddress,
            StardewValley::Offsets::GameLocationBuildingsRef +
                sizeof(std::uintptr_t))) {
        return result;
    }
    const auto collection = *reinterpret_cast<const std::uintptr_t*>(
        locationAddress + StardewValley::Offsets::GameLocationBuildingsRef);
    if (!IsReadableRange(
            collection,
            StardewValley::Offsets::NetCollectionItemsRef +
                sizeof(std::uintptr_t))) {
        return result;
    }

    // GameLocation.GetInstancedBuildingInteriors at main+0x0149FFB0 loads
    // collection+0x48 and immediately constructs a List<Building> enumerator
    // from it (including the List version at +0x1C).  Follow that exact list
    // rather than applying the different NetCollection layout used by rings.
    const auto list = *reinterpret_cast<const std::uintptr_t*>(
        collection + StardewValley::Offsets::NetCollectionItemsRef);
    const void* data = nullptr;
    if (!TryReadDirectList(list, &data, &result.count)) {
        return result;
    }

    result.buildings = reinterpret_cast<Building* const*>(data);
    result.readable = true;
    return result;
}

bool ChestActions::IsAvailable() noexcept {
    return StardewValley::Offsets::IsKnown(StardewValley::Offsets::ChestAddItem) &&
           StardewValley::Offsets::IsKnown(
               StardewValley::Offsets::ChestGetItemsForPlayerBody);
}

Item* ChestActions::AddItem(Object* chest, Item* item) noexcept {
    if (!IsAvailable() || chest == nullptr || item == nullptr) {
        return item;
    }

    auto addItem = reinterpret_cast<AddItemFn>(
        exl::util::modules::GetTargetOffset(StardewValley::Offsets::ChestAddItem));
    Item* offered = item;
    return addItem(chest, &offered);
}

ItemCollectionView ChestActions::GetItems(Object* chest) noexcept {
    ItemCollectionView result{};
    if (chest == nullptr || !IsAvailable() ||
        !StardewValley::Offsets::IsKnown(StardewValley::Offsets::Game1GetPlayer)) {
        return result;
    }

    const auto getItems = reinterpret_cast<GetChestItemsFn>(
        exl::util::modules::GetTargetOffset(
            StardewValley::Offsets::ChestGetItemsForPlayerBody));
    // 0 is intentionally passed here: main+0x1960210 is the parameterless
    // wrapper observed in the target build and overwrites X1 with
    // Game1.player.UniqueMultiplayerID before its tail call.  A direct
    // overload must be re-proved before this value is changed.
    const auto listOrNetField = getItems(chest, 0);
    if (listOrNetField == nullptr) {
        return result;
    }
    result.getterReturnedList = true;

    // main+0x01961010 returns an IList<Item>* directly. Its own callers at
    // main+0x0196055C and main+0x01960598 resolve Count/get_Item through the
    // Brute interface dispatcher. Follow that exact path instead of probing
    // object fields: the old guessed layouts could report a non-empty Chest
    // as empty even when their range checks prevented a crash.
    if (!TryGetICollectionCount(listOrNetField, &result.count)) {
        return result;
    }

    result.list = listOrNetField;
    result.readable = true;
    result.usedNetFieldValue = false;
    return result;
}

bool MachineActions::IsInputAvailable() noexcept {
    return StardewValley::Offsets::IsKnown(
               StardewValley::Offsets::ObjectAttemptAutoLoadVirtualSlot) &&
           StardewValley::Offsets::IsKnown(
               StardewValley::Offsets::ObjectAttemptAutoLoadThisAdjustment);
}

Item* MachineActions::CloneOne(Item* item) noexcept {
    if (item == nullptr ||
        !StardewValley::Offsets::IsKnown(StardewValley::Offsets::ItemGetOneBody)) {
        return nullptr;
    }
    const auto getOne = reinterpret_cast<GetOneFn>(
        exl::util::modules::GetTargetOffset(StardewValley::Offsets::ItemGetOneBody));
    return getOne(item);
}

bool MachineActions::AttemptAutoLoad(
    Object* machine, void* inventory, Farmer* who) noexcept {
    if (!IsInputAvailable() || machine == nullptr ||
        inventory == nullptr || who == nullptr) {
        return false;
    }

    VirtualMethod method{};
    if (!TryResolveVirtualMethod(
            machine,
            StardewValley::Offsets::ObjectAttemptAutoLoadVirtualSlot,
            StardewValley::Offsets::ObjectAttemptAutoLoadThisAdjustment,
            &method)) {
        return false;
    }
    const auto attemptAutoLoad = reinterpret_cast<AttemptAutoLoadVirtualFn>(
        method.function);
    return attemptAutoLoad(
        reinterpret_cast<Object*>(
            reinterpret_cast<std::uintptr_t>(machine) + method.thisAdjustment),
        inventory, who);
}

} // namespace AutomateLite::Game
