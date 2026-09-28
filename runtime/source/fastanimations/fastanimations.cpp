#include "fastanimations/fastanimations.hpp"
#include "fastanimations/logic.hpp"
#include "fastanimations/signatures.hpp"
#include "uiinfo/aot.hpp"
#include "uiinfo/settings.hpp"

namespace AutomateLite::FastAnimations {
namespace {
using namespace UIInfo::Aot;
template<class T> void Write(void* p,unsigned offset,T value) {
    *reinterpret_cast<T*>(reinterpret_cast<std::uintptr_t>(p)+offset)=value;
}
void* Player() { return Call<void*>(0x139C170); }
void* Menu() { return Call<void*>(0x139DCB0); }
void* Location() { return Call<void*>(0x139C3C0); }
bool Safe() {
    void* game=Static<void*>(0xE27FB10);
    return game && !Call<bool>(0x139F590) && !Call<bool>(0x139DC40,game);
}
bool WorldSafe() {
    return Safe() && Call<std::uint8_t>(0x139CBD0)==3 && Player() && Location() &&
        !Static<bool>(0xE27F6BB) && !Static<bool>(0xE27F6BC) &&
        !Call<void*>(0x139EBE0) && !Static<void*>(0xE27FAB8);
}
bool SameText(void* a,void* b) {
    if(!a || !b) return false;
    const int n=Read<int>(a,0x10);
    if(n<0 || n>4096 || n!=Read<int>(b,0x10)) return false;
    for(int i=0;i<n;++i) if(Read<char16_t>(a,0x14+2*i)!=Read<char16_t>(b,0x14+2*i)) return false;
    return true;
}
// Use the same localized prompt test as the reference mod. Never accept a
// dialogue merely because it happens to have a Yes button.
bool AcceptFood() {
    void* menu=Menu();
    if(!WorldSafe() || !Is(menu,0xDDFB2D8)) return false;
    void* player=Player();
    void* food=Read<void*>(player,0x3B0);
    void* content=Static<void*>(0xE27F378);
    if(!food || !content) return false;
    void* prompt=Call<void*>(0x16CFCA0,menu);
    void* name=Virtual<void*>(food,0x350);
    void* eat=Call<void*>(0x1529290,content,Literal(u"Strings\\StringsFromCSFiles:Game1.cs.3159"),name);
    bool match=SameText(prompt,eat);
    if(!match) {
        void* drink=Call<void*>(0x1529290,content,Literal(u"Strings\\StringsFromCSFiles:Game1.cs.3160"),name);
        match=SameText(prompt,drink);
    }
    if(!match || Menu()!=menu || Player()!=player || Read<void*>(player,0x3B0)!=food) return false;
    void* responses=Read<void*>(menu,0x80);
    // Managed arrays: length via bounds pointer, elements via data pointer.
    if(!responses) return false;
    void* bounds=Read<void*>(responses,0x20);
    void* data=Read<void*>(responses,0x10);
    if(!bounds || !data || Read<int>(bounds,0)!=2) return false;
    void* yes=Read<void*>(data,0);
    if(!yes) return false;
    if(!Virtual<bool>(Location(),0x790,yes)) return false;
    if(Menu()==menu) Call<void>(0x16CF890,menu);
    return true;
}
// Every repeated update rechecks the current menu and its busy state. Direct
// calls use the exact concrete body, leaving original completion logic intact.
std::uintptr_t MenuAnimation(void* menu) {
    if(Is(menu,0xDDFDE88)) return Read<int>(menu,0xE8)>0 ? 0x16F00B0 : 0;
    if(Is(menu,0xDDFD588)) return Call<bool>(0x16E4A10,menu) ? 0x16E8210 : 0;
    if(Is(menu,0xDE08A60)) return Call<bool>(0x17A1DC0,menu) ? 0x17A5A30 : 0;
    if(Is(menu,0xDE04E88)) return Read<bool>(menu,0x94)||Read<bool>(menu,0x95) ? 0x1756A20 : 0;
    if(Is(menu,0xDE0A7E0)) return Read<double>(menu,0x70)>0 ? 0x17C0BB0 : 0;
    if(Is(menu,0xDDFB2D8)) {
        if(Read<bool>(menu,0xDA)) return 0x16D1ED0;
        void* text=Call<void*>(0x16CFCA0,menu);
        return text && Read<int>(menu,0xC4)<Read<int>(text,0x10) ? 0x16D1ED0 : 0;
    }
    // ShippingMenu private fields verified in update and CanReceiveInput.
    // Never advance the save sub-menu, even after its completion flag changes.
    if(Is(menu,0xDE06CA8)) return !Read<bool>(menu,0x118) && !Read<void*>(menu,0x110) &&
        (Read<int>(menu,0xDC)>0 || Read<bool>(menu,0x100)) ? 0x176FF20 : 0;
    if(Is(menu,0xDE099A0)) return Read<bool>(menu,0x1A9) ? 0x17B64F0 : 0;
    return 0;
}
void Menus(void* time,unsigned extra) {
    void* original=Menu();
    if(Is(original,0xDDF65F0)) {
        // Only animate PERFECT text; do not run the fishing minigame twice.
        for(unsigned i=0;i<extra && Menu()==original && Safe();++i) {
            void* sparkle=Read<void*>(original,0x118);
            if(!sparkle) break;
            if(Call<bool>(0x1138D60,sparkle,time) && Menu()==original && Read<void*>(original,0x118)==sparkle)
                Write<void*>(original,0x118,nullptr);
        }
        return;
    }
    for(unsigned i=0;i<extra && Safe() && Menu()==original;++i) {
        const auto body=MenuAnimation(original);
        if(!body) break;
        Call<void>(body,original,time);
    }
    if(Is(Menu(),0xDE099A0)) {
        void* title=Menu();
        void* sub=Call<void*>(0x17B1780);
        if(Is(sub,0xDE00F68)) for(unsigned i=0;i<extra && Safe() && Menu()==title &&
            Call<void*>(0x17B1780)==sub && Read<int>(sub,0x7C)>0;++i)
            Call<void>(0x1733DB0,sub,time);
    }
}
// List<AnimationFrame>.get_Item returns a 40-byte value through x8. Reading
// its owned array avoids inventing a pointer-return ABI for that value type.
struct Frame { int number{-1},milliseconds{}; };
Frame AnimationFrame(void* sprite,int index=0) {
    void* list=sprite ? Read<void*>(sprite,0x70) : nullptr;
    if(!list || index<0 || Read<int>(list,0x18)<=index) return {};
    void* array=Read<void*>(list,0x10);
    if(!array) return {};
    void* bounds=Read<void*>(array,0x20);
    void* data=Read<void*>(array,0x10);
    if(!bounds || !data || Read<int>(bounds,0)<=index) return {};
    return {Read<int>(data,40*index),Read<int>(data,40*index+4)};
}
enum class Action { None,Eating,Fishing,Tool,Slingshot,Harvest,Milk,Shear,Flute,Book,Totem,Hold,Mount };
template<class R,class... Args> R Interface(void* object,std::uintptr_t root,int slot,Args... args) {
    void* entry=Call<void*>(0xE30,object,Static<void*>(root),slot);
    auto function=Read<std::uintptr_t>(entry,0);
    auto* self=reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(object)+Read<std::uint8_t>(entry,8));
    return reinterpret_cast<R(*)(void*,Args...)>(function)(self,args...);
}
bool ContainsTotem(void* text) {
    if(!text) return false;
    const int n=Read<int>(text,0x10);
    if(n<5 || n>256) return false;
    for(int i=0;i<=n-5;++i) {
        bool match=true;
        for(int j=0;j<5;++j) if(Read<char16_t>(text,0x14+2*(i+j))!=u"Totem"[j]) match=false;
        if(match) return true;
    }
    return false;
}
void FluteDelay(int elapsed) {
    // Exact cached delegate created by Object.performUseAction at 0x1934844.
    // Only its own timer is shortened, not unrelated global delayed actions.
    void* behavior=Static<void*>(0xE283768);
    void* list=Static<void*>(0xE27FAF8);
    if(!behavior || !list || !Static<void*>(0xF812E60) || !Static<void*>(0xF820B48)) return;
    const int count=Interface<int>(list,0xF812E60,0);
    if(count<0 || count>256) return;
    for(int i=0;i<count;++i) {
        void* action=Interface<void*>(list,0xF820B48,0,i);
        if(action && Read<void*>(action,0x40)==behavior)
            Write(action,0x10,ReducePause(Read<int>(action,0x10),elapsed));
    }
}
bool SameRect(Rectangle a,Rectangle b) {
    return a.x==b.x && a.y==b.y && a.width==b.width && a.height==b.height;
}
// The list owns its sprites. Remove completed sprites immediately so an end
// callback cannot run a second time during the next ordinary game update.
void SpriteStep(void* list,void* sprite,void* time) {
    if(Call<bool>(0x1A4B170,sprite,time) && Call<bool>(0x1A4D4D0,list,sprite))
        Call<bool>(0x1A4D740,list,sprite);
}
void FishingSprites(void* time,unsigned extra) {
    void* p=Player();
    void* rod=Call<void*>(0x1318EC0,p);
    if(!Is(rod,0xDE535A0)) return;
    for(unsigned pass=0;pass<extra && WorldSafe();++pass) {
        for(unsigned group=0;group<2;++group) {
            if(Call<void*>(0x1318EC0,p)!=rod) return;
            void* list=group ? Read<void*>(rod,0x1A8) : Static<void*>(0xE27F9C0);
            if(!list) continue;
            int n=Call<int>(0x1A4D190,list);
            if(n<0 || n>512) continue;
            for(int i=n-1;i>=0 && WorldSafe();--i) {
                if(i>=Call<int>(0x1A4D190,list)) continue;
                void* s=Call<void*>(0x1A4D0A0,list,i);
                if(!s) continue;
                bool selected=!group && Read<int>(s,0x60)==987654321;
                if(group) {
                    const auto r=Read<Rectangle>(s,0xF8);
                    selected=(Equals(Read<void*>(s,0xE8),u"LooseSprites\\Cursors") && SameRect(r,{64,1920,128,32})) ||
                        (Equals(Read<void*>(s,0xE8),u"LooseSprites\\Cursors_1_6") && SameRect(r,{256,75,128,32}));
                }
                if(selected) SpriteStep(list,s,time);
            }
        }
    }
}
void ItemSprite(void* p,void* item,void* time) {
    if(!item) return;
    void* id=Call<void*>(0x150EAE0,item);
    if(Equals(id,u"(O)434")) return; // Stardrop has its own sequence.
    void* data=Call<void*>(0x15198D0,id);
    if(!data) return;
    void* texture=Call<void*>(0x1521930,data);
    const auto rect=Call<Rectangle>(0x1521AF0,data,0,std::uint64_t{0});
    void* list=Call<void*>(0x1425D70,Location());
    if(!list) return;
    const int count=Call<int>(0x1A4D190,list);
    if(count<0 || count>512) return;
    for(int i=count-1;i>=0;--i) {
        void* s=Call<void*>(0x1A4D0A0,list,i);
        if(s && texture && Call<void*>(0x1A45A60,s)==texture && SameRect(Read<Rectangle>(s,0xF8),rect)) {
            SpriteStep(list,s,time); return;
        }
    }
}
Action PlayerAction(void* p) {
    if(Read<bool>(p,0x650)) return Action::Eating;
    void* horse=Call<void*>(0x13183A0,p);
    if(Read<bool>(p,0x398) || (horse && NetBool(horse,0x4B8))) return Action::Mount;
    void* tool=Call<void*>(0x1318EC0,p);
    if(tool && Call<bool>(0x1318EA0,p)) {
        if(Is(tool,0xDE535A0)) return !Read<bool>(tool,0x18C) && !Read<bool>(tool,0x188) ? Action::Fishing : Action::None;
        if(Is(tool,0xDE59058)) return Read<double>(tool,0x130)>16.666 ? Action::Slingshot : Action::None;
        // Dagger special uses its original fixed multi-hit sequence.
        if(Is(tool,0xDE55AF8)) return Read<bool>(tool,0x180) && NetInt(tool,0x128)==3 ? Action::None : Action::Tool;
        return !Call<bool>(0x1340690,p) ? Action::Tool : Action::None;
    }
    void* sprite=Call<void*>(0x13195F0,p);
    const auto frame=AnimationFrame(sprite);
    const int animation=sprite ? Read<int>(sprite,0xA8) : -1;
    if(frame.number>=0 && animation>=279 && animation<=282) return Action::Harvest;
    if(frame.number>=0 && animation>=287 && animation<=290) return Action::Milk;
    if(frame.number>=0 && animation>=283 && animation<=286) return Action::Shear;
    if(frame.number==98) return Action::Flute;
    if(frame.number==57 && frame.milliseconds==1000) return Action::Book;
    if(frame.number==57 && frame.milliseconds==2000) {
        void* item=Call<void*>(0x1318FD0,p);
        if(item && ContainsTotem(Virtual<void*>(item,0x360))) return Action::Totem;
    }
    if(frame.number==57 && frame.milliseconds==0) {
        const auto next=AnimationFrame(sprite,1),last=AnimationFrame(sprite,2);
        if(next.number==57 && next.milliseconds==2500 && last.milliseconds==500) return Action::Hold;
    }
    return Action::None;
}
void PlayerAnimations(void* time,unsigned extra) {
    void* p=Player(); void* location=Location();
    const auto action=PlayerAction(p);
    if(action==Action::None) return;
    for(unsigned i=0;i<extra && WorldSafe() && !Menu() && Player()==p && Location()==location &&
        PlayerAction(p)==action;++i) {
        if(action==Action::Slingshot) {
            void* tool=Call<void*>(0x1318EC0,p);
            const double start=Read<double>(tool,0x130);
            const double step=static_cast<double>(Read<std::int64_t>(time,0x18))/10000.0;
            // Advance the pull's origin, not the global GameTime or aim input.
            Write(tool,0x130,start>step ? start-step : 0.001);
            continue;
        }
        if(action==Action::Mount) {
            Virtual<void>(p,0x300,time,location);
            if(!WorldSafe() || Location()!=location || Player()!=p) break;
            void* horse=Call<void*>(0x13183A0,p);
            if(horse) Call<void>(0x1199CA0,horse,time,location);
            continue;
        }
        void* item=action==Action::Eating ? Read<void*>(p,0x3B0) :
            (action==Action::Harvest || action==Action::Hold ? Read<void*>(p,0x3A8) :
                (action==Action::Totem ? Call<void*>(0x1318FD0,p) : nullptr));
        // Match the visible item before Farmer.Update may consume or replace it.
        if(item) ItemSprite(p,item,time);
        if(!WorldSafe() || Menu() || Player()!=p || Location()!=location || PlayerAction(p)!=action) break;
        if(action==Action::Flute) FluteDelay(static_cast<int>(Read<std::int64_t>(time,0x18)/10000));
        if(action==Action::Totem && item && Equals(Call<void*>(0x150EAE0,item),u"(O)681")) {
            // Rain totem pause, without advancing the global updatePause routine.
            auto* pause=reinterpret_cast<float*>(Address(0xE27F8EC));
            const float step=static_cast<float>(Read<std::int64_t>(time,0x18)/10000);
            if(*pause>0) *pause=*pause>step ? *pause-step : 0.001f;
        }
        Call<void>(0x133ED90,p,time,location);
        if((action==Action::Book || action==Action::Flute || action==Action::Hold) &&
            WorldSafe() && Player()==p && Location()==location && PlayerAction(p)==action)
            Write(p,0x704,ReducePause(Read<int>(p,0x704),static_cast<int>(Read<std::int64_t>(time,0x18)/10000)));
    }
}
void WorldAnimations(void* time,unsigned extra) {
    void* location=Location();
    for(unsigned i=0;i<extra && WorldSafe() && Location()==location && !Menu();++i) {
        if(Is(location,0xDDCEC10) && (Read<bool>(location,0x338)||Read<bool>(location,0x339)))
            Call<void>(0x153C470,location,time);
        else if(Is(location,0xDDD4990) && (Read<bool>(location,0x328)||Read<bool>(location,0x329)))
            Call<void>(0x155AA90,location,time);
        if(!WorldSafe() || Location()!=location || Menu()) return;
        void* platform=Static<void*>(0xE27D7C8);
        if(platform && Read<int>(platform,0x38)>0) Call<void>(0x111D100,platform,time);
    }
    if(!WorldSafe() || Location()!=location || Menu()) return;
    // Reacquire dictionary views after each potentially mutating animation.
    // Only native indices persist; never cache managed arrays across callbacks.
    auto objects=Game::GameState::GetObjects(location);
    const auto count=objects.count<4096 ? objects.count : 4096;
    for(unsigned index=0;objects.readable && index<count && WorldSafe() && Location()==location && !Menu();++index) {
        if(!objects.IsActive(index)) continue;
        void* chest=objects.values[index];
        if(!Is(chest,0xDE30070) || NetInt(chest,0x238)<=0) continue;
        for(unsigned i=0;i<extra && WorldSafe() && Location()==location && !Menu() && NetInt(chest,0x238)>0;++i)
            Call<void>(0x19613F0,chest,time);
        objects=Game::GameState::GetObjects(location);
    }
    auto terrain=Game::GameState::GetTerrainFeatures(location);
    const auto terrainCount=terrain.count<8192 ? terrain.count : 8192;
    for(unsigned index=0;terrain.readable && index<terrainCount && WorldSafe() && Location()==location && !Menu();++index) {
        void* tree=terrain.Get(index);
        unsigned field=Is(tree,0xDE50908) ? 0xA8 : (Is(tree,0xDE4F3A0) ? 0xA0 : 0);
        if(!field || !NetBool(tree,field)) continue;
        const auto body=field==0xA8 ? 0x1A75550 : 0x1A58CA0;
        for(unsigned i=0;i<extra && WorldSafe() && Location()==location && !Menu() &&
            Call<void*>(0x1A71E70,tree)==location && NetBool(tree,field);++i)
            if(Call<bool>(body,tree,time)) break;
        terrain=Game::GameState::GetTerrainFeatures(location);
    }
}
void Fade(void* time,unsigned extra) {
    if(!Call<bool>(0x139CD40) || Menu() || Static<bool>(0xE27F6C3) || Static<bool>(0xE27F6B8)) return;
    const float alpha=Call<float>(0x139CE40);
    const float next=AdvanceFade(alpha,Call<bool>(0x139CD80),
        static_cast<int>(Read<std::int64_t>(time,0x18)/10000),extra);
    Call<void>(0x139CE60,next);
}
}
void Update() {
    static bool checked{},compatible{};
    if(!checked) {
        checked=true; compatible=true;
        for(const auto& s:Signatures) if(Static<std::uint32_t>(s.offset)!=s.word) compatible=false;
        Logging.Log("[FastAnimations] exact-build signatures=%s; no additional hooks",compatible ? "ok" : "FAILED (disabled)");
    }
    if(!compatible) return;
    UIInfo::RefreshSettings();
    if(!UIInfo::Enabled(UIInfo::Option::FastAnimations) || !Safe()) return;
    void* time=Static<void*>(0xE27FAF0);
    if(!time) return;
    const auto elapsed=Read<std::int64_t>(time,0x18);
    if(elapsed<=0 || elapsed>1000000) return; // no catch-up bursts after a stall
    const unsigned extra=ExtraUpdates(true,UIInfo::Enabled(UIInfo::Option::FastAnimationsTriple));
    void* minigame=Call<void*>(0x139ED10);
    if(minigame) {
        if(Is(minigame,0xDE11F68)) for(unsigned i=0;i<extra && Safe() &&
            Call<void*>(0x139ED10)==minigame && Read<bool>(minigame,0x40);++i)
            if(Call<bool>(0x184DAE0,minigame,time)) break;
        return;
    }
    if(Static<bool>(0xE27F6BC) || Call<void*>(0x139EBE0)) {
        if(Is(Menu(),0xDE0A7E0)) Menus(time,extra);
        return;
    }
    if(AcceptFood()) return;
    if(Menu()) { Menus(time,extra); return; }
    if(!WorldSafe() || Static<bool>(0xE27F6D6) || Static<bool>(0xE27F6C3)) return;
    void* location=Location();
    const bool playerAnimating=PlayerAction(Player())!=Action::None;
    PlayerAnimations(time,extra);
    if(!WorldSafe() || Menu() || Location()!=location || playerAnimating) return;
    FishingSprites(time,extra);
    if(!WorldSafe() || Menu()) return;
    WorldAnimations(time,extra);
    if(WorldSafe() && !Menu()) Fade(time,extra);
}
}
