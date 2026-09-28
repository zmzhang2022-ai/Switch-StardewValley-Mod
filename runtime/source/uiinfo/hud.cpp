#include "uiinfo/hud.hpp"
#include "uiinfo/aot.hpp"
#include "uiinfo/logic.hpp"
#include "uiinfo/settings.hpp"
#include "uiinfo/friendship.hpp"
#include "uiinfo/characters.hpp"

namespace AutomateLite::UIInfo {
namespace {
using namespace Aot;
struct Gain { int skill{-1}; int experience{}; int delta{}; int previousLevel{}; std::uint64_t time{}; };
Gain g_Gain;
struct LevelNotice { int skill{-1},level{}; std::uint64_t time{}; };
LevelNotice g_Level;
std::uint64_t g_XpSave{};
TextBuffer<128> g_CurrentItem;
int g_PendingSkill{-1};
std::uint64_t g_Save{};
TextBuffer<2048> g_Daily;
std::uint64_t g_NextRefresh{};
std::uint64_t g_MerchantSave{};
int g_MerchantDay{-1};
const char16_t* SkillName(int skill) {
    constexpr const char16_t* names[] = {u"耕种",u"钓鱼",u"采集",u"采矿",u"战斗"};
    return skill >= 0 && skill < 5 ? names[skill] : u"技能";
}
int Experience(void* farmer, int skill) {
    // Exact vanilla gainExperience +0x270: NetArray +0x48 owns List<NetInt>.
    // Concrete list getter at 0xDC3820 checks count and backing array bounds.
    auto* array = Read<void*>(farmer,0x1D8);
    auto* list = array ? Read<void*>(array,0x48) : nullptr;
    if (!list || skill < 0 || skill >= Read<int>(list,0x18)) return -1;
    void* field = Call<void*>(0xDC3820,list,skill);
    return field ? Call<int>(0x10EBB30,field) : -1;
}
HOOK_DEFINE_TRAMPOLINE(ExperienceHook) {
    static void Callback(void* farmer, int which, int amount) {
        const bool local = farmer && farmer == Game::GameState::GetPlayer() && which >= 0 && which < 5;
        const int before = local ? Experience(farmer,which) : -1;
        Orig(farmer,which,amount);
        if (before < 0) return;
        const int after = Experience(farmer,which);
        if (after <= before) return;
        const auto save=Static<std::uint64_t>(0xE27FA00);
        if(g_XpSave!=save) { g_Gain={}; g_Level={}; g_XpSave=save; g_CurrentItem={}; g_PendingSkill=-1; }
        const auto now=Milliseconds();
        const bool accumulate=g_Gain.skill==which && now-g_Gain.time<1000;
        g_Gain = {which,after,(accumulate ? g_Gain.delta : 0)+after-before,
            accumulate ? g_Gain.previousLevel : Logic::LevelFromExperience(before),now};
        if(Enabled(Option::LevelUp) && Logic::LevelFromExperience(after)>Logic::LevelFromExperience(before)) {
            g_Level={which,Logic::LevelFromExperience(after),now};
            // String + out ICue overload, invoker 728EA88 passes an address.
            void* cue{}; Call<bool>(0x13A78D0,Literal(u"newRecipe"),&cue);
        }
    }
};
const char16_t* WeatherName(void* weather) {
    if (Equals(weather,u"Rain")) return u"雨";
    if (Equals(weather,u"GreenRain")) return u"绿雨";
    if (Equals(weather,u"Storm")) return u"雷雨";
    if (Equals(weather,u"Snow")) return u"雪";
    if (Equals(weather,u"Wind")) return u"风";
    if (Equals(weather,u"Festival")) return u"节日";
    if (Equals(weather,u"Wedding")) return u"婚礼";
    if (Equals(weather,u"Sun")) return u"晴";
    return u"未知";
}
void LineBreak(TextBuffer<2048>& text) { if (text.Size()) text.Append(u"\n"); }
int TotalDays() { void* date=Call<void*>(0x139F550); return date ? Call<int>(0x1B08AF0,date) : -1; }
void AppendRecipe(void* farmer,TextBuffer<2048>& text) {
    const int day=Static<int>(0xE27F7A8);
    if(day%7!=0 && day%7!=3) return;
    void* stats=Call<void*>(0x139DC70);
    if(!stats) return;
    const auto played=Call<unsigned>(0x1A3A160,stats);
    if(played<=5) return;
    int week=(played%224)/7;
    if(played%224==0) week=32;
    if(day%7==3) {
        void* team=Call<void*>(0x1317270,farmer);
        if(!team) return;
        if(NetInt(team,0x208)==TotalDays()) week=NetInt(team,0x210);
        // getRerunWeek only initializes the recipe lookup cache and creates
        // a fresh day/save-seeded RNG. It does not teach a recipe or write
        // team rerun NetInts. Full body audited in collection-tv-rerun.asm.
        else week=Call<int>(0x199D2D0,static_cast<void*>(nullptr));
    }
    if(week<1 || week>32) return;
    void* content=Call<void*>(0x139C0C0);
    void* channel=content ? Call<void*>(0x11D3160,content) : nullptr;
    if(!channel) return;
    TextBuffer<16> key; key.AppendNumber(week);
    void* entry{};
    if(!Call<bool>(0x3E6B30,channel,Managed(key),&entry) || !entry) return;
    const int length=Read<int>(entry,0x10);
    if(length<1 || length>8192) return;
    TextBuffer<192> recipe;
    for(int i=0;i<length;++i) {
        const auto c=Read<char16_t>(entry,0x14+i*2);
        if(c==u'/') break;
        if(!recipe.Append(&c,1)) return;
    }
    if(recipe.Size() && !Call<bool>(0x13334A0,farmer,Managed(recipe))) {
        LineBreak(text); text.Append(u"电视可学新菜谱："); text.Append(recipe.Data());
    }
}
void AppendIslandWeather(TextBuffer<2048>& text) {
    void* root=Static<void*>(0xE27FB48);
    void* state=root ? Read<void*>(root,0x58) : nullptr;
    void* owner=state ? Read<void*>(state,0xA0) : nullptr;
    void* dictionary=owner ? Read<void*>(owner,0x48) : nullptr;
    void* field{};
    // GetWeatherForLocation creates a NetField when absent. Read only its
    // successful lookup branch, so this overlay never changes world weather.
    if(!dictionary || !Call<bool>(0x18E43E0,dictionary,Literal(u"Island"),&field) || !field) return;
    void* weather=Virtual<void*>(owner,0x210,field);
    if(weather) { LineBreak(text); text.Append(u"姜岛明日："); text.Append(WeatherName(Call<void*>(0x18C3EE0,weather))); }
}
}

void ObserveMerchantShop(void* shop) {
    using namespace Aot;
    if(!shop) return;
    // Forest.checkAction at 0x158F5F0 passes this exact game-owned shop ID.
    void* id=**reinterpret_cast<void***>(Address(0xDFEA450));
    TextBuffer<96> name; if(!id || !Append(name,id)) return;
    if(Equals(Read<void*>(shop,0x90),name.Data())) {
        g_MerchantSave=Static<std::uint64_t>(0xE27FA00); g_MerchantDay=TotalDays(); g_NextRefresh=0;
    }
}

void UpdateDailyHud(void* farmer,TextBuffer<2048>& text) {
    using namespace Aot;
    const double luck = Call<double>(0x1318710,farmer);
    if (Enabled(Option::Luck) && luck >= -1.0 && luck <= 1.0) {
        text.Append(u"运势 ");
        text.Append(luck >= 0.07 ? u"极好" : luck > 0.02 ? u"好" : luck < -0.07 ? u"极差" : luck < -0.02 ? u"差" : u"中性");
        if(Enabled(Option::ExactLuck)) {
        text.Append(u"（");
        // Four decimal places preserve the game's actual daily-luck value.
        const int value = static_cast<int>(luck * 10000.0 + (luck >= 0 ? 0.5 : -0.5));
        if (value < 0) text.Append(u"-");
        const int absolute = value < 0 ? -value : value;
        text.AppendNumber(absolute/10000); text.Append(u".");
        for (int divisor=1000;divisor;divisor/=10) text.AppendNumber(absolute/divisor%10);
        text.Append(u"）");
        }
    }
    void* weather = Static<void*>(0xE27F710);
    if (weather && Enabled(Option::Weather)) {
        text.Append(u"   明日："); text.Append(WeatherName(weather));
        AppendIslandWeather(text);
    }
    const int season=Static<int>(0xE27F720), day=Static<int>(0xE27F7A8);
    if (Enabled(Option::Berries) && Logic::IsBerrySeason(season,day)) {
        LineBreak(text); text.Append(season == 0 ? u"美洲大树莓采摘季" : u"黑莓采摘季");
    }
    if (Enabled(Option::Hazelnuts) && season == 2 && day >= 15) { LineBreak(text); text.Append(u"榛子季"); }
    // Exact vanilla merchant predicate is day % 7 == 0 or 5, verified in
    // Forest.ShouldTravelingMerchantVisitToday; no location is created here.
    const bool visited=g_MerchantSave==Static<std::uint64_t>(0xE27FA00) && g_MerchantDay==TotalDays();
    if (Enabled(Option::Merchant) && !(Enabled(Option::HideVisitedMerchant) && visited) && day > 0 && (day%7 == 0 || day%7 == 5)) {
        LineBreak(text); text.Append(visited ? u"旅行货车今日营业（已访问）" : u"旅行货车今日营业");
    }
    if(Enabled(Option::Recipes)) AppendRecipe(farmer,text);
    if(Enabled(Option::Birthday)) VisitVillagers([&](void* birthday) {
        if(!Call<bool>(0x18FFCD0,birthday)) return;
        if(Enabled(Option::HideFullBirthday) && Call<int>(0x131FE40,farmer,Call<void*>(0x1188600,birthday))>=
            Call<int>(0x1AD7950,birthday)*250) return;
        LineBreak(text); text.Append(u"今日生日："); Append(text,Virtual<void*>(birthday,0x80));
        if(Enabled(Option::Gifts)) {
            void* friendship=Friendship(farmer,Call<void*>(0x1188600,birthday));
            if(friendship) text.Append(Call<int>(0x1398000,friendship)>0 ? u"（已送礼）" : u"（今日未送礼）");
        }
    });
    auto* toolRef=Read<void*>(farmer,0x5B0);
    auto* tool=toolRef ? Read<void*>(toolRef,0x58) : nullptr;
    if (tool && Enabled(Option::ToolUpgrade)) {
        LineBreak(text); Append(text,Virtual<void*>(tool,Offsets::DisplayNameSlot));
        const int days=NetInt(farmer,0x5B8);
        if (days > 0) { text.Append(u"升级还需 "); text.AppendNumber(days); text.Append(u" 天"); }
        else text.Append(u"升级完成，可以领取");
    }
    const int houseDays = NetInt(farmer,0x5C8);
    if (houseDays > 0 && Enabled(Option::BuildingProgress)) { LineBreak(text); text.Append(u"住宅升级还需 "); text.AppendNumber(houseDays); text.Append(u" 天"); }
    if(Enabled(Option::BuildingProgress)) {
        void* building=Call<void*>(0x13C51A0,Literal(u"Robin"));
        if(building) {
            const int construction=NetInt(building,0x70),upgrade=NetInt(building,0x78);
            if(construction>0 || upgrade>0) {
                LineBreak(text); text.Append(u"罗宾施工还需 ");
                text.AppendNumber(construction>0 ? construction : upgrade); text.Append(u" 天");
            }
        }
    }
}
void DrawDailyHud(void* batch) {
    using namespace Aot;
    void* farmer=Game::GameState::GetPlayer();
    if(!farmer || !batch) return;
    const auto save=Static<std::uint64_t>(0xE27FA00), now=Milliseconds();
    if(g_Save!=save) { g_Save=save; g_NextRefresh=0; }
    if(g_XpSave!=save) { g_Gain={}; g_Level={}; g_XpSave=save; g_CurrentItem={}; g_PendingSkill=-1; }
    if(now>=g_NextRefresh) {
        g_Daily={}; UpdateDailyHud(farmer,g_Daily); g_NextRefresh=now+1000;
    }
    Text(batch,g_Daily,24.0f,24.0f,0xFFFFFFFF,0.75f);
    if(Enabled(Option::LevelUp) && g_Level.skill>=0 && now-g_Level.time<2000) {
        const auto age=now-g_Level.time;
        auto position=Call<Game::TilePosition>(0x1188350,farmer);
        position=Call<Game::TilePosition>(0x13E6E80,position);
        position=Call<Game::TilePosition>(0x1AFDBB0,position);
        TextBuffer<96> levelText; levelText.Append(SkillName(g_Level.skill));
        levelText.Append(u"升级！ Lv."); levelText.AppendNumber(g_Level.level);
        const auto alpha=static_cast<unsigned>(age<1500 ? 255 : (2000-age)*255/500);
        Text(batch,levelText,position.x-60,position.y-64-static_cast<float>(age)*0.025f,
            FadeColor(0xFF80FFFF,alpha),0.85f);
    }
    void* item=Call<void*>(0x1318FD0,farmer);
    void* name=item ? Virtual<void*>(item,0x360) : nullptr;
    const bool itemChanged=name ? !Equals(name,g_CurrentItem.Data()) : g_CurrentItem.Size()!=0;
    if(itemChanged || g_Gain.skill<0) {
        g_CurrentItem={}; Append(g_CurrentItem,name);
        int skill=2;
        // Concrete metadata roots from these exact type factories; Item.Name
        // is used only for the original scythe exception, never translated text.
        if(Is(item,0xDE535A0))skill=1;
        else if(Is(item,0xDE575D0))skill=3;
        else if(Is(item,0xDE55AF8) && !Equals(name,u"Scythe"))skill=4;
        else if(!Is(item,0xDE523D8)) {
            void* location=Game::GameState::GetCurrentLocation();
            if(location && Call<bool>(0x1425D80,location))skill=0;
        }
        // Remember a tool switch during the gain's minimum display time;
        // otherwise copying the new item name would lose that switch forever.
        g_PendingSkill=skill;
    }
    if(g_PendingSkill>=0 && (g_Gain.skill<0 || now-g_Gain.time>=1000)) {
        const int xp=Experience(farmer,g_PendingSkill);
        if(xp>=0) g_Gain={g_PendingSkill,xp,0,Logic::LevelFromExperience(xp),now};
        g_PendingSkill=-1;
    }
    if(g_Gain.skill<0) {
        if(Enabled(Option::ExperienceFade)) return;
        const int xp=Experience(farmer,0);
        if(xp<0) return;
        g_Gain={0,xp,0,Logic::LevelFromExperience(xp),0};
    }
    const auto elapsed=now-g_Gain.time;
    if(Enabled(Option::ExperienceFade) && elapsed>=5000) return;
    const unsigned alpha=Enabled(Option::ExperienceFade) && elapsed>4000 ?
        static_cast<unsigned>((5000-elapsed)*255/1000) : 255;
    const int level=Logic::LevelFromExperience(g_Gain.experience);
    const int base=Logic::ExperienceThresholds[level];
    const int next=Logic::ExperienceThresholds[level < 10 ? level+1 : 10];
    const int width=level==10 ? 240 : 240*(g_Gain.experience-base)/(next-base);
    const auto viewport=reinterpret_cast<void*>(Address(0xE27F4C8));
    const int height=Call<int>(0x1D8EF50,viewport);
    const int y=height > 180 ? height-160 : 400;
    if(Enabled(Option::ExperienceBar)) {
        Box(batch,{24,y,244,12},FadeColor(0xDC181818,alpha));
        Box(batch,{26,y+2,width,8},FadeColor(0xFF40C890,alpha));
    }
    if(!Enabled(Option::ExperienceBar) && !(Enabled(Option::ExperienceGain) && elapsed<5000) &&
        !(Enabled(Option::LevelUp) && elapsed<5000 && level>g_Gain.previousLevel)) return;
    TextBuffer<256> gain;
    gain.Append(SkillName(g_Gain.skill)); gain.Append(u" Lv."); gain.AppendNumber(level);
    if(Enabled(Option::ExperienceGain) && elapsed<5000 && g_Gain.delta>0) { gain.Append(u"  +"); gain.AppendNumber(g_Gain.delta); }
    gain.Append(u"  ");
    gain.AppendNumber(g_Gain.experience); gain.Append(u" / "); gain.AppendNumber(next);
    if (Enabled(Option::LevelUp) && elapsed<5000 && level>g_Gain.previousLevel) gain.Append(u"  升级！");
    Text(batch,gain,24.0f,static_cast<float>(y-30),FadeColor(0xFFFFFFFF,alpha),0.75f);
}

void InstallExperienceHook() {
    // Complete function entry, including sub sp,sp,#0xA0; verified invoker
    // 0x725E074 and real gainExperience body, not a mid-function patch.
    if (Static<std::uint32_t>(0x1321E40) == 0xD10283FFu)
        ExperienceHook::InstallAtOffset(0x1321E40);
    else Logging.Log("[UIInfoSuite2] gainExperience fingerprint mismatch; XP hook skipped");
}
}
