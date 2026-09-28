#include "uiinfo/shop.hpp"
#include "uiinfo/aot.hpp"
#include "uiinfo/settings.hpp"
#include "uiinfo/hud.hpp"
namespace AutomateLite::UIInfo {
namespace {
using namespace Aot;
TextBuffer<512> g_Harvest;
TextBuffer<128> g_Seed;
std::uint64_t g_Save{},g_Next{};
void Refresh(void* item) {
    if(!Is(item,0xDE2B6D8)) { g_Harvest={}; g_Next=0; return; }
    void* seed=Call<void*>(0x150EA40,item);
    const auto now=Milliseconds(),save=Static<std::uint64_t>(0xE27FA00);
    if(g_Save==save && Equals(seed,g_Seed.Data()) && now<g_Next) return;
    g_Save=save; g_Next=now+500; g_Seed={}; Append(g_Seed,seed); g_Harvest={};
    void* data{};
    if(!seed || Equals(seed,u"770")) return;
    void* harvestId{};
    const bool sapling=Call<bool>(0x19464F0,item);
    if(sapling) {
        if(!Call<bool>(0x1A5A9A0,seed,&data) || !data) return;
        void* fruit=Read<void*>(data,0x20);
        if(!fruit || Call<int>(0x4C58810,fruit)<1) return;
        void* drop=Call<void*>(0x4C58818,fruit,0);
        if(drop)harvestId=Read<void*>(drop,0x18);
    } else {
        if(!Call<bool>(0x11CA0D0,seed,&data) || !data) return;
        harvestId=Read<void*>(data,0x30);
    }
    if(!harvestId) return;
    // Read crop data directly: constructing a Crop would choose random mixed
    // seeds and can consume RNG. Only an unattached harvested Item is created.
    void* harvest=Call<void*>(0x1519AF0,harvestId,1,0,false);
    if(!harvest || !Is(harvest,0xDE2B6D8)) return;
    const int price=Virtual<int>(harvest,Offsets::SellToStorePriceSlot,std::int64_t{-1});
    if(price<=0) return;
    g_Harvest.Append(u"收成："); Append(g_Harvest,Virtual<void*>(harvest,Offsets::DisplayNameSlot));
    g_Harvest.Append(u"\n普通品质单个售价："); g_Harvest.AppendNumber(price); g_Harvest.Append(u" 金币");
    if(sapling) { g_Harvest.Append(u"\n果树成熟后在结果季每日产出"); return; }
    const int min=Read<int>(data,0x38),max=Read<int>(data,0x3C);
    if(min>1 || max>1) { g_Harvest.Append(u"\n基础产量："); g_Harvest.AppendNumber(min);
        g_Harvest.Append(u"–"); g_Harvest.AppendNumber(max); }
    const int regrow=Read<int>(data,0x20);
    if(regrow>0) { g_Harvest.Append(u"\n再次收获间隔："); g_Harvest.AppendNumber(regrow); g_Harvest.Append(u" 天"); }
}
HOOK_DEFINE_TRAMPOLINE(ShopDrawHook) {
    static void Callback(void* shop,void* batch) {
        Orig(shop,batch); ObserveMerchantShop(shop);
        if(!Enabled(Option::HarvestPrices)) return;
        Refresh(Read<void*>(shop,0xB8));
        if(!g_Harvest.Size()) return;
        auto* viewport=reinterpret_cast<void*>(Address(0xE27F4C8));
        const int height=Call<int>(0x1D8EF50,viewport);
        const int width=Call<int>(0x1D8EF70,viewport);
        int x=Read<int>(shop,0x28)+16,y=height-136;
        if(x+420>width-8)x=width-428;
        if(x<8)x=8;
        if(y<8)y=8;
        Box(batch,{x,y,420,120},0xEF201C18);
        Text(batch,g_Harvest,static_cast<float>(x+10),static_cast<float>(y+8),0xFFFFFFFF,0.7f);
    }
};
}
void InstallShopHook() {
    if(Static<std::uint32_t>(0x1782900)==0x6DB63BEFu) ShopDrawHook::InstallAtOffset(0x1782900);
    else Logging.Log("[UIInfoSuite2] shop draw fingerprint mismatch");
}
}
