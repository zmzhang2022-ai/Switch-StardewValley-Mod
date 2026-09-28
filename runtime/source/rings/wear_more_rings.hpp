#pragma once

#include <cstdint>

namespace AutomateLite::Rings {

// Fixed four-slot implementation for Stardew Valley 1.6.15.3.  Slot zero is
// Farmer.leftRing; the three remaining visible slots are backed by the child
// list of a vanilla CombinedRing stored in Farmer.rightRing.
class WearMoreRings final {
public:
    static constexpr std::uint32_t SlotCount = 4;

    static bool AttachInventoryPage(void* page) noexcept;
    static bool IsAttachedInventoryPage(void* page) noexcept;
    static void SetNamesHidden(bool hidden) noexcept;
    static void SynchronizeItems() noexcept;
    static bool HandleClick(
        void* page, std::int32_t x, std::int32_t y) noexcept;
    static bool HandleHover(
        void* page, std::int32_t x, std::int32_t y) noexcept;
    static void Draw(void* page, void* spriteBatch) noexcept;
};

} // namespace AutomateLite::Rings
