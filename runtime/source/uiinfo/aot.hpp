#pragma once
#include "uiinfo/text_buffer.hpp"
#include "uiinfo/offsets.hpp"
#include "game/runtime.hpp"
#include "lib.hpp"

// Exact-build helpers. No managed pointer is retained across callbacks.
namespace AutomateLite::UIInfo::Aot {
inline std::uint64_t Milliseconds() {
    // Same Horizon system counter and frequency as nn/os/impl tick manager.
    std::uint64_t tick;
    asm volatile("mrs %0, cntpct_el0" : "=r"(tick) :: "memory");
    return tick/19200;
}
inline std::uint32_t FadeColor(std::uint32_t color,unsigned alpha) {
    if(alpha>255) alpha=255;
    std::uint32_t result{};
    for(unsigned shift=0;shift<32;shift+=8) result|=(((color>>shift)&255)*alpha/255)<<shift;
    return result;
}
inline std::uintptr_t Address(std::uintptr_t offset) {
    return exl::util::modules::GetTargetOffset(offset);
}
template <typename T> T Read(void* object, std::uintptr_t offset) {
    return *reinterpret_cast<const T*>(reinterpret_cast<std::uintptr_t>(object) + offset);
}
template <typename T> T Static(std::uintptr_t offset) {
    return *reinterpret_cast<const T*>(Address(offset));
}
template <typename R, typename... Args> R Call(std::uintptr_t offset, Args... args) {
    return reinterpret_cast<R (*)(Args...)>(Address(offset))(args...);
}
template <typename R, typename... Args> R Virtual(void* object, std::uintptr_t slot, Args... args) {
    void* table = Read<void*>(Read<void*>(object, 0), 8);
    const auto function = Read<std::uintptr_t>(table, slot);
    const auto adjustment = Read<std::uint8_t>(table, slot + 8);
    return reinterpret_cast<R (*)(void*, Args...)>(function)(
        reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(object) + adjustment), args...);
}
inline bool Is(void* object, std::uintptr_t metadataRoot) {
    auto* type = Static<void*>(metadataRoot);
    return object && type && Call<void*>(0xAF0, object, type);
}
// NetInt/NetBool conversions: vanilla Crop.ReadyToHarvest consumers at
// 0x11CA934..978; concrete conversion bodies 0x10EBB30 and 0x10E3770.
inline int NetInt(void* object, std::uintptr_t field) {
    return object ? Call<int>(0x10EBB30, Read<void*>(object, field)) : 0;
}
inline bool NetBool(void* object, std::uintptr_t field) {
    return object && Call<bool>(0x10E3770, Read<void*>(object, field));
}
template <std::size_t N> bool Append(TextBuffer<N>& text, void* managed) {
    if (!managed) return true;
    const int length = Read<int>(managed, 0x10);
    return length >= 0 && text.Append(reinterpret_cast<char16_t*>(
        reinterpret_cast<std::uintptr_t>(managed) + 0x14), length);
}
inline bool Equals(void* managed, const char16_t* value) {
    if (!managed) return false;
    const int length = Read<int>(managed, 0x10);
    if (length < 0 || length > 4096) return false;
    for (int i = 0; i < length; ++i) {
        if (!value[i] || Read<char16_t>(managed, 0x14 + i * 2) != value[i]) return false;
    }
    return value[length] == 0;
}
template <std::size_t N> void* Managed(const TextBuffer<N>& text) {
    void* result = Call<void*>(Offsets::AllocateString, static_cast<int>(text.Size()));
    if (!result) return nullptr;
    auto* target = reinterpret_cast<char16_t*>(reinterpret_cast<std::uintptr_t>(result) + 0x14);
    for (std::size_t i = 0; i <= text.Size(); ++i) target[i] = text.Data()[i];
    return result;
}
template <std::size_t N> void* Literal(const char16_t (&value)[N]) {
    TextBuffer<N> text;
    text.Append(value);
    return Managed(text);
}
// ParsedItemData.DisplayName is +0x60. ItemRegistry.GetData is a static string
// overload (invoker 0x7311160), not the item-creating overload.
template <std::size_t N> bool ItemName(TextBuffer<N>& text, void* id) {
    void* data = id ? Call<void*>(0x1519890, id) : nullptr;
    return data && Append(text, Read<void*>(data, 0x60));
}
struct Rectangle { int x, y, width, height; };
struct NullableRectangle { Rectangle value{}; std::uint8_t hasValue{}; std::uint8_t padding[7]{}; };
inline void Box(void* batch, Rectangle bounds, std::uint32_t color) {
    void* texture = Static<void*>(0xE27F680); // Game1.staminaRect, white texture
    if (!batch || !texture || bounds.width <= 0 || bounds.height <= 0) return;
    NullableRectangle source{};
    Call<void>(0xBFF70, batch, texture, bounds, &source, color);
}
template <std::size_t N> void Text(void* batch, const TextBuffer<N>& text,
    float x, float y, std::uint32_t color = 0xFFFFFFFFu, float scale = 1.0f) {
    if (!batch || !text.Size()) return;
    void* font = **reinterpret_cast<void***>(Address(Offsets::SmallFontRoot));
    if (!font) return;
    void* managed = Managed(text);
    if (!managed) return;
    Call<void>(Offsets::DrawTextWithShadow, batch, managed, font,
        Game::TilePosition{x, y}, color, scale, 0.99f, -1, -1, 0.5f, 3);
}
}
