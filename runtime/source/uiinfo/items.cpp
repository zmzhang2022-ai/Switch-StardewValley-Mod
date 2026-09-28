#include "uiinfo/items.hpp"
#include "uiinfo/aot.hpp"
#include "uiinfo/settings.hpp"
namespace AutomateLite::UIInfo {
using namespace Aot;
void AppendItemInformation(void* item, TextBuffer<2048>& text) {
    if (!item || !Enabled(Option::ItemInformation) || !Game::GameState::GetPlayer()) return;
    const bool object=Is(item,0xDE2B6D8);
    const int unit=object ? Virtual<int>(item,Offsets::SellToStorePriceSlot,std::int64_t{-1}) :
        Virtual<int>(item,Offsets::SalePriceSlot,false)/2;
    const int stack=Virtual<int>(item,Offsets::StackSlot);
    if (unit >= 0 && stack > 0) {
        text.Append(u"\n出售单价："); text.AppendNumber(unit); text.Append(u" 金币");
        if (stack>1) { text.Append(u"\n整组售价："); text.AppendNumber(static_cast<std::int64_t>(unit)*stack); text.Append(u" 金币"); }
    }
    if (!object) return;
    if (Call<bool>(0x194DEC0,item)) text.Append(u"\n博物馆尚未捐赠");
    if (Call<bool>(0x193D940,item)) {
        void* farmer=Game::GameState::GetPlayer();
        void* shipped=Read<void*>(farmer,0x698);
        void* dictionary=shipped ? Read<void*>(shipped,0x48) : nullptr;
        void* id=Call<void*>(0x150EA40,item);
        void* field{};
        // Same TryGetValue and owning dictionary unwrap as Farmer.shippedBasic.
        if (dictionary && id && (!Call<bool>(0xDE36C0,dictionary,id,&field) ||
            (field && Virtual<int>(shipped,0x210,field)<=0))) text.Append(u"\n出货收集：尚未出货");
    }
    const auto locations=Game::GameState::GetLoadedLocations();
    if (!locations.readable) return;
    for (std::uint32_t i=0;i<locations.count;++i) {
        void* center=locations.Get(i);
        if (Is(center,0xDDD2CA0)) {
            // Both real CommunityCenter constructors call refreshBundlesIngredientsInfo
            // (1543480 / 1543614). A null cache means initialization is incomplete;
            // do not create or mutate a location merely to draw a tooltip.
            if (Read<void*>(center,0x368) && Call<bool>(0x1549000,center,item)) text.Append(u"\n社区中心献祭包需要此物品");
            break;
        }
    }
}
namespace {
HOOK_DEFINE_TRAMPOLINE(ItemTooltipHook) {
    static void Callback(void* batch,void* hoverText,void* title,void* item,
        bool held,int heal,int currency,void* extraIndex,int extraAmount,
        void* ingredients,int money,void* additional) {
        TextBuffer<2048> text;
        if (Enabled(Option::ItemInformation) && item && Append(text,hoverText)) {
            AppendItemInformation(item,text);
            if (void* result=Managed(text)) hoverText=result;
        }
        Orig(batch,hoverText,title,item,held,heal,currency,extraIndex,extraAmount,ingredients,money,additional);
    }
};
}
void InstallItemHooks() {
    // Static drawToolTip(string,string,Item,...) confirmed by invoker
    // 0x73C93B4 and body 0x16F91B0. No nullable value parameters in this overload.
    if (Static<std::uint32_t>(0x16F91B0)==0xD10543FFu)
        ItemTooltipHook::InstallAtOffset(0x16F91B0);
    else Logging.Log("[UIInfoSuite2] item tooltip fingerprint mismatch");
}
}
