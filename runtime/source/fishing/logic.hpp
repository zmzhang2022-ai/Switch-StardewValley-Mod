#pragma once
#include <cstdint>

namespace AutomateLite::Fishing {
enum class Status { Off, Ready, Casting, Waiting, Catching, Looting, Eating,
    NoFood, Full, Late, Menu, Disconnected, InvalidWater, Interrupted, Unsupported };
struct Chord {
    bool latched{};
    constexpr bool Update(bool left, bool right) {
        if (!left && !right) latched = false;
        if (left && right && !latched) { latched = true; return true; }
        return false;
    }
};
constexpr bool NeedsFood(float stamina) { return !(stamina >= 10.0f); }
// Prefer the smallest sufficient recovery; otherwise the largest available.
constexpr bool BetterFood(int recovery, int best, float missing) {
    if (recovery <= 0) return false;
    if (best <= 0) return true;
    const bool enough = recovery >= missing, bestEnough = best >= missing;
    if (enough != bestEnough) return enough;
    return enough ? recovery < best : recovery > best;
}
constexpr bool TooLate(int time) { return time >= 2500; }
}
