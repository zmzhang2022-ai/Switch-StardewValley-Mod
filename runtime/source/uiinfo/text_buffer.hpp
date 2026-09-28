#pragma once
#include <cstddef>
#include <cstdint>

namespace AutomateLite::UIInfo {
// Owns only native UTF-16 characters. No managed pointer survives a callback.
template <std::size_t Capacity>
class TextBuffer final {
public:
    constexpr bool Append(const char16_t* value) noexcept {
        if (!value) return false;
        std::size_t length = 0;
        while (value[length]) {
            if (++length >= Capacity) return false;
        }
        return Append(value, length);
    }
    constexpr bool Append(const char16_t* value, std::size_t length) noexcept {
        if (!value && length) return false;
        if (length > Capacity - 1 - m_Size) return false;
        for (std::size_t i = 0; i < length; ++i) m_Data[m_Size++] = value[i];
        m_Data[m_Size] = 0;
        return true;
    }
    template <std::size_t N>
    constexpr bool Append(const char16_t (&value)[N]) noexcept {
        return Append(value, N - 1);
    }
    constexpr bool AppendNumber(std::int64_t value) noexcept {
        char16_t reversed[21]{};
        char16_t result[21]{};
        std::size_t count = 0, length = 0;
        const bool negative = value < 0;
        // Unsigned arithmetic also handles INT64_MIN without undefined behavior.
        std::uint64_t magnitude = negative
            ? std::uint64_t{0} - static_cast<std::uint64_t>(value)
            : static_cast<std::uint64_t>(value);
        do {
            reversed[count++] = static_cast<char16_t>(u'0' + magnitude % 10);
            magnitude /= 10;
        } while (magnitude != 0);
        if (negative) result[length++] = u'-';
        while (count != 0) result[length++] = reversed[--count];
        return Append(result, length);
    }
    constexpr const char16_t* Data() const noexcept { return m_Data; }
    constexpr std::size_t Size() const noexcept { return m_Size; }
private:
    static_assert(Capacity > 1);
    char16_t m_Data[Capacity]{};
    std::size_t m_Size{};
};
}
