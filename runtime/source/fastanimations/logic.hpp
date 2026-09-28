#pragma once
#include <cstdint>
namespace AutomateLite::FastAnimations {
// Reserved high bit distinguishes v13 masks from explicitly configured v13.1 masks.
constexpr std::uint32_t ConfigMarker=0x80000000u;
constexpr std::uint32_t EnabledBit=1u<<29, TripleBit=1u<<30;
constexpr std::uint32_t MigrateMask(std::uint32_t mask) {
    return (mask&ConfigMarker) ? mask : (mask|ConfigMarker|EnabledBit)&~TripleBit;
}
constexpr unsigned ExtraUpdates(bool enabled,bool triple) { return enabled ? (triple ? 2u : 1u) : 0u; }
constexpr int ReducePause(int value,int elapsed) {
    return value<=0 ? value : (elapsed>=value ? 0 : value-elapsed);
}
// ScreenFade.Update checks alpha > 1.1 / alpha < -0.1 BEFORE advancing it.
// This is transition state, not a render opacity: clamping to [0,1] after the
// original tick erases its sentinel values and prevents completion callbacks.
constexpr float AdvanceFade(float alpha,bool fadeIn,int elapsed,unsigned extra) {
    const float step=0.0019f*static_cast<float>(elapsed)*extra;
    return alpha+(fadeIn ? step : -step);
}
}
