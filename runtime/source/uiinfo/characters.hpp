#pragma once
#include "uiinfo/aot.hpp"
namespace AutomateLite::UIInfo {
// Only callback-local managed references. Includes instantiated interiors;
// does not create maps or characters merely to display information.
template <typename Visitor> void VisitVillagers(Visitor visitor) {
    using namespace Aot;
    void* visited[512]{}; unsigned used{};
    auto location=[&](void* place) {
        if(!place || used>=512) return;
        void* collection=Read<void*>(place,0xB0);
        if(!collection || !Read<void*>(collection,0x48)) return;
        const int count=Call<int>(0x5125A0C,collection);
        if(count<0 || count>2048) return;
        for(int i=0;i<count && used<512;++i) {
            void* npc=Call<void*>(0x5125A20,collection,i);
            if(!npc || !Call<bool>(0x18F0510,npc)) continue;
            bool duplicate=false;
            for(unsigned j=0;j<used;++j) if(visited[j]==npc) { duplicate=true; break; }
            if(duplicate) continue;
            visited[used++]=npc; visitor(npc);
        }
    };
    auto locations=Game::GameState::GetLoadedLocations();
    if(!locations.readable) return;
    for(unsigned i=0;i<locations.count;++i) {
        void* place=locations.Get(i); if(!place)continue; location(place);
        auto interiors=Game::GameState::GetInstancedBuildingInteriors(place);
        if(interiors.readable) for(unsigned j=0;j<interiors.count;++j) location(interiors.Get(j));
    }
}
}
