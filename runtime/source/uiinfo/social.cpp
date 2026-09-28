#include "uiinfo/social.hpp"
#include "uiinfo/aot.hpp"
#include "uiinfo/settings.hpp"
namespace AutomateLite::UIInfo {
namespace {
using namespace Aot;
HOOK_DEFINE_TRAMPOLINE(SocialHeartHook) {
    static void Callback(void* page,void* batch,int index,void* entry,int heart,bool dating,bool spouse) {
        Orig(page,batch,index,entry,heart,dating,spouse);
        if(!Enabled(Option::Hearts) || !entry || heart<0 || heart>=14) return;
        void* friendship=Read<void*>(entry,0x48);
        if(!friendship) return;
        const int points=Call<int>(0x1397FC0,friendship);
        if(points<0 || heart!=points/250 || points%250==0) return;
        if(Read<bool>(entry,0x31) && heart>=8 && !dating && !spouse) return;
        void* sprites=Read<void*>(page,0xA0);
        if(!sprites || index<0 || index>=Read<int>(sprites,0x18)) return;
        void* sprite=Call<void*>(0x166BC80,sprites,index);
        if(!sprite) return;
        // Vanilla drawNPCSlotHeart uses portrait Y +36/+64 and page X +316.
        const int x=Read<int>(page,0x28)+316+(heart%10)*32;
        const int y=Read<int>(sprite,0x14)+(heart>=10 ? 64 : 36);
        // Fill whole source columns, retaining the original heart silhouette.
        const int columns=(points%250)*7/250;
        if(!columns) return;
        NullableRectangle source{{213,428,columns,6},1,{}};
        Call<void>(0xBFF70,batch,Static<void*>(0xE27F558),Rectangle{x,y,columns*4,24},&source,0xFFFFFFFFu);
    }
};
HOOK_DEFINE_TRAMPOLINE(SocialRowHook) {
    static void Callback(void* page,void* batch,int index) {
        Orig(page,batch,index);
        if(!Enabled(Option::Gifts) && !Enabled(Option::Hearts) && !Enabled(Option::NpcMap)) return;
        void* entries=Read<void*>(page,0x98);
        if(!entries || index<0 || index>=Read<int>(entries,0x18)) return;
        void* entry=Call<void*>(0x1791B10,page,index);
        if(!entry || Read<bool>(entry,0x39) || !Read<bool>(entry,0x30)) return;
        void* friendship=Read<void*>(entry,0x48);
        if(!friendship) return;
        TextBuffer<160> text;
        if(Enabled(Option::Hearts)) {
            text.Append(u"好感 "); text.AppendNumber(Call<int>(0x1397FC0,friendship));
        }
        if(Enabled(Option::Gifts)) {
            text.Append(Call<int>(0x1398000,friendship)>0 ? u"  今日已送礼" : u"  今日未送礼");
            text.Append(u"  本周 "); text.AppendNumber(Call<int>(0x1397FD0,friendship)); text.Append(u" / 2");
        }
        const int y=Call<int>(0x1791AE0,page,index)-6;
        Text(batch,text,static_cast<float>(Read<int>(page,0x28)+316),static_cast<float>(y),0xFF302820,0.6f);
        if(!Enabled(Option::NpcMap)) return;
        const int x=Read<int>(page,0x28)+Read<int>(page,0x30)-100;
        const bool tracked=IsNpcTracked(Read<void*>(entry,0x20));
        Box(batch,{x,y-2,80,26},tracked ? 0xFF486848 : 0xFF685848);
        TextBuffer<32> label; label.Append(tracked ? u"地图 ✓" : u"地图 ×");
        Text(batch,label,static_cast<float>(x+3),static_cast<float>(y),0xFFFFFFFF,0.6f);
    }
};
HOOK_DEFINE_TRAMPOLINE(SocialClickHook) {
    static void Callback(void* page,int x,int y,bool sound) {
        const int left=Read<int>(page,0x28)+Read<int>(page,0x30)-100;
        void* entries=Read<void*>(page,0x98);
        if(Enabled(Option::NpcMap) && entries && x>=left && x<left+80) {
            const int count=Read<int>(entries,0x18),first=Read<int>(page,0xA8);
            for(int i=first;i<count && i<first+6;++i) {
                if(i<0) continue;
                const int top=Call<int>(0x1791AE0,page,i)-8;
                if(y<top || y>=top+26) continue;
                void* entry=Call<void*>(0x1791B10,page,i);
                if(entry && !Read<bool>(entry,0x39) && Read<bool>(entry,0x30)) {
                    ToggleNpcTracking(Read<void*>(entry,0x20)); return;
                }
            }
        }
        Orig(page,x,y,sound);
    }
};
}
void InstallSocialHooks() {
    if(Static<std::uint32_t>(0x1793420)==0xD10343FFu) SocialHeartHook::InstallAtOffset(0x1793420);
    else Logging.Log("[UIInfoSuite2] social heart fingerprint mismatch");
    if(Static<std::uint32_t>(0x1792840)==0xD107C3FFu) SocialRowHook::InstallAtOffset(0x1792840);
    else Logging.Log("[UIInfoSuite2] social row fingerprint mismatch");
    if(Static<std::uint32_t>(0x1792380)==0xF81C0FF7u) SocialClickHook::InstallAtOffset(0x1792380);
    else Logging.Log("[UIInfoSuite2] social selection fingerprint mismatch");
}
}
