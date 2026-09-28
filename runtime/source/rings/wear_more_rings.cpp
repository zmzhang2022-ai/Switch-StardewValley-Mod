/* Copyright (C) 2026 zmzhang2022-ai | GPL-2.0-only | https://github.com/zmzhang2022-ai/Switch-StardewValley-Mod */
#include "rings/wear_more_rings.hpp"

#include "game/offsets.hpp"
#include "lib.hpp"

#include <array>

namespace AutomateLite::Rings {
namespace {

using namespace StardewValley;

struct Rectangle final {
    std::int32_t x{};
    std::int32_t y{};
    std::int32_t width{};
    std::int32_t height{};
};

struct NullableRectangle final {
    Rectangle value{};
    std::uint8_t hasValue{};
    std::uint8_t padding[7]{};
};

static_assert(sizeof(Rectangle) == 0x10);
static_assert(sizeof(NullableRectangle) == 0x18);

using ClickableConstructorFn = void* (*)(void*, Rectangle, void*);
using ListAddFn = void (*)(void*, void*);
using GetPlayerFn = void* (*)();
using GetCursorFn = void* (*)(void*);
using NormalizeItemFn = void* (*)(void*);
using SetHeldItemFn = void (*)(void*, void*);
using FarmerEquipFn = void* (*)(void*, void*, void*);
using IsInstanceFn = void* (*)(void*, void*);
using TypeInitEnterFn = bool (*)(void*);
using TypeInitExitFn = void (*)(void*);
using TypeFactoryFn = void* (*)();
using CombinedRingConstructorFn = void* (*)(void*);
using CollectionElementAtFn = void* (*)(void*, std::int32_t);
using CollectionAddFn = void (*)(void*, void*);
using GetSourceRectangleFn = Rectangle (*)(
    void*, std::int32_t, std::int32_t, std::int32_t);
using GetColorFn = std::uint32_t (*)();
using SpriteBatchDrawFn = void (*)(
    void*, void*, Rectangle, const NullableRectangle*, std::uint32_t);
using ItemDrawFn = void (*)(void*, void*, float, float, float);
using ItemStringFn = void* (*)(void*);

constexpr std::uintptr_t kListItems = 0x10;
constexpr std::uintptr_t kListSize = 0x18;
constexpr std::uintptr_t kArrayValues = 0x10;
constexpr std::uint32_t kMaximumEquipmentComponents = 32;
constexpr std::uint32_t kRightSlotCount = 3;
constexpr std::int32_t kRingCellSize = 64;

void* g_Page{};
std::array<void*, WearMoreRings::SlotCount> g_Components{};
std::array<void*, WearMoreRings::SlotCount> g_NormalNames{};
bool g_NamesHidden{};
bool g_LoggedOverflow{};

std::uintptr_t Target(std::uintptr_t offset) noexcept {
    return exl::util::modules::GetTargetOffset(offset);
}

void* ResolveRootedObject(std::uintptr_t slot) noexcept {
    const auto storage = *reinterpret_cast<void***>(Target(slot));
    return storage == nullptr ? nullptr : *storage;
}

template <typename T>
T* Field(void* object, std::uintptr_t offset) noexcept {
    return object == nullptr
        ? nullptr
        : reinterpret_cast<T*>(reinterpret_cast<std::uintptr_t>(object) + offset);
}

void* ListItem(void* list, std::uint32_t index) noexcept {
    if (list == nullptr || index >= kMaximumEquipmentComponents) {
        return nullptr;
    }
    const auto size = *Field<std::uint32_t>(list, kListSize);
    auto* const array = *Field<void*>(list, kListItems);
    if (index >= size || array == nullptr) {
        return nullptr;
    }
    auto** const values = *Field<void**>(array, kArrayValues);
    return values == nullptr ? nullptr : values[index];
}

void* FindComponentById(void* list, std::int32_t id) noexcept {
    if (list == nullptr || id < 0) {
        return nullptr;
    }
    const auto size = *Field<std::uint32_t>(list, kListSize);
    const auto limit = size < kMaximumEquipmentComponents
        ? size
        : kMaximumEquipmentComponents;
    for (std::uint32_t index = 0; index < limit; ++index) {
        void* const component = ListItem(list, index);
        if (component != nullptr &&
            *Field<std::int32_t>(
                component, Offsets::ClickableComponentMyId) == id) {
            return component;
        }
    }
    return nullptr;
}

bool PointInside(void* component, std::int32_t x, std::int32_t y) noexcept {
    const auto* const bounds = Field<Rectangle>(
        component, Offsets::ClickableComponentBounds);
    if (bounds == nullptr || bounds->width <= 0 || bounds->height <= 0) {
        return false;
    }
    return x >= bounds->x && y >= bounds->y &&
           x < bounds->x + bounds->width &&
           y < bounds->y + bounds->height;
}

void SetNeighbor(
    void* component, std::uintptr_t offset, std::int32_t value) noexcept {
    if (component != nullptr) {
        *Field<std::int32_t>(component, offset) = value;
    }
}

void* GetInitializedType(
    std::uintptr_t flagSlot, std::uintptr_t storageSlot,
    std::uintptr_t factoryOffset) noexcept {
    auto* const flag = *reinterpret_cast<std::uint8_t**>(Target(flagSlot));
    auto** const storage = *reinterpret_cast<void***>(Target(storageSlot));
    if (storage == nullptr) {
        return nullptr;
    }
    if (*storage != nullptr) {
        return *storage;
    }

    const auto factory = reinterpret_cast<TypeFactoryFn>(Target(factoryOffset));
    if (flag == nullptr) {
        return factory();
    }

    const auto enter = reinterpret_cast<TypeInitEnterFn>(
        Target(Offsets::TypeInitEnter));
    const auto leave = reinterpret_cast<TypeInitExitFn>(
        Target(Offsets::TypeInitExit));
    if (*flag == 0 && enter(flag)) {
        *storage = factory();
        leave(flag);
    }
    return *storage;
}

bool IsRing(void* item) noexcept {
    if (item == nullptr) {
        return false;
    }
    void* const type = GetInitializedType(
        Offsets::RingMetadataInitFlagSlot,
        Offsets::RingMetadataStorageSlot,
        Offsets::RingMetadataFactory);
    if (type == nullptr) {
        return false;
    }
    const auto isInstance = reinterpret_cast<IsInstanceFn>(
        Target(Offsets::IsInstanceOfType));
    return isInstance(item, type) != nullptr;
}

bool IsCombinedRing(void* item) noexcept {
    if (item == nullptr) {
        return false;
    }
    void* const type = GetInitializedType(
        Offsets::CombinedRingMetadataInitFlagSlot,
        Offsets::CombinedRingMetadataStorageSlot,
        Offsets::CombinedRingMetadataFactory);
    if (type == nullptr) {
        return false;
    }
    const auto isInstance = reinterpret_cast<IsInstanceFn>(
        Target(Offsets::IsInstanceOfType));
    return isInstance(item, type) != nullptr;
}

void* GetNetRefValue(void* field) noexcept {
    return field == nullptr
        ? nullptr
        : *Field<void*>(field, Offsets::NetRefValue);
}

void* GetFarmerEquipmentRef(
    void* farmer, std::uintptr_t fieldOffset) noexcept {
    // Farmer.leftRing/rightRing are managed reference fields which contain a
    // NetRef pointer.  main+0x01704680..0x017046AC does ADD farmer, offset and
    // then LDR [field] before passing the result to Farmer.Equip.  The address
    // of the field is not itself a NetRef.
    auto** const field = Field<void*>(farmer, fieldOffset);
    return field == nullptr ? nullptr : *field;
}

std::uint32_t ReadCombinedChildren(
    void* combinedRing, std::array<void*, kRightSlotCount>& children,
    bool* overflow) noexcept {
    children.fill(nullptr);
    if (overflow != nullptr) {
        *overflow = false;
    }
    if (!IsCombinedRing(combinedRing)) {
        if (combinedRing != nullptr) {
            children[0] = combinedRing;
            return 1;
        }
        return 0;
    }

    void* const collection = *Field<void*>(
        combinedRing, Offsets::CombinedRingChildren);
    if (collection == nullptr) {
        return 0;
    }
    void* const countRef = *Field<void*>(
        collection, Offsets::NetCollectionCountRef);
    if (countRef == nullptr) {
        return 0;
    }
    const auto rawCount = *Field<std::int32_t>(countRef, Offsets::NetRefValue);
    if (rawCount < 0 || rawCount > 20) {
        if (overflow != nullptr) {
            *overflow = true;
        }
        return 0;
    }
    if (rawCount > static_cast<std::int32_t>(kRightSlotCount)) {
        if (overflow != nullptr) {
            *overflow = true;
        }
        return kRightSlotCount;
    }

    void* const itemsRef = *Field<void*>(
        collection, Offsets::NetCollectionItemsRef);
    void* const itemsValue = GetNetRefValue(itemsRef);
    void* const elementList = itemsValue == nullptr
        ? nullptr
        : *Field<void*>(itemsValue, Offsets::NetCollectionItemsRef);
    if (elementList == nullptr) {
        return 0;
    }

    const auto elementAt = reinterpret_cast<CollectionElementAtFn>(
        Target(Offsets::NetCollectionElementAtBody));
    for (std::int32_t index = 0; index < rawCount; ++index) {
        void* const childRef = elementAt(elementList, index);
        children[static_cast<std::uint32_t>(index)] =
            GetNetRefValue(childRef);
    }
    return static_cast<std::uint32_t>(rawCount);
}

bool AddCombinedChild(void* collection, void* ring) noexcept {
    if (collection == nullptr || ring == nullptr) {
        return false;
    }
    const auto vtable = *reinterpret_cast<std::uintptr_t*>(collection);
    if (vtable == 0) {
        return false;
    }
    const auto methods = *reinterpret_cast<std::uintptr_t*>(
        vtable + sizeof(std::uintptr_t));
    if (methods == 0) {
        return false;
    }
    const auto function = *reinterpret_cast<std::uintptr_t*>(
        methods + Offsets::NetCollectionAddSlot);
    const auto adjustment = *reinterpret_cast<std::uint8_t*>(
        methods + Offsets::NetCollectionAddAdjustment);
    if (function == 0) {
        return false;
    }
    reinterpret_cast<CollectionAddFn>(function)(
        reinterpret_cast<void*>(
            reinterpret_cast<std::uintptr_t>(collection) + adjustment),
        ring);
    return true;
}

void* BuildRightEquipment(
    const std::array<void*, kRightSlotCount>& children,
    std::uint32_t count) noexcept {
    if (count == 0) {
        return nullptr;
    }
    if (count == 1) {
        return children[0];
    }
    if (count > kRightSlotCount) {
        return nullptr;
    }

    const auto construct = reinterpret_cast<CombinedRingConstructorFn>(
        Target(Offsets::CombinedRingConstructor));
    void* const combined = construct(nullptr);
    void* const collection = combined == nullptr
        ? nullptr
        : *Field<void*>(combined, Offsets::CombinedRingChildren);
    if (collection == nullptr) {
        return nullptr;
    }
    for (std::uint32_t index = 0; index < count; ++index) {
        if (!AddCombinedChild(collection, children[index])) {
            return nullptr;
        }
    }
    return combined;
}

bool GetVisibleItems(
    std::array<void*, WearMoreRings::SlotCount>& items,
    bool* overflow = nullptr) noexcept {
    items.fill(nullptr);
    if (overflow != nullptr) {
        *overflow = false;
    }
    const auto getPlayer = reinterpret_cast<GetPlayerFn>(
        Target(Offsets::Game1GetPlayer));
    void* const farmer = getPlayer();
    if (farmer == nullptr) {
        return false;
    }
    void* const leftRef = GetFarmerEquipmentRef(
        farmer, Offsets::FarmerLeftRingRef);
    void* const rightRef = GetFarmerEquipmentRef(
        farmer, Offsets::FarmerRightRingRef);
    if (leftRef == nullptr || rightRef == nullptr) {
        return false;
    }
    items[0] = GetNetRefValue(leftRef);

    std::array<void*, kRightSlotCount> right{};
    bool rightOverflow = false;
    const auto count = ReadCombinedChildren(
        GetNetRefValue(rightRef), right, &rightOverflow);
    for (std::uint32_t index = 0; index < count && index < right.size(); ++index) {
        items[index + 1] = right[index];
    }
    if (overflow != nullptr) {
        *overflow = rightOverflow;
    }
    return true;
}

void SetComponentItem(std::uint32_t slot, void* item) noexcept {
    if (slot < g_Components.size() && g_Components[slot] != nullptr) {
        *Field<void*>(
            g_Components[slot], Offsets::ClickableComponentItem) = item;
    }
}

void* CallItemString(
    void* item, std::uintptr_t slot, std::uintptr_t adjustmentSlot) noexcept {
    if (item == nullptr) {
        return nullptr;
    }
    const auto vtable = *reinterpret_cast<std::uintptr_t*>(item);
    const auto methods = vtable == 0
        ? 0
        : *reinterpret_cast<std::uintptr_t*>(vtable + sizeof(std::uintptr_t));
    if (methods == 0) {
        return nullptr;
    }
    const auto function = *reinterpret_cast<std::uintptr_t*>(methods + slot);
    const auto adjustment = *reinterpret_cast<std::uint8_t*>(
        methods + adjustmentSlot);
    return function == 0
        ? nullptr
        : reinterpret_cast<ItemStringFn>(function)(
              reinterpret_cast<void*>(
                  reinterpret_cast<std::uintptr_t>(item) + adjustment));
}

void DrawCell(void* component, void* item, void* spriteBatch) noexcept {
    if (component == nullptr || spriteBatch == nullptr) {
        return;
    }
    void* const texture = ResolveRootedObject(
        Offsets::Game1MenuTextureStorageSlot);
    if (texture == nullptr) {
        return;
    }

    const auto sourceRectangle = reinterpret_cast<GetSourceRectangleFn>(
        Target(Offsets::Game1GetSourceRectForStandardTileSheet));
    const auto getWhite = reinterpret_cast<GetColorFn>(
        Target(Offsets::ColorGetWhite));
    const auto drawRectangle = reinterpret_cast<SpriteBatchDrawFn>(
        Target(Offsets::SpriteBatchDrawRectangle));
    const auto* const bounds = Field<Rectangle>(
        component, Offsets::ClickableComponentBounds);
    NullableRectangle source{};
    source.value = sourceRectangle(
        texture, item == nullptr ? 41 : 10, -1, -1);
    source.hasValue = 1;
    drawRectangle(spriteBatch, texture, *bounds, &source, getWhite());

    if (item != nullptr) {
        const auto drawItem = reinterpret_cast<ItemDrawFn>(
            Target(Offsets::ItemDrawInMenuSimple));
        const float scale = *Field<float>(
            component, Offsets::ClickableComponentScale);
        drawItem(
            item, spriteBatch, static_cast<float>(bounds->x),
            static_cast<float>(bounds->y), scale);
    }
}

} // namespace

bool WearMoreRings::AttachInventoryPage(void* page) noexcept {
    g_Page = nullptr;
    g_Components.fill(nullptr);
    g_NormalNames.fill(nullptr);
    g_NamesHidden = false;
    if (page == nullptr) {
        return false;
    }

    void* const list = *Field<void*>(page, Offsets::InventoryPageEquipmentIcons);
    if (list == nullptr || *Field<std::uint32_t>(list, kListSize) < 6) {
        return false;
    }

    // InventoryPage's constructor at main+0x01703210..0x017036DC proves the
    // vanilla equipment IDs and coordinates.  The two original ring cells are
    // ID 102 and 103 in the *same left column*, followed by boots ID 104.
    // Hat/shirt/pants are 101/108/109 in the center-right column.  The 1.6
    // trinket (combat-pet) loop at main+0x01703748 creates ID 120 at the first
    // cell of the far-right column.  Never infer these controls by list index:
    // optional trinket slots change the list length.
    void* const left = FindComponentById(list, 102);
    void* const right = FindComponentById(list, 103);
    void* const boots = FindComponentById(list, 104);
    void* const hat = FindComponentById(list, 101);
    void* const shirt = FindComponentById(list, 108);
    void* const pants = FindComponentById(list, 109);
    void* const trinket = FindComponentById(list, 120);
    auto* const leftBounds = Field<Rectangle>(
        left, Offsets::ClickableComponentBounds);
    auto* const rightBounds = Field<Rectangle>(
        right, Offsets::ClickableComponentBounds);
    auto* const bootsBounds = Field<Rectangle>(
        boots, Offsets::ClickableComponentBounds);
    auto* const hatBounds = Field<Rectangle>(
        hat, Offsets::ClickableComponentBounds);
    auto* const shirtBounds = Field<Rectangle>(
        shirt, Offsets::ClickableComponentBounds);
    auto* const pantsBounds = Field<Rectangle>(
        pants, Offsets::ClickableComponentBounds);
    auto* const trinketBounds = Field<Rectangle>(
        trinket, Offsets::ClickableComponentBounds);
    if (left == nullptr || right == nullptr || boots == nullptr ||
        hat == nullptr || shirt == nullptr || pants == nullptr ||
        leftBounds == nullptr || rightBounds == nullptr ||
        bootsBounds == nullptr || hatBounds == nullptr ||
        shirtBounds == nullptr || pantsBounds == nullptr) {
        return false;
    }

    // Requested layout 2:
    //   left column:      ring 102, new ring 200, boots 104
    //   native column:    hat 101, shirt 108, pants 109
    //   far-right column: moved ring 103, new ring 201, trinket 120
    //
    // New ring 200 occupies the original 103 rectangle.  The moved 103 uses
    // the trinket's original rectangle (or the constructor-proven +0x118 X
    // delta when the trinket slot is locked); 201 is one cell below it.  The
    // trinket itself moves down two cells.
    const Rectangle leftOriginal = *leftBounds;
    const Rectangle rightOriginal = *rightBounds;
    const Rectangle farTop = trinketBounds != nullptr
        ? *trinketBounds
        : Rectangle{
              leftOriginal.x + 0x118,
              leftOriginal.y,
              kRingCellSize,
              kRingCellSize};
    const std::int32_t originalRightLeft = *Field<std::int32_t>(
        right, Offsets::ClickableComponentLeftNeighborId);
    const std::int32_t farOuterRight = trinket != nullptr
        ? *Field<std::int32_t>(
              trinket, Offsets::ClickableComponentRightNeighborId)
        : -1;
    const std::int32_t farOuterUp = trinket != nullptr
        ? *Field<std::int32_t>(
              trinket, Offsets::ClickableComponentUpNeighborId)
        : *Field<std::int32_t>(
              left, Offsets::ClickableComponentUpNeighborId);
    const std::int32_t farOuterDown = trinket != nullptr
        ? *Field<std::int32_t>(
              trinket, Offsets::ClickableComponentDownNeighborId)
        : -1;

    *rightBounds = farTop;
    if (trinketBounds != nullptr) {
        trinketBounds->x = farTop.x;
        trinketBounds->y = farTop.y + 2 * kRingCellSize;
    }

    g_Components[0] = left;
    g_Components[1] = right;
    g_NormalNames[0] = *Field<void*>(left, Offsets::ClickableComponentName);
    g_NormalNames[1] = *Field<void*>(right, Offsets::ClickableComponentName);

    void* const rightName = ResolveRootedObject(Offsets::StringSlotRightRing);
    const auto construct = reinterpret_cast<ClickableConstructorFn>(
        Target(Offsets::ClickableComponentConstructor));
    const auto add = reinterpret_cast<ListAddFn>(
        Target(Offsets::ClickableComponentListAdd));
    for (std::uint32_t slot = 2; slot < SlotCount; ++slot) {
        const Rectangle bounds = slot == 2
            ? rightOriginal
            : Rectangle{
                  farTop.x,
                  farTop.y + kRingCellSize,
                  kRingCellSize,
                  kRingCellSize};
        void* const component = construct(
            nullptr, bounds, rightName);
        if (component == nullptr) {
            return false;
        }
        *Field<std::uint8_t>(
            component, Offsets::ClickableComponentFullyImmutable) = 1;
        *Field<std::int32_t>(
            component, Offsets::ClickableComponentMyId) =
            static_cast<std::int32_t>(198 + slot);
        add(list, component);
        g_Components[slot] = component;
        g_NormalNames[slot] = rightName;
    }

    // Rebuild controller navigation from the constructor-proven component IDs
    // so every row crosses left -> native -> far-right and every column moves
    // vertically without skipping a newly inserted ring.
    SetNeighbor(left, Offsets::ClickableComponentRightNeighborId, 101);
    SetNeighbor(left, Offsets::ClickableComponentDownNeighborId, 200);

    SetNeighbor(right, Offsets::ClickableComponentLeftNeighborId, 101);
    SetNeighbor(right, Offsets::ClickableComponentRightNeighborId, farOuterRight);
    SetNeighbor(right, Offsets::ClickableComponentUpNeighborId, farOuterUp);
    SetNeighbor(right, Offsets::ClickableComponentDownNeighborId, 201);

    SetNeighbor(
        g_Components[2], Offsets::ClickableComponentLeftNeighborId,
        originalRightLeft);
    SetNeighbor(g_Components[2], Offsets::ClickableComponentUpNeighborId, 102);
    SetNeighbor(g_Components[2], Offsets::ClickableComponentRightNeighborId, 108);
    SetNeighbor(g_Components[2], Offsets::ClickableComponentDownNeighborId, 104);

    SetNeighbor(g_Components[3], Offsets::ClickableComponentUpNeighborId, 103);
    SetNeighbor(g_Components[3], Offsets::ClickableComponentLeftNeighborId, 108);
    SetNeighbor(
        g_Components[3], Offsets::ClickableComponentRightNeighborId,
        farOuterRight);
    SetNeighbor(
        g_Components[3], Offsets::ClickableComponentDownNeighborId,
        trinket != nullptr ? 120 : farOuterDown);

    SetNeighbor(boots, Offsets::ClickableComponentUpNeighborId, 200);
    SetNeighbor(boots, Offsets::ClickableComponentRightNeighborId, 109);
    SetNeighbor(hat, Offsets::ClickableComponentLeftNeighborId, 102);
    SetNeighbor(hat, Offsets::ClickableComponentRightNeighborId, 103);
    SetNeighbor(shirt, Offsets::ClickableComponentLeftNeighborId, 200);
    SetNeighbor(shirt, Offsets::ClickableComponentRightNeighborId, 201);
    SetNeighbor(pants, Offsets::ClickableComponentLeftNeighborId, 104);
    SetNeighbor(
        pants, Offsets::ClickableComponentRightNeighborId,
        trinket != nullptr ? 120 : 201);
    if (trinket != nullptr) {
        SetNeighbor(trinket, Offsets::ClickableComponentLeftNeighborId, 109);
        SetNeighbor(trinket, Offsets::ClickableComponentUpNeighborId, 201);
        SetNeighbor(
            trinket, Offsets::ClickableComponentRightNeighborId, farOuterRight);
        SetNeighbor(
            trinket, Offsets::ClickableComponentDownNeighborId, farOuterDown);
    }

    g_Page = page;
    SynchronizeItems();
    Logging.Log(
        "[WearMoreRings] layout2 attached at %llX "
        "left=(%d,%d)/(%d,%d)/boots(%d,%d) "
        "far=(%d,%d)/(%d,%d)/trinket(%d,%d) present=%u",
        static_cast<unsigned long long>(
            reinterpret_cast<std::uintptr_t>(page)),
        leftOriginal.x, leftOriginal.y,
        rightOriginal.x, rightOriginal.y,
        bootsBounds->x, bootsBounds->y,
        farTop.x, farTop.y,
        farTop.x, farTop.y + kRingCellSize,
        trinketBounds != nullptr ? trinketBounds->x : -1,
        trinketBounds != nullptr ? trinketBounds->y : -1,
        trinket != nullptr ? 1U : 0U);
    return true;
}

bool WearMoreRings::IsAttachedInventoryPage(void* page) noexcept {
    return page != nullptr && page == g_Page && g_Components[0] != nullptr &&
           g_Components[1] != nullptr && g_Components[2] != nullptr &&
           g_Components[3] != nullptr;
}

void WearMoreRings::SetNamesHidden(bool hidden) noexcept {
    if (g_Page == nullptr || hidden == g_NamesHidden) {
        return;
    }
    void* const hiddenName = ResolveRootedObject(Offsets::StringSlotMineElevator);
    if (hidden && hiddenName == nullptr) {
        return;
    }
    for (std::uint32_t slot = 0; slot < SlotCount; ++slot) {
        if (g_Components[slot] != nullptr) {
            *Field<void*>(
                g_Components[slot], Offsets::ClickableComponentName) =
                hidden ? hiddenName : g_NormalNames[slot];
        }
    }
    g_NamesHidden = hidden;
}

void WearMoreRings::SynchronizeItems() noexcept {
    if (g_Page == nullptr) {
        return;
    }
    std::array<void*, SlotCount> items{};
    bool overflow = false;
    if (!GetVisibleItems(items, &overflow)) {
        return;
    }
    for (std::uint32_t slot = 0; slot < SlotCount; ++slot) {
        SetComponentItem(slot, items[slot]);
    }
    if (overflow && !g_LoggedOverflow) {
        g_LoggedOverflow = true;
        Logging.Log(
            "[WearMoreRings] right CombinedRing has more than three children; "
            "editing is disabled to prevent item loss");
    }
}

bool WearMoreRings::HandleClick(
    void* page, std::int32_t x, std::int32_t y) noexcept {
    if (!IsAttachedInventoryPage(page)) {
        return false;
    }
    std::uint32_t slot = SlotCount;
    for (std::uint32_t index = 0; index < SlotCount; ++index) {
        if (PointInside(g_Components[index], x, y)) {
            slot = index;
            break;
        }
    }
    if (slot == SlotCount) {
        return false;
    }

    const auto getPlayer = reinterpret_cast<GetPlayerFn>(
        Target(Offsets::Game1GetPlayer));
    void* const farmer = getPlayer();
    if (farmer == nullptr) {
        return true;
    }
    const auto getCursor = reinterpret_cast<GetCursorFn>(
        Target(Offsets::FarmerGetCursorSlotItem));
    const auto normalize = reinterpret_cast<NormalizeItemFn>(
        Target(Offsets::NormalizeHeldItemBody));
    void* const cursor = normalize(getCursor(farmer));
    if (cursor != nullptr && !IsRing(cursor)) {
        return true;
    }

    const auto equip = reinterpret_cast<FarmerEquipFn>(
        Target(Offsets::FarmerEquipBody));
    void* oldItem = nullptr;
    if (slot == 0) {
        void* const leftRef = GetFarmerEquipmentRef(
            farmer, Offsets::FarmerLeftRingRef);
        if (leftRef == nullptr) {
            return true;
        }
        oldItem = GetNetRefValue(leftRef);
        if (cursor == oldItem) {
            return true;
        }
        equip(farmer, cursor, leftRef);
    } else {
        void* const rightRef = GetFarmerEquipmentRef(
            farmer, Offsets::FarmerRightRingRef);
        if (rightRef == nullptr) {
            return true;
        }
        std::array<void*, kRightSlotCount> children{};
        bool overflow = false;
        std::uint32_t count = ReadCombinedChildren(
            GetNetRefValue(rightRef), children, &overflow);
        if (overflow) {
            return true;
        }

        const std::uint32_t childIndex = slot - 1;
        oldItem = childIndex < count ? children[childIndex] : nullptr;
        if (cursor == nullptr) {
            if (oldItem == nullptr) {
                return true;
            }
            for (std::uint32_t index = childIndex; index + 1 < count; ++index) {
                children[index] = children[index + 1];
            }
            children[count - 1] = nullptr;
            --count;
        } else if (childIndex < count) {
            if (cursor == oldItem) {
                return true;
            }
            children[childIndex] = cursor;
        } else {
            if (count >= kRightSlotCount) {
                return true;
            }
            // The backing list is intentionally dense. Clicking either later
            // empty cell inserts into the first available visible slot.
            children[count++] = cursor;
        }

        void* const newRight = BuildRightEquipment(children, count);
        if (count != 0 && newRight == nullptr) {
            Logging.Log("[WearMoreRings] ERROR: couldn't build right ring container");
            return true;
        }
        equip(farmer, newRight, rightRef);
    }

    const auto prepare = reinterpret_cast<NormalizeItemFn>(
        Target(Offsets::PrepareUnequippedItemBody));
    const auto setHeld = reinterpret_cast<SetHeldItemFn>(
        Target(Offsets::InventoryPageSetHeldItem));
    setHeld(page, prepare(oldItem));
    SynchronizeItems();
    Logging.Log(
        "[WearMoreRings] slot=%u equipped=%llX returned=%llX",
        slot,
        static_cast<unsigned long long>(
            reinterpret_cast<std::uintptr_t>(cursor)),
        static_cast<unsigned long long>(
            reinterpret_cast<std::uintptr_t>(oldItem)));
    return true;
}

bool WearMoreRings::HandleHover(
    void* page, std::int32_t x, std::int32_t y) noexcept {
    if (!IsAttachedInventoryPage(page)) {
        return false;
    }
    std::array<void*, SlotCount> items{};
    if (!GetVisibleItems(items)) {
        return false;
    }
    for (std::uint32_t slot = 0; slot < SlotCount; ++slot) {
        if (!PointInside(g_Components[slot], x, y)) {
            continue;
        }
        void* const item = items[slot];
        *Field<void*>(page, Offsets::InventoryPageHoveredItem) = item;
        *Field<std::int32_t>(page, Offsets::InventoryPageHoverAmount) = -1;
        *Field<void*>(page, Offsets::InventoryPageHoverText) =
            CallItemString(item, 0x180, 0x188);
        *Field<void*>(page, Offsets::InventoryPageHoverTitle) =
            CallItemString(item, 0x350, 0x358);
        return true;
    }
    return false;
}

void WearMoreRings::Draw(void* page, void* spriteBatch) noexcept {
    if (!IsAttachedInventoryPage(page)) {
        return;
    }
    std::array<void*, SlotCount> items{};
    if (!GetVisibleItems(items)) {
        return;
    }
    for (std::uint32_t slot = 0; slot < SlotCount; ++slot) {
        SetComponentItem(slot, items[slot]);
        DrawCell(g_Components[slot], items[slot], spriteBatch);
    }
}

} // namespace AutomateLite::Rings
