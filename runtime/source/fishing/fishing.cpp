/* Copyright (C) 2026 zmzhang2022-ai | GPL-2.0-only */
#include "fishing/fishing.hpp"
#include "fishing/logic.hpp"
#include "fishing/signatures.hpp"
#include "uiinfo/aot.hpp"

// Exact 1.6.15.3 AOT bodies and consumers: docs/AUTO_FISHING_SWITCH.md,
// analysis/fishing-*.json and *.asm. No managed object survives a callback here.
namespace AutomateLite::Fishing {
namespace {
using namespace UIInfo::Aot;
bool installed{}, enabled{}, eating{}, castOutstanding{}, sawFishing{};
Chord chord;
Status status = Status::Off;
std::uint64_t lastAction{}, eatingSince{}, castSince{};
std::uint64_t locationHash{}, playerId{};
int day{}, year{}, rodSlot{-1}, foodSlot{-1}, facing{}, failedCasts{};

template<typename T> void Write(void* object, std::uintptr_t offset, T value) {
    *reinterpret_cast<T*>(reinterpret_cast<std::uintptr_t>(object)+offset)=value;
}
void* Cast(void* object, std::uintptr_t resolver) {
    return object ? Call<void*>(0xAF0,object,Call<void*>(resolver)) : nullptr;
}
void* Player() { return Call<void*>(0x139C170); }
void* Menu() { return Call<void*>(0x139DCB0); }
void* Rod(void* player) { return Cast(Call<void*>(0x1318EC0,player),0x51A2C0); }
void SetStatus(Status value) {
    if(status!=value) Logging.Log("[AutoFishing] state=%d",static_cast<int>(value));
    status=value;
}
void Stop(Status reason=Status::Off) {
    enabled=false; castOutstanding=false; sawFishing=false;
    SetStatus(reason);
}
std::uint64_t LocationHash() {
    void* location=Call<void*>(0x139C3C0);
    if(!location) return 0;
    void* name=Call<void*>(0x1425CA0,location);
    if(!name) return 0;
    int length=Read<int>(name,0x10);
    if(length<=0 || length>512) return 0;
    std::uint64_t hash=1469598103934665603ull;
    for(int i=0;i<length;++i) hash=(hash^Read<char16_t>(name,0x14+2*i))*1099511628211ull;
    return hash;
}
bool WorldSafe() {
    if(Call<std::uint8_t>(0x139CBD0)!=3 || Call<bool>(0x139F590)) return false;
    void* game=Static<void*>(0xE27FB10);
    return game && !Call<bool>(0x139DC40,game) && !Static<bool>(0xE27F6BB)
        && !Static<bool>(0xE27F6BC) && !Call<void*>(0x139EBE0);
}
bool SameSession(void* player) {
    return player && Call<std::uint64_t>(0x13180C0,player)==playerId
        && Static<int>(0xE27F7A8)==day && Static<int>(0xE27F7AC)==year
        && LocationHash()==locationHash;
}
bool Connected() {
    void* input=Static<void*>(0xE27F5C0);
    return input && Call<bool>(0xD40D0,reinterpret_cast<void*>(
        reinterpret_cast<std::uintptr_t>(input)+0x9C));
}
bool Suspended() {
    return Static<bool>(0xE27F6D6) || Static<bool>(0xE27F6C3);
}
void PollChord() {
    void* input=Static<void*>(0xE27F5C0);
    if(!input) return;
    void* pad=reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(input)+0x9C);
    if(!Call<bool>(0xD40D0,pad)) { chord.latched=true; return; }
    bool pressed=chord.Update(Call<bool>(0xD4220,pad,0x40),Call<bool>(0xD4220,pad,0x80));
    if(!pressed) return;
    if(enabled) { Stop(); return; }
    if(eating || !WorldSafe()) { SetStatus(Status::Unsupported); return; }
    void* player=Player();
    if(!player || !Rod(player) || Menu()) { SetStatus(Status::Interrupted); return; }
    locationHash=LocationHash();
    if(!locationHash) return;
    playerId=Call<std::uint64_t>(0x13180C0,player);
    day=Static<int>(0xE27F7A8); year=Static<int>(0xE27F7AC);
    rodSlot=Call<int>(0x1318CD0,player);
    enabled=true; failedCasts=0; castOutstanding=false; sawFishing=false;
    lastAction=0;
    SetStatus(Status::Ready);
}

// Original eatHeldObject calls eatObject then reduceActiveItemByOne only when
// isEating became true (0x13257D8..5800). Restore the rod after its animation.
bool TryEat(void* player, std::uint64_t now) {
    void* inventory=Call<void*>(0x1314DF0,player);
    if(!inventory) return false;
    int count=Call<int>(0x1507F50,inventory);
    int maxItems=Call<int>(0x1318540,player);
    if(count<0 || count>144 || maxItems<1 || maxItems>144) return false;
    if(count>maxItems) count=maxItems;
    int chosen=-1,best=0;
    float missing=Call<int>(0x1319360,player)-Call<float>(0x1319270,player);
    for(int i=0;i<count;++i) {
        void* item=Cast(Call<void*>(0x1507F70,inventory,i),0x50BB40);
        if(!item || Virtual<int>(item,0x390)<=0) continue;
        // Stardrop has special permanent-stat/cutscene behavior; do not trigger
        // it as a repeatable meal even though vanilla reports 999 recovery.
        if(Equals(Call<void*>(0x150EAE0,item),u"(O)434")) continue;
        int recovery=Call<int>(0x1935E90,item);
        if(BetterFood(recovery,best,missing)) { chosen=i; best=recovery; }
    }
    if(chosen<0) return false;
    facing=Call<int>(0x11885E0,player);
    foodSlot=chosen;
    Call<void>(0x1319150,player,chosen);
    Call<void>(0x1325460,player);
    eating=Read<bool>(player,0x650);
    if(!eating) {
        Call<void>(0x1319150,player,rodSlot);
        Call<void>(0x1189190,player,facing);
        return false;
    }
    eatingSince=now;
    SetStatus(Status::Eating);
    return true;
}
bool RestoreAfterEating(void* player, std::uint64_t now) {
    if(!eating) return false;
    if(Read<bool>(player,0x650)) {
        if(now-eatingSince>20000) Stop(Status::Interrupted);
        return true;
    }
    eating=false;
    // A manual tool change takes priority; do not override it.
    if(Call<int>(0x1318CD0,player)!=foodSlot) { Stop(Status::Interrupted); return true; }
    void* inventory=Call<void*>(0x1314DF0,player);
    int count=inventory?Call<int>(0x1507F50,inventory):0;
    if(rodSlot<0 || rodSlot>=count || !Cast(Call<void*>(0x1507F70,inventory,rodSlot),0x51A2C0)) {
        Stop(Status::Interrupted); return true;
    }
    Call<void>(0x1319150,player,rodSlot);
    Call<void>(0x1189190,player,facing);
    lastAction=now;
    if(enabled) SetStatus(Status::Ready);
    return true;
}

void LootTreasure(void* menu,void* rod,void* player,std::uint64_t now) {
    if(!Cast(menu,0x4FC340) || Read<int>(menu,0x130)!=3 || Read<void*>(menu,0x138)!=rod) {
        SetStatus(Status::Menu); return;
    }
    // Never interfere with an item the user has picked up on the cursor.
    if(Call<void*>(0x1742A20,menu)) { SetStatus(Status::Menu); return; }
    void* slots=Read<void*>(menu,0xC8);
    void* items=slots?Cast(Read<void*>(slots,0x98),0x8AE3E0):nullptr;
    if(!items) { Stop(Status::Interrupted); return; }
    int count=Call<int>(0x6F6D24C,items);
    if(count<0 || count>128) { Stop(Status::Interrupted); return; }
    // One transfer per callback. A partial merge remains in the source list;
    // remove it ONLY if vanilla reports the entire stack was accepted.
    for(int i=count-1;i>=0;--i) {
        void* item=Call<void*>(0x10B1B70,items,i);
        if(!item) continue;
        if(status==Status::Full && now-lastAction<1000) return;
        lastAction=now;
        if(Call<bool>(0x132DA20,player,item,false)) {
            Call<void>(0x132EA00,items,i);
            SetStatus(Status::Looting);
        } else SetStatus(Status::Full);
        return;
    }
    if(Call<bool>(0x170F080,menu)) Call<void>(0x16F59A0,menu,true);
    lastAction=now; SetStatus(Status::Ready);
}

// beginUsing can be deferred by Farmer's animation/events. Setting power only
// after pressUseToolButton misses that path. This is the actual transition:
// doStartCasting clears isTimingCast before tickUpdate consumes castingPower.
HOOK_DEFINE_TRAMPOLINE(AutoFishingStartCastingHook) {
    static void Callback(void* rod) {
        if(installed && enabled && WorldSafe() && !Suspended() && Connected()) {
            void* player=Player();
            if(SameSession(player) && Rod(player)==rod)
                Write(rod,0x17C,1.0f);
        }
        Orig(rod);
    }
};

HOOK_DEFINE_TRAMPOLINE(AutoFishingBobberUpdateHook) {
    static void Callback(void* menu,void* time) {
        bool assist=installed && enabled && WorldSafe() && !Suspended() && Connected();
        void* player=assist?Player():nullptr;
        assist=assist && SameSession(player) && Rod(player) && Menu()==menu;
        if(assist) {
            // Perfect alone leaves base quality 0 unchanged. Supply iridium to
            // vanilla pullFishFromWater; its >=gold + perfect branch retains 4.
            // Only BobberBar fish are affected, never junk/treasure loot.
            Write(menu,0xD0,4);
            Write(menu,0xC3,true);
            if(Read<bool>(menu,0xC1)) Write(menu,0xC2,true);
            if(!Read<bool>(menu,0x68)) {
                Write(menu,0x128,1.0f);
                Write(menu,0xBF,false); Write(menu,0xA0,1.0f);
                // Keep the catch perfect during the one vanilla success update.
                // Its normal fade-out stops audio, awards and calls pullFish.
                Write(menu,0xC8,568); Write(menu,0x120,0.0f); Write(menu,0x124,0.0f);
                Write(menu,0x90,200.0f); Write(menu,0x94,0.0f);
                Write(menu,0x98,0.0f); Write(menu,0x9C,200.0f);
            }
            SetStatus(Status::Catching);
        }
        Orig(menu,time);
        // No extra update calls and no writes to a menu that has been closed.
        if(assist && Menu()==menu) {
            Write(menu,0xC3,true);
            if(Read<bool>(menu,0xC1)) Write(menu,0xC2,true);
        }
    }
};
}

