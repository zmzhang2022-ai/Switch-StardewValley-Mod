#pragma once
#include <cstdint>

// Exact 1.6.15.3 main A5C617C14A7F3F6620B3BC8136965A4822D32B9C.
// Evidence and invokers: docs/UIINFO_SUITE2_SWITCH.md and analysis/uiinfo-aot-batch1.txt.
namespace AutomateLite::UIInfo::Offsets {
inline constexpr std::uintptr_t DrawHud = 0x013E7C30;
inline constexpr std::uintptr_t GetGameMode = 0x0139CBD0;
inline constexpr std::uintptr_t GetActiveMenu = 0x0139DCB0;
inline constexpr std::uintptr_t SpriteBatchRoot = 0x0DFF13C8;
inline constexpr std::uintptr_t SmallFontRoot = 0x0DFEF730;
inline constexpr std::uintptr_t DisplayHudStorage = 0x0DFF1CC0;
inline constexpr std::uintptr_t EventUpStorage = 0x0DFEF0B0;
// drawHUD+0x158..15C returns early while this rooted object is non-null.
// Keep the observed vanilla gate; the object's semantic type is not yet identified.
inline constexpr std::uintptr_t HudSuppressionRoot = 0x0DFEF048;
inline constexpr std::uintptr_t FreezeControlsStorage = 0x0DFF0028;
inline constexpr std::uintptr_t ViewportFreezeStorage = 0x0DFF0D40;
inline constexpr std::uintptr_t ViewportHoldStorage = 0x0DFF1860;
inline constexpr std::uintptr_t CursorTileStorage = 0x0DFF3C90;
inline constexpr std::uintptr_t TakingMapScreenshot = 0xEB; // Game1 instance field, bool
// OverlaidDictionary<Vector2,Object>.TryGetValue: x0=this, s0/s1=tile, x1=out Object*.
inline constexpr std::uintptr_t TryGetObject = 0x018EAE50;
inline constexpr std::uintptr_t ObjectMetadataRoot = 0x0DF0DC28;
// Item's slots, NOT HouseRenovation's same-named slots at 0x130/0x140/0xF0.
inline constexpr std::uintptr_t SellToStorePriceSlot = 0x1A0; // x1=int64 playerID (-1)
inline constexpr std::uintptr_t SalePriceSlot = 0x1B0; // w1=bool ignoreProfitMargins
inline constexpr std::uintptr_t DisplayNameSlot = 0x350;
inline constexpr std::uintptr_t StackSlot = 0x390;
// String allocator used by String.Concat: w0=UTF-16 length, returns a NEW string.
// Dynamic text MUST NOT use 0x12CE0: that function interns text in a global table.
inline constexpr std::uintptr_t AllocateString = 0x00012340;
inline constexpr std::uintptr_t StringLength = 0x10;
inline constexpr std::uintptr_t StringCharacters = 0x14;
// x0=batch,x1=text,x2=font,s0/s1=position,w3=color,s2=scale,s3=depth,
// w4/w5=shadow offsets,s4=shadow intensity,w6=number of shadows.
// The string overload calls SpriteBatch.DrawString at 0xC0E20.
// 0x1AECD00 instead expects StringBuilder and calls 0xC1A60: do not substitute it.
inline constexpr std::uintptr_t DrawTextWithShadow = 0x01AED050;
}
