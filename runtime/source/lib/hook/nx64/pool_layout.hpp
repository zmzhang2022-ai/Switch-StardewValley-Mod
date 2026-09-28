#pragma once
#include <cstddef>
namespace exl::hook::nx64::pool {
inline constexpr std::size_t MaxInstructions = 5;
inline constexpr std::size_t TrampolineWords = MaxInstructions * 10;
inline constexpr std::size_t TrampolineBytes = TrampolineWords * 4;
constexpr std::size_t Capacity(std::size_t bytes) { return bytes / TrampolineBytes; }
constexpr bool Contains(std::size_t bytes, std::size_t index) {
    return index < Capacity(bytes);
}
}
