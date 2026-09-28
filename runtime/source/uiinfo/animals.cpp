#include "uiinfo/animals.hpp"
#include "uiinfo/aot.hpp"
#include "uiinfo/ui_info_suite.hpp"
#include "uiinfo/settings.hpp"
namespace AutomateLite::UIInfo {
namespace {
using namespace Aot;
struct Marker { Game::TilePosition pixel{}; bool needsPet{}; bool produce{}; TextBuffer<160> detail; };
Marker g_Markers[128];
unsigned g_Count{};
bool PetNeedsPet(void* pet) {
    void* farmer=Game::GameState::GetPlayer();
    void* owner=Read<void*>(pet,0x4D8);
    void* dictionary=owner ? Read<void*>(owner,0x48) : nullptr;
    if (!farmer || !dictionary) return false;
    const auto id=Call<std::int64_t>(0x13180C0,farmer);
    void* field{};
    if (!Call<bool>(0xDDD5B0,dictionary,id,&field)) return true;
    if (!field) return false;
    const int lastDay=Virtual<int>(owner,0x210,field);
    void* date=Call<void*>(0x139F550);
    return date && lastDay!=Call<int>(0x1B08AF0,date);
}
void Record(void* animal,bool pet) {
    if (!CanDrawOverlay() || !Enabled(Option::Animals) || g_Count>=128) return;
    const int friendship=NetInt(animal,pet ? 0x4E8 : 0x1C8);
    const bool needsPet=pet ? PetNeedsPet(animal) : !NetBool(animal,0x220);
    void* produce=nullptr;
    if (!pet) {
        auto* field=Read<void*>(animal,0x1C0);
        produce=field ? Read<void*>(field,0x58) : nullptr;
        if (Equals(produce,u"") || Equals(produce,u"-1")) produce=nullptr;
    }
    const bool showPet=needsPet && !(Enabled(Option::HideFullAnimals) && friendship>=1000);
    if (!showPet && !produce) return;
    Marker marker{};
    marker.pixel=Call<Game::TilePosition>(0x1188350,animal);
    marker.needsPet=showPet; marker.produce=produce!=nullptr;
    Append(marker.detail,Virtual<void*>(animal,0x80));
    if (showPet) marker.detail.Append(u"\n今天还未抚摸");
    marker.detail.Append(u"\n好感："); marker.detail.AppendNumber(friendship); marker.detail.Append(u" / 1000");
    if (produce) { marker.detail.Append(u"\n可收取："); ItemName(marker.detail,produce); }
    g_Markers[g_Count++]=marker;
}
HOOK_DEFINE_TRAMPOLINE(AnimalDrawHook) {
    static void Callback(void* animal,void* batch) { Orig(animal,batch); Record(animal,false); }
};
HOOK_DEFINE_TRAMPOLINE(PetDrawHook) {
    static void Callback(void* pet,void* batch) { Orig(pet,batch); Record(pet,true); }
};
}
void ClearAnimalMarkers() { g_Count=0; }
void DrawAnimalMarkers(void* batch) {
    using namespace Aot;
    const int mouseX=Call<int>(0x13C3920,true),mouseY=Call<int>(0x13C3A40,true);
    void* viewport=reinterpret_cast<void*>(Address(0xE27F4C8));
    const int width=Call<int>(0x1D8EF70,viewport),height=Call<int>(0x1D8EF50,viewport);
    for(unsigned i=0;i<g_Count;++i) {
        const auto& marker=g_Markers[i];
        auto position=Call<Game::TilePosition>(0x13E6E80,marker.pixel);
        position=Call<Game::TilePosition>(0x1AFDBB0,position);
        const int x=static_cast<int>(position.x),y=static_cast<int>(position.y)-32;
        if(x < -64 || y < -64 || x > width || y > height) continue;
        TextBuffer<32> icon;
        if(marker.needsPet) icon.Append(u"♥");
        if(marker.produce) icon.Append(u"！");
        Text(batch,icon,static_cast<float>(x),static_cast<float>(y),0xFF80C0FF,0.8f);
        if(mouseX>=x-8 && mouseX<=x+64 && mouseY>=y-8 && mouseY<=y+100) {
            int left=x+48,top=y;
            if(left+350>width-8)left=width-358;
            if(top+112>height-8)top=height-120;
            if(left<8)left=8;
            if(top<8)top=8;
            Box(batch,{left,top,350,112},0xE6202018);
            Text(batch,marker.detail,static_cast<float>(left+8),static_cast<float>(top+8),0xFFFFFFFF,0.7f);
        }
    }
    g_Count=0;
}
void InstallAnimalHooks() {
    using namespace Aot;
    if(Static<std::uint32_t>(0x1308D50)==0xD10383FFu) AnimalDrawHook::InstallAtOffset(0x1308D50);
    else Logging.Log("[UIInfoSuite2] animal draw fingerprint mismatch");
    if(Static<std::uint32_t>(0x11AF830)==0xD104C3FFu) PetDrawHook::InstallAtOffset(0x11AF830);
    else Logging.Log("[UIInfoSuite2] pet draw fingerprint mismatch");
}
}
