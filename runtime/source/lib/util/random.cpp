#include "random.hpp"

namespace exl::util {

    u64 GetRandomU64() {
        // SplitMix64 finalizer. This is only used to choose a candidate virtual
        // address; cryptographic randomness is neither required nor implied.
        u64 value = svcGetSystemTick() + 0x9E3779B97F4A7C15ULL;
        value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
        value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
        return value ^ (value >> 31);
    }

    extern "C" u64 exl_random() {
        return GetRandomU64();
    }
}
