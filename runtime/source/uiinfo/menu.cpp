#include "uiinfo/menu.hpp"
#include "uiinfo/settings.hpp"
#include "uiinfo/aot.hpp"
namespace AutomateLite::UIInfo {
namespace {
using namespace Aot;
bool g_Open{};
unsigned g_Selected{};
constexpr unsigned Options=static_cast<unsigned>(Option::Count), Rows=8;
constexpr unsigned Actions=4, Entries=Options+Actions;
Rectangle Viewport() {
    void* viewport=reinterpret_cast<void*>(Address(0xE27F4C8));
    return {0,0,Call<int>(0x1D8EF70,viewport),Call<int>(0x1D8EF50,viewport)};
}
Rectangle Panel() {
    auto screen=Viewport();
    const int width=screen.width<920 ? screen.width-24 : 896;
    return {(screen.width-width)/2,24,width,screen.height-48};
}
Rectangle EntryButton() { auto screen=Viewport(); return {16,screen.height-52,164,36}; }
bool Contains(Rectangle r,int x,int y) { return x>=r.x && y>=r.y && x<r.x+r.width && y<r.y+r.height; }
void Label(void* batch,const char16_t* value,int x,int y,float scale=0.75f) {
    TextBuffer<256> text; text.Append(value); Text(batch,text,static_cast<float>(x),static_cast<float>(y),0xFFFFFFFF,scale);
}
void Close() { if(g_Open) SaveSettings(); g_Open=false; }
void Activate(unsigned index) {
    if(index<Options) { Toggle(static_cast<Option>(index)); return; }
    if(index==Options) { SaveSettings(true); return; }
    if(index==Options+3) { Close(); return; }
    if(!Enabled(Option::Calendar)) return;
    // Vanilla GameMenu uses null-this constructors to allocate managed menus.
    // Billboard(bool) invoker/body are independently verified; no custom GC object.
    if(Static<std::uint32_t>(0x1678130)!=0xD10343FFu) return;
    Close();
    void* board=Call<void*>(0x1678130,static_cast<void*>(nullptr),index==Options+2);
    if(board) Call<void>(0x139DCC0,board);
}
void Draw(void* menu,void* batch) {
    if(!g_Open) {
        const auto r=EntryButton(); Box(batch,r,0xEF302820); Label(batch,u"UI 信息设置",r.x+8,r.y+5); return;
    }
    const auto screen=Viewport(),panel=Panel();
    Box(batch,screen,0xFA181818); Box(batch,panel,0xFF302820);
    Label(batch,u"UI Info Suite 2 — 此存档设置",panel.x+20,panel.y+16,0.9f);
    Label(batch,u"方向键选择 · 确认键切换 · 返回键保存关闭",panel.x+20,panel.y+58,0.7f);
    const unsigned page=g_Selected/Rows;
    const int step=(panel.height-164)/Rows;
    for(unsigned i=0;i<Rows;++i) {
        const unsigned index=page*Rows+i;
        if(index>=Entries) break;
        const int y=panel.y+94+static_cast<int>(i)*step;
        if(index==g_Selected) Box(batch,{panel.x+10,y-2,panel.width-20,step-2},0xFF68543A);
        TextBuffer<256> text;
        if(index<Options) {
            text.Append(Enabled(static_cast<Option>(index)) ? u"[开]  " : u"[关]  "); text.Append(OptionLabel(index));
        } else {
            constexpr const char16_t* actions[]={u"将当前设置用于新存档",u"打开日历",u"打开每日委托",u"保存并返回"};
            text.Append(actions[index-Options]);
            if((index==Options+1 || index==Options+2) && !Enabled(Option::Calendar)) text.Append(u"（开关已关闭）");
        }
        Text(batch,text,static_cast<float>(panel.x+24),static_cast<float>(y+4),0xFFFFFFFF,0.75f);
    }
    Label(batch,SettingsStatus(),panel.x+20,panel.y+panel.height-60,0.65f);
    TextBuffer<96> footer; footer.Append(u"◀ 上页    "); footer.AppendNumber(page+1);
    footer.Append(u" / "); footer.AppendNumber((Entries+Rows-1)/Rows); footer.Append(u"    下页 ▶");
    Text(batch,footer,static_cast<float>(panel.x+20),static_cast<float>(panel.y+panel.height-30),0xFFFFFFFF,0.7f);
    Call<void>(0x16F2AA0,menu,batch,true,-1);
}
HOOK_DEFINE_TRAMPOLINE(MenuDrawHook) {
    static void Callback(void* menu,void* batch) {
        RefreshSettings();
        if(!g_Open) Orig(menu,batch);
        Draw(menu,batch);
    }
};
HOOK_DEFINE_TRAMPOLINE(MenuClickHook) {
    static void Callback(void* menu,int x,int y,bool sound) {
        if(!g_Open) {
            // Preserve items held by the current vanilla page before replacing
            // its drawing/input or opening a Billboard (same PC precondition).
            if(Contains(EntryButton(),x,y) && Viewport().height>=360 && Call<bool>(0x16EE120,menu)) {
                RefreshSettings(); g_Open=true; g_Selected=0; return;
            }
            Orig(menu,x,y,sound); return;
        }
        const auto p=Panel(); const int step=(p.height-164)/Rows;
        if(y>=p.y+94 && y<p.y+94+static_cast<int>(Rows)*step && x>=p.x && x<p.x+p.width) {
            const unsigned index=(g_Selected/Rows)*Rows+static_cast<unsigned>((y-p.y-94)/step);
            if(index<Entries) { g_Selected=index; Activate(index); }
        } else if(y>=p.y+p.height-34) {
            const unsigned page=g_Selected/Rows, pages=(Entries+Rows-1)/Rows;
            g_Selected=((x<p.x+p.width/2 ? page+pages-1 : page+1)%pages)*Rows;
        }
    }
};
HOOK_DEFINE_TRAMPOLINE(MenuPadHook) {
    static void Callback(void* menu,int button) {
        if(!g_Open) { Orig(menu,button); return; }
        if(button==0x2000 || button==0x10) { Close(); return; }
        if(button==0x1000) { Activate(g_Selected); return; }
        if(button==1 || button==0x10000000) g_Selected=(g_Selected+Entries-1)%Entries;
        if(button==2 || button==0x20000000) g_Selected=(g_Selected+1)%Entries;
        const unsigned page=g_Selected/Rows,pages=(Entries+Rows-1)/Rows;
        if(button==4 || button==0x800000) g_Selected=((page+pages-1)%pages)*Rows;
        if(button==8 || button==0x400000) g_Selected=((page+1)%pages)*Rows;
    }
};
HOOK_DEFINE_TRAMPOLINE(MenuKeyHook) {
    static void Callback(void* menu,int key) {
        if(!g_Open) { Orig(menu,key); return; }
        if(key==27) Close();
    }
};
HOOK_DEFINE_TRAMPOLINE(MenuRightHook) {
    static void Callback(void* menu,int x,int y,bool sound) {
        if(g_Open) { Close(); return; } Orig(menu,x,y,sound);
    }
};
HOOK_DEFINE_TRAMPOLINE(MenuScrollHook) {
    static void Callback(void* menu,int direction) {
        if(!g_Open) { Orig(menu,direction); return; }
        g_Selected=direction>0 ? (g_Selected+Entries-1)%Entries : (g_Selected+1)%Entries;
    }
};
HOOK_DEFINE_TRAMPOLINE(MenuCleanupHook) {
    static void Callback(void* menu) { Close(); Orig(menu); }
};
}
void InstallMenuHooks() {
    // Hook full GameMenu methods; inventory/ring trampolines remain single-owner.
    if(Static<std::uint32_t>(0x16EE2B0)!=0x6DB63BEFu ||
        Static<std::uint32_t>(0x16ED9A0)!=0xA9BC5FF8u ||
        Static<std::uint32_t>(0x16ED720)!=0xF81D0FF5u ||
        Static<std::uint32_t>(0x16EECA0)!=0xF81B0FF9u ||
        Static<std::uint32_t>(0x16EDE50)!=0xA9BE4FF4u ||
        Static<std::uint32_t>(0x16EDEA0)!=0xA9BE4FF4u ||
        Static<std::uint32_t>(0x16EEE90)!=0xA9BF7BFDu) {
        Logging.Log("[UIInfoSuite2] settings menu fingerprint mismatch"); return;
    }
    MenuDrawHook::InstallAtOffset(0x16EE2B0);
    MenuClickHook::InstallAtOffset(0x16ED9A0);
    MenuPadHook::InstallAtOffset(0x16ED720);
    MenuKeyHook::InstallAtOffset(0x16EECA0);
    MenuRightHook::InstallAtOffset(0x16EDE50);
    MenuScrollHook::InstallAtOffset(0x16EDEA0);
    MenuCleanupHook::InstallAtOffset(0x16EEE90);
}
}
