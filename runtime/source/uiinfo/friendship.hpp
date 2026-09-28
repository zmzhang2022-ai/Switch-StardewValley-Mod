#pragma once
#include "uiinfo/aot.hpp"
namespace AutomateLite::UIInfo {
inline void* Friendship(void* farmer,void* name) {
    if(!farmer || !name) return nullptr;
    auto* owner=Aot::Read<void*>(farmer,0x6D8);
    auto* dictionary=owner ? Aot::Read<void*>(owner,0x48) : nullptr;
    void* field{};
    // Exact owning dictionary chain from Farmer.tryGetFriendshipLevelForNPC.
    return dictionary && Aot::Call<bool>(0xDE8220,dictionary,name,&field) && field ?
        Aot::Virtual<void*>(owner,0x210,field) : nullptr;
}
}
