#include "uiinfo/npc_map.hpp"
#include "uiinfo/friendship.hpp"
#include "uiinfo/settings.hpp"
#include "uiinfo/npc_map_logic.hpp"
#include "uiinfo/npc_map_signatures.hpp"
namespace AutomateLite::UIInfo {
namespace {
using namespace Aot;
namespace Logic=NpcMapLogic;
constexpr unsigned Capacity=256;
struct Point { int x,y; };
struct Position { void* area; void* location; Point tile; };
struct NullablePosition { Position value; std::uint8_t present; std::uint8_t padding[7]; };
static_assert(sizeof(Position)==24 && sizeof(NullablePosition)==32);
// Only copied strings/numbers survive a draw. No cached managed object/texture.
struct Marker {
    TextBuffer<96> id,name; TextBuffer<192> detail;
    Game::TilePosition pixel{}; Rectangle source{};
    unsigned special{},group{}; bool birthday{},quest{};
};
Marker g_Markers[Capacity]; unsigned g_Count{};
unsigned g_Hover[Capacity],g_HoverCount{};
std::uint64_t g_Next{},g_Save{},g_HoverSince{};
int g_Day{},g_Year{},g_Season{};
TextBuffer<128> g_Region;
bool g_Installed{},g_HoverReady{};
// Callback-local roots, including a ridden horse absent from location.characters.
struct LiveNpcs {
    void* values[Capacity]{}; unsigned count{};
    void Add(void* npc) {
        if(!npc || count==Capacity) return;
        for(unsigned i=0;i<count;++i) if(values[i]==npc) return;
        // The base IsVillager body is constant true. Virtual dispatch matters!
        if(!Virtual<bool>(npc,0x110) && !Is(npc,0xDDAC400) && !Is(npc,0xDDAB4A0)) return;
        values[count++]=npc;
    }
    void Location(void* place) {
        if(!place) return;
        void* collection=Read<void*>(place,0xB0);
        if(!collection || !Read<void*>(collection,0x48)) return;
        const int n=Call<int>(0x5125A0C,collection);
        if(n<0 || n>2048) return;
        for(int i=0;i<n && count<Capacity;++i) Add(Call<void*>(0x5125A20,collection,i));
    }
    void Collect(void* player) {
        auto locations=Game::GameState::GetLoadedLocations();
        if(locations.readable) for(unsigned i=0;i<locations.count;++i) {
            void* place=locations.Get(i); if(!place) continue;
            Location(place);
            auto interiors=Game::GameState::GetInstancedBuildingInteriors(place);
            if(interiors.readable) for(unsigned j=0;j<interiors.count;++j) Location(interiors.Get(j));
        }
        Location(Game::GameState::GetCurrentLocation()); Add(Call<void*>(0x13183A0,player));
    }
    void* Find(const char16_t* name) const {
        for(unsigned i=0;i<count;++i) if(Equals(Call<void*>(0x1188600,values[i]),name)) return values[i];
        return nullptr;
    }
};
void* FindLocation(const char16_t* name) {
    auto locations=Game::GameState::GetLoadedLocations();
    if(locations.readable) for(unsigned i=0;i<locations.count;++i) {
        void* place=locations.Get(i);
        if(place && Equals(Call<void*>(0x1425C80,place),name)) return place;
    }
    return nullptr;
}
bool MapPosition(void* place,Point tile,Marker& marker) {
    if(!place || tile.x<0 || tile.y<0) return false;
    auto position=Call<NullablePosition>(0x1B0E7B0,place,tile);
    if(!position.present || !position.value.area) return false;
    void* region=Call<void*>(0x1B0C020,position.value.area);
    if(!region || !Equals(Call<void*>(0x1B0D0E0,region),g_Region.Data())) return false;
    marker.pixel=Call<Game::TilePosition>(0x1B0CC70,&position.value);
    return marker.pixel.x>=0 && marker.pixel.y>=0 && marker.pixel.x<32768 && marker.pixel.y<32768;
}
struct QuestTargets {
    TextBuffer<96> names[64]; unsigned count{};
    void Collect(void* player) {
        void* list=Read<void*>(player,0x1B0);
        if(!list) return;
        const int n=Call<int>(0x36C62B8,list);
        if(n<0 || n>1024) return;
        for(int i=0;i<n && count<64;++i) {
            void* quest=Call<void*>(0x36C61A0,list,i);
            if(!quest || !NetBool(quest,0x30) || !NetBool(quest,0x40) || NetBool(quest,0x38)) continue;
            // Concrete metadata, never select a derived layout by questType alone.
            unsigned field=Is(quest,0xDE46020) ? 0xB0 : Is(quest,0xDE47ED0) ? 0xB8 :
                (Is(quest,0xDE45618) || Is(quest,0xDE47820)) ? 0xA8 : 0;
            void* net=field ? Read<void*>(quest,field) : nullptr;
            if(net && Append(names[count],Read<void*>(net,0x58)) && names[count].Size()) ++count;
        }
    }
    bool Has(void* name) const {
        for(unsigned i=0;i<count;++i) if(Equals(name,names[i].Data())) return true;
        return false;
    }
};
void AddSpecial(unsigned kind,void* location,Point tile,const char16_t* name,Rectangle source) {
    if(g_Count>=Capacity) return;
    Marker m{}; m.special=kind; m.source=source;
    if(!MapPosition(location,tile,m)) return;
    m.name.Append(name); m.detail.Append(name); m.detail.Append(u" · 今日营业\n");
    g_Markers[g_Count++]=m;
}
void Refresh(void* page,void* player,const LiveNpcs& live) {
    void* region=Read<void*>(page,0x88);
    if(!region) { g_Count=0; g_Next=0; return; }
    void* regionId=Call<void*>(0x1B0D0E0,region);
    const auto now=Milliseconds(),save=Static<std::uint64_t>(0xE27FA00);
    const int day=Static<int>(0xE27F7A8),season=Static<int>(0xE27F720),year=Static<int>(0xE27F7AC);
    if(save==g_Save && day==g_Day && season==g_Season && year==g_Year && Equals(regionId,g_Region.Data()) && now<g_Next) return;
    g_Save=save; g_Day=day; g_Season=season; g_Year=year;
    g_Next=now+500; g_Count=0; g_Region={};
    if(!Append(g_Region,regionId)) return;
    QuestTargets quests{}; quests.Collect(player);
    for(unsigned i=0;i<live.count && g_Count<Capacity;++i) {
        void* npc=live.values[i]; void* name=Call<void*>(0x1188600,npc);
        if(!name || !IsNpcTracked(name)) continue;
        Marker m{}; if(!Append(m.id,name) || !m.id.Size()) continue;
        if(!MapPosition(Call<void*>(0x1188700,npc),Call<Point>(0x1188500,npc),m)) continue;
        m.source=Virtual<Rectangle>(npc,0x560); // includes child's override
        Append(m.name,Virtual<void*>(npc,0x80));
        if(!m.name.Size()) m.name.Append(m.id.Data());
        m.birthday=Call<bool>(0x18FFCD0,npc); m.quest=quests.Has(name);
        m.detail.Append(m.name.Data());
        if(m.birthday) m.detail.Append(u" · 生日");
        if(m.quest) m.detail.Append(u" · 每日委托");
        m.detail.Append(u"\n");
        void* friendship=Friendship(player,name);
        if(friendship) {
            m.detail.Append(Call<bool>(0x1398060,friendship) ? u"已交谈" : u"未交谈");
            if(Enabled(Option::Gifts)) m.detail.Append(Call<int>(0x1398000,friendship)>0 ? u" · 已送礼" : u" · 未送礼");
            m.detail.Append(u" · "); m.detail.AppendNumber(Call<int>(0x1397FC0,friendship)/250); m.detail.Append(u"心");
        } else m.detail.Append(u"尚无好感记录");
        g_Markers[g_Count++]=m;
    }
    void* forest=FindLocation(u"Forest");
    if(forest && Is(forest,0xDDD9018) && Call<bool>(0x158F0E0,forest)) {
        Point tile=Call<Point>(0x158F140,forest); tile.x+=4;
        AddSpecial(1,forest,tile,u"旅行商人",{191,1410,22,21});
    }
    void* town=FindLocation(u"Town");
    if(town) {
        void* days=Call<void*>(0x1AE9B30);
        const int n=days ? Call<int>(0x3EC9C58,days) : 0;
        if(n>=0 && n<=28) for(int i=0;i<n;++i) if(Call<int>(0x839F0,days,i)==day) {
            AddSpecial(2,town,{108,25},u"书摊老板",{180,490,14,18}); break;
        }
    }
    for(unsigned i=0;i<g_Count;++i) g_Markers[i].group=i;
    for(unsigned i=0;i<g_Count;++i) for(unsigned j=0;j<i;++j)
        if(Logic::Near(g_Markers[i].pixel.x,g_Markers[i].pixel.y,g_Markers[j].pixel.x,g_Markers[j].pixel.y)) {
            const unsigned from=g_Markers[i].group,to=g_Markers[j].group;
            for(unsigned k=0;k<g_Count;++k) if(g_Markers[k].group==from) g_Markers[k].group=to;
        }
}
HOOK_DEFINE_TRAMPOLINE(NpcMapHook) {
    static void Callback(void* page,void* batch,float alpha) {
        g_HoverReady=false; Orig(page,batch,alpha);
        RefreshSettings(); void* player=Game::GameState::GetPlayer();
        if(!Enabled(Option::NpcMap) || !player || Call<bool>(0x139F590)) { g_Next=0; return; }
        LiveNpcs live{}; live.Collect(player); Refresh(page,player,live);
        const auto bounds=Read<Rectangle>(page,0xA4);
        if(!Logic::ValidBounds(bounds.width,bounds.height)) return;
        const int mx=Call<int>(0x13C3920,true),my=Call<int>(0x13C3A40,true);
        const unsigned opacity=static_cast<unsigned>((alpha>=0 && alpha<=1 ? alpha : 1)*255);
        bool groups[Capacity]{};
        for(unsigned i=0;i<g_Count;++i) {
            const auto& m=g_Markers[i];
            if(!Logic::OnMap(m.pixel.x,m.pixel.y,bounds.width,bounds.height)) continue;
            const int x=bounds.x+static_cast<int>(m.pixel.x),y=bounds.y+static_cast<int>(m.pixel.y);
            void* npc=m.special ? nullptr : live.Find(m.id.Data());
            if(!m.special && !npc) continue;
            void* sprite=npc ? Virtual<void*>(npc,0xE0) : nullptr;
            void* texture=m.special ? Static<void*>(m.special==1 ? 0xE27F558 : 0xE27F568) :
                (sprite ? Call<void*>(0x10FF650,sprite) : nullptr);
            Box(batch,{x-16,y-22,32,40},FadeColor(m.birthday ? 0xFF4088DD : m.quest ? 0xFF308848 : 0xFF504030,opacity));
            if(texture && Logic::ValidSource(m.source.x,m.source.y,m.source.width,m.source.height)) {
                NullableRectangle source{m.source,1,{}};
                Call<void>(0xBFF70,batch,texture,Rectangle{x-14,y-20,28,36},&source,FadeColor(0xFFFFFFFF,opacity));
            } else {
                TextBuffer<4> initial; initial.Append(m.name.Data(),1);
                Text(batch,initial,static_cast<float>(x-9),static_cast<float>(y-10),FadeColor(0xFFFFFFFF,opacity),0.65f);
            }
            if(m.birthday || m.quest) {
                TextBuffer<4> badge; badge.Append(m.birthday ? u"生" : u"!");
                Text(batch,badge,static_cast<float>(x+8),static_cast<float>(y-26),FadeColor(0xFFFFFFFF,opacity),0.5f);
            }
            if(mx>=x-20 && mx<x+20 && my>=y-26 && my<y+22) groups[m.group]=true;
        }
        unsigned selected[Capacity]{},count{};
        for(unsigned i=0;i<g_Count;++i) if(groups[g_Markers[i].group]) selected[count++]=i;
        bool same=count==g_HoverCount;
        for(unsigned i=0;i<count && same;++i) if(g_Hover[i]!=selected[i]) same=false;
        if(!same) g_HoverSince=Milliseconds();
        g_HoverCount=count; for(unsigned i=0;i<count;++i) g_Hover[i]=selected[i];
        g_HoverReady=count>0;
    }
};
HOOK_DEFINE_TRAMPOLINE(NpcMapTooltipHook) {
    static void Callback(void* page,void* batch) {
        if(!g_HoverReady || !Enabled(Option::NpcMap)) { Orig(page,batch); return; }
        g_HoverReady=false;
        void* viewport=reinterpret_cast<void*>(Address(0xE27F4C8));
        const int sw=Call<int>(0x1D8EF70,viewport),sh=Call<int>(0x1D8EF50,viewport);
        if(sw<320 || sh<160) { Orig(page,batch); return; }
        const unsigned rows=Logic::TooltipRows(sh),pages=(g_HoverCount+rows-1)/rows;
        const unsigned current=static_cast<unsigned>((Milliseconds()-g_HoverSince)/3500)%pages;
        TextBuffer<2048> text;
        if(g_HoverCount>1) {
            text.Append(u"附近 "); text.AppendNumber(g_HoverCount); text.Append(u" 位人物");
            if(pages>1) { text.Append(u" · "); text.AppendNumber(current+1); text.Append(u"/"); text.AppendNumber(pages); text.Append(u" 自动翻页"); }
            text.Append(u"\n");
        }
        unsigned shown{};
        for(unsigned i=current*rows;i<g_HoverCount && shown<rows;++i,++shown) {
            if(shown) text.Append(u"\n"); text.Append(g_Markers[g_Hover[i]].detail.Data());
        }
        const int width=sw<560 ? sw-16 : 544,height=static_cast<int>(shown)*56+(g_HoverCount>1 ? 28 : 0)+16;
        int x=Call<int>(0x13C3920,true)+24,y=Call<int>(0x13C3A40,true)+24;
        x=Logic::Clamp(x,8,sw-width-8); y=Logic::Clamp(y,8,sh-height-8);
        Box(batch,{x,y,width,height},0xF0201C18);
        Text(batch,text,static_cast<float>(x+8),static_cast<float>(y+8),0xFFFFFFFF,0.65f);
    }
};
}
void InstallMapHook() {
    if(g_Installed) return;
    for(const auto& s:NpcMapSignatures::Entries) for(unsigned i=0;i<4;++i)
        if(Static<std::uint32_t>(s.offset+i*4)!=s.words[i]) {
            Logging.Log("[NPCMapLocations] fingerprint mismatch main+0x%llX; map extension disabled",static_cast<unsigned long long>(s.offset)); return;
        }
    NpcMapHook::InstallAtOffset(0x173C7F0);
    NpcMapTooltipHook::InstallAtOffset(0x173DD40);
    g_Installed=true;
}
}
