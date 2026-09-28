#include "uiinfo/settings.hpp"
#include "uiinfo/settings_format.hpp"
#include "fastanimations/logic.hpp"
#include "uiinfo/aot.hpp"
#include "nn/fs/fs_files.hpp"
namespace nn::fs {
Result MountSdCard(const char* name);
Result CreateDirectory(const char* path);
Result SetFileSize(FileHandle file,long size);
}
namespace AutomateLite::UIInfo {
namespace {
constexpr unsigned Count=static_cast<unsigned>(Option::Count);
static_assert(Count<32);
constexpr std::uint32_t Bit(Option o) { return 1u<<static_cast<unsigned>(o); }
constexpr std::uint32_t Defaults=((1u<<Count)-1)&~(Bit(Option::ExactLuck)|
    Bit(Option::HideFullBirthday)|Bit(Option::HideVisitedMerchant)|Bit(Option::HideFullAnimals)|
    Bit(Option::AllRanges)|Bit(Option::HoldRanges)|Bit(Option::FastAnimationsTriple));
std::uint32_t g_Mask=Defaults;
std::uint64_t g_Save{};
bool g_Loaded{},g_MountAttempted{},g_Mounted{};
bool g_Dirty{};
char16_t g_Hidden[Config::HiddenCapacity][Config::NameCapacity]{};
unsigned g_HiddenCount{};
const char16_t* g_Status=u"设置尚未加载";
bool Mount() {
    if (!g_MountAttempted) {
        g_MountAttempted=true;
        g_Mounted=nn::fs::MountSdCard("uiinfo")==0;
        if(g_Mounted) {
            nn::fs::CreateDirectory("uiinfo:/config");
            nn::fs::CreateDirectory("uiinfo:/config/uiinfosuite2");
        }
    }
    return g_Mounted;
}
void Path(char (&path)[80],std::uint64_t save,unsigned slot) {
    constexpr char prefix[]="uiinfo:/config/uiinfosuite2/";
    unsigned n=0; for(char c:prefix) { if(!c) break; path[n++]=c; }
    for(int shift=60;shift>=0;shift-=4) path[n++]="0123456789abcdef"[(save>>shift)&15];
    path[n++]='-'; path[n++]=static_cast<char>('a'+slot);
    for(char c: ".cfg") { path[n++]=c; }
}
bool Read(std::uint64_t save,unsigned slot,Config::Record& r) {
    char path[80]{}; Path(path,save,slot);
    nn::fs::FileHandle file{};
    if(nn::fs::OpenFile(&file,path,nn::fs::OpenMode_Read)!=0) return false;
    long size{};
    const bool ok=nn::fs::GetFileSize(&size,file)==0 && size==sizeof(r) &&
        nn::fs::ReadFile(file,0,&r,sizeof(r))==0;
    nn::fs::CloseFile(file);
    return ok && Config::Valid(r,save);
}
int Latest(std::uint64_t save,Config::Record& chosen) {
    Config::Record a{},b{};
    const bool av=Read(save,0,a),bv=Read(save,1,b);
    if(!av && !bv) return -1;
    const int slot=bv && (!av || Config::Newer(b.words[2],a.words[2])) ? 1 : 0;
    chosen=slot ? b : a; return slot;
}
void Apply(const Config::Record& record) {
    g_Mask=FastAnimations::MigrateMask(record.words[3])&((1u<<Count)-1); g_HiddenCount=record.words[6];
    for(unsigned i=0;i<Config::HiddenCapacity;++i) for(unsigned j=0;j<Config::NameCapacity;++j)
        g_Hidden[i][j]=record.hidden[i][j];
}
}
bool Enabled(Option option) { return (g_Mask&Bit(option))!=0; }
void Toggle(Option option) {
    if(static_cast<unsigned>(option)>=Count) return;
    g_Mask^=Bit(option); g_Dirty=true; g_Status=u"已修改，关闭面板时保存";
}
void RefreshSettings() {
    if(Aot::Call<std::uint8_t>(0x139CBD0)!=3 || !Game::GameState::GetPlayer()) { g_Loaded=false; return; }
    const auto save=Aot::Static<std::uint64_t>(0xE27FA00);
    if(g_Loaded && save==g_Save) return;
    g_Save=save; g_Loaded=true; g_Mask=Defaults; g_HiddenCount=0; g_Dirty=true;
    for(auto& name:g_Hidden) for(auto& c:name)c=0;
    if(!Mount()) { g_Status=u"SD 配置不可用；本次设置仅保留在内存"; return; }
    Config::Record r{};
    if(Latest(0,r)>=0) Apply(r);
    if(save && Latest(save,r)>=0) { Apply(r); g_Dirty=false; g_Status=u"已加载此存档设置"; }
    else g_Status=u"使用默认设置；保存后独立用于此存档";
}
bool SaveSettings(bool asDefaults) {
    if(!asDefaults && !g_Dirty) return true;
    if(!g_Loaded || (!asDefaults && !g_Save) || !Mount()) {
        g_Status=u"无法保存设置；本次修改仍在内存中"; return false;
    }
    const auto save=asDefaults ? 0 : g_Save;
    Config::Record previous{};
    const int latest=Latest(save,previous);
    const unsigned slot=latest==0 ? 1 : 0;
    auto record=Config::Make(save,latest<0 ? 1 : previous.words[2]+1,g_Mask|FastAnimations::ConfigMarker);
    record.words[6]=g_HiddenCount;
    for(unsigned i=0;i<Config::HiddenCapacity;++i) for(unsigned j=0;j<Config::NameCapacity;++j)
        record.hidden[i][j]=g_Hidden[i][j];
    record.words[7]=Config::Checksum(record);
    char path[80]{}; Path(path,save,slot);
    nn::fs::CreateFile(path,sizeof(record));
    nn::fs::FileHandle file{};
    bool ok=nn::fs::OpenFile(&file,path,nn::fs::OpenMode_Write)==0;
    if(ok) {
        const nn::fs::WriteOption flush{nn::fs::WriteOptionFlag_Flush};
        ok=nn::fs::SetFileSize(file,sizeof(record))==0 && nn::fs::WriteFile(file,0,&record,sizeof(record),flush)==0;
        nn::fs::CloseFile(file);
    }
    Config::Record check{};
    ok=ok && Read(save,slot,check) && check.words[2]==record.words[2] && check.words[7]==record.words[7];
    g_Status=ok ? (asDefaults ? u"已保存为新存档默认设置" : u"已保存此存档设置") :
        u"保存失败；上一次有效设置仍可恢复";
    if(ok && !asDefaults) g_Dirty=false;
    return ok;
}
const char16_t* SettingsStatus() { return g_Status; }
bool IsNpcTracked(void* name) {
    for(unsigned i=0;i<g_HiddenCount;++i) if(Aot::Equals(name,g_Hidden[i])) return false;
    return true;
}
void ToggleNpcTracking(void* name) {
    if(!name) return;
    for(unsigned i=0;i<g_HiddenCount;++i) if(Aot::Equals(name,g_Hidden[i])) {
        --g_HiddenCount;
        for(unsigned j=0;j<Config::NameCapacity;++j) {
            g_Hidden[i][j]=g_Hidden[g_HiddenCount][j]; g_Hidden[g_HiddenCount][j]=0;
        }
        g_Dirty=true; SaveSettings(); return;
    }
    TextBuffer<Config::NameCapacity> copy;
    if(g_HiddenCount>=Config::HiddenCapacity || !Aot::Append(copy,name) || !copy.Size()) {
        g_Status=u"无法添加更多隐藏村民"; return;
    }
    for(unsigned j=0;j<=copy.Size();++j) g_Hidden[g_HiddenCount][j]=copy.Data()[j];
    ++g_HiddenCount; g_Dirty=true; SaveSettings();
}
const char16_t* OptionLabel(unsigned index) {
    constexpr const char16_t* names[]={u"经验进度条",u"经验条自动淡出",u"经验获得提示",u"升级提示",
        u"物品售价与收集信息",u"作物与机器悬停",u"物品作用范围",u"炸弹范围",
        u"每日运势",u"精确运势数值",u"明日天气",u"生日提醒",u"隐藏满好感生日",
        u"旅行货车",u"隐藏已访问货车",u"电视新菜谱",u"工具升级",u"建筑进度",
        u"浆果季",u"榛子季",u"动物抚摸与产物",u"隐藏满好感抚摸",u"社交好感进度",u"今日送礼",
        u"地图村民位置",u"商店种子收成价格",u"便携日历与委托",u"显示附近全部范围",u"按住左摇杆才显示范围",
        u"动画加速与免吃喝确认",u"动画速度 3 倍（关闭为 2 倍）"};
    static_assert(sizeof(names)/sizeof(names[0])==Count);
    return index<Count ? names[index] : u"";
}
}