void Install() {
    for(const auto& signature:kSignatures) {
        for(unsigned i=0;i<4;++i) if(Static<std::uint32_t>(signature.offset+i*4)!=signature.words[i]) {
            Logging.Log("[AutoFishing] ERROR signature mismatch +%llX; disabled",
                static_cast<unsigned long long>(signature.offset));
            return;
        }
    }
    AutoFishingBobberUpdateHook::InstallAtOffset(0x167FA90);
    AutoFishingStartCastingHook::InstallAtOffset(0x1A95E80);
    installed=true;
    Logging.Log("[AutoFishing] v12.1 installed; max cast + iridium; L3+R3; default OFF");
}
void BeforeTick() {
    if(!installed) return;
    PollChord();
    if((enabled || eating) && !WorldSafe()) { Stop(Status::Interrupted); eating=false; }
}
void Update() {
    if(!installed) return;
    PollChord();
    if(!enabled && !eating) return;
    if(!WorldSafe()) { Stop(Status::Interrupted); eating=false; return; }
    void* player=Player();
    if(!SameSession(player)) { Stop(Status::Interrupted); eating=false; return; }
    const auto now=Milliseconds();
    if(RestoreAfterEating(player,now)) return;
    if(!enabled) return;
    if(!Connected()) { SetStatus(Status::Disconnected); return; }
    if(Suspended()) { SetStatus(Status::Menu); return; }
    void* rod=Rod(player);
    if(!rod || Call<int>(0x1318CD0,player)!=rodSlot) { Stop(Status::Interrupted); return; }
    void* menu=Menu();
    if(menu) {
        if(Cast(menu,0x4F8E40)) { SetStatus(Status::Catching); return; }
        LootTreasure(menu,rod,player,now); return;
    }
    if(Read<bool>(rod,0x19F)) {
        if(now-lastAction<250) return;
        lastAction=now;
        // Match tickUpdate's actual caller at 0x1A9E864..870: w2=0.
        // Bait/tackle consumption already belongs to the earlier catch path.
        Call<void>(0x1A9F390,rod,player,false);
        castOutstanding=false; failedCasts=0;
        SetStatus(Status::Looting); return;
    }
    if(Read<bool>(rod,0x188)) sawFishing=true;
    if(Read<bool>(rod,0x18A) && !Read<bool>(rod,0x189) && !Read<bool>(rod,0x19D)
        && !Read<bool>(rod,0x19C) && !Read<bool>(rod,0x1A2)) {
        if(now-lastAction<100) return;
        lastAction=now;
        void* bobber=Read<void*>(rod,0x128);
        void* location=Call<void*>(0x139C3C0);
        if(!bobber || !location) { Stop(Status::Interrupted); return; }
        int x=static_cast<int>(Call<float>(0x18D58E0,bobber));
        int y=static_cast<int>(Call<float>(0x18D59F0,bobber));
        Call<void>(0x1A96820,rod,location,x,y,1,player);
        SetStatus(Status::Catching); return;
    }
    if(Read<bool>(rod,0x18C) || Read<bool>(rod,0x18D) || Read<bool>(rod,0x18E)
        || Read<bool>(rod,0x188) || Read<bool>(rod,0x19C) || Read<bool>(rod,0x19D)
        || Read<bool>(rod,0x1A2) || Call<bool>(0x1318EA0,player)) {
        SetStatus(Read<bool>(rod,0x188)?Status::Waiting:Status::Casting);
        if(castOutstanding && now-castSince>180000) Stop(Status::Interrupted);
        return;
    }
    if(!Call<bool>(0x1318E80,player) || Read<bool>(player,0x650)) return;
    if(TooLate(Static<int>(0xE27F7B0))) { SetStatus(Status::Late); return; }
    if(now-lastAction<1000) return;
    lastAction=now;
    if(castOutstanding) {
        castOutstanding=false;
        if(!sawFishing && ++failedCasts>=3) { Stop(Status::InvalidWater); return; }
    }
    if(Call<bool>(0x1330610,player)) { SetStatus(Status::Full); return; }
    if(NeedsFood(Call<float>(0x1319270,player))) {
        if(!TryEat(player,now)) SetStatus(Status::NoFood);
        return;
    }
    if(Call<bool>(0x13DAC40)) {
        rod=Rod(player);
        if(rod && Read<bool>(rod,0x18C)) {
            Write(rod,0x17C,1.0f);
            Call<void>(0x1A9F260,rod);
        }
        castOutstanding=true; sawFishing=false; castSince=now;
        SetStatus(Status::Casting);
    } else if(++failedCasts>=3) Stop(Status::InvalidWater);
}
void Draw(void* batch) {
    if(!installed || !batch || (!enabled && status==Status::Off)) return;
    const char16_t* label=u"自动钓鱼：已关闭（L3+R3）";
    switch(status) {
    case Status::Ready: label=u"自动钓鱼：已开启（L3+R3 关闭）"; break;
    case Status::Casting: label=u"自动钓鱼：抛竿 / 收竿"; break;
    case Status::Waiting: label=u"自动钓鱼：等待咬钩"; break;
    case Status::Catching: label=u"自动钓鱼：完美捕获"; break;
    case Status::Looting: label=u"自动钓鱼：领取鱼 / 宝箱"; break;
    case Status::Eating: label=u"自动钓鱼：正在吃食物"; break;
    case Status::NoFood: label=u"自动钓鱼：暂停，体力不足且无可用食物"; break;
    case Status::Full: label=u"自动钓鱼：暂停，请清理背包"; break;
    case Status::Late: label=u"自动钓鱼：暂停，已到凌晨 1:00"; break;
    case Status::Menu: label=u"自动钓鱼：暂停，等待关闭菜单"; break;
    case Status::Disconnected: label=u"自动钓鱼：暂停，手柄已断开"; break;
    case Status::InvalidWater: label=u"自动钓鱼：已停止，请面向可钓水面后重开"; break;
    case Status::Interrupted: label=u"自动钓鱼：已停止，L3+R3 重新开启"; break;
    case Status::Unsupported: label=u"自动钓鱼：请在单人游戏中手持鱼竿开启"; break;
    default: break;
    }
    UIInfo::TextBuffer<96> text; text.Append(label);
    Box(batch,{16,170,540,36},0xDC302820);
    Text(batch,text,24,174);
}
}
