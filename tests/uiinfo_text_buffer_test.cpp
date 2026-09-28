#include "uiinfo/text_buffer.hpp"
using AutomateLite::UIInfo::TextBuffer;
template <std::size_t N, std::size_t M>
constexpr bool Equals(const TextBuffer<N>& text, const char16_t (&expected)[M]) {
    if (text.Size() != M - 1) return false;
    for (std::size_t i = 0; i < M; ++i)
        if (text.Data()[i] != expected[i]) return false;
    return true;
}
constexpr bool NumericEdges() {
    TextBuffer<64> zero, minimum, maximum, stackPrice;
    return zero.AppendNumber(0) && Equals(zero, u"0") &&
        minimum.AppendNumber(-9223372036854775807LL - 1) &&
        Equals(minimum, u"-9223372036854775808") &&
        maximum.AppendNumber(9223372036854775807LL) &&
        Equals(maximum, u"9223372036854775807") &&
        stackPrice.AppendNumber(std::int64_t{2147483647} * 2147483647) &&
        Equals(stackPrice, u"4611686014132420609");
}
constexpr bool CapacityEdges() {
    TextBuffer<5> text;
    if (!text.Append(u"售价")) return false;
    if (text.AppendNumber(123)) return false; // rejected atomically, keeps terminator
    if (!Equals(text, u"售价")) return false;
    if (!text.AppendNumber(12) || !Equals(text, u"售价12")) return false;
    return !text.Append(u"x") && Equals(text, u"售价12");
}
static_assert(NumericEdges(), "64-bit stack totals and signed limits");
static_assert(CapacityEdges(), "UTF-16 capacity, null terminator, no partial writes");
constexpr bool PointerEdges() {
    TextBuffer<5> text;
    const char16_t* first=u"售价";
    const char16_t* overflow=u"123";
    return !text.Append(nullptr) && !text.Append(nullptr,1) && text.Append(nullptr,0) &&
        text.Append(first) && !text.Append(overflow) && Equals(text,u"售价") &&
        text.Append(static_cast<const char16_t*>(u"12")) && Equals(text,u"售价12");
}
static_assert(PointerEdges());
