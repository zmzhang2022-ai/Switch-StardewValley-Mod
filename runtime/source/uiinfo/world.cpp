#include "uiinfo/world.hpp"
#include "uiinfo/aot.hpp"
#include "uiinfo/logic.hpp"

namespace AutomateLite::UIInfo {
namespace {
using namespace Aot;
void CropInformation(void* dirt, TextBuffer<2048>& text) {
    void* crop = Call<void*>(0x1A672E0, dirt);
    if (crop && !NetBool(crop, 0x80)) {
        void* list = Read<void*>(crop, 0x18);
        if (!list) return;
        const int count = Call<int>(0x48097E0, list);
        if (count < 1 || count > 32) return;
        int phases[32]{};
        for (int i = 0; i < count; ++i) phases[i] = Call<int>(0x48096C8, list, i);
        const int days = Logic::CropDays(phases, count, NetInt(crop, 0x30),
            NetInt(crop, 0x40), NetBool(crop, 0x68));
        if (days < 0) return;
        void* harvest = Read<void*>(crop, 0x38);
        if (!harvest || !ItemName(text, Read<void*>(harvest, 0x58))) text.Append(u"作物");
        if (days == 0) text.Append(u"：可以收获");
        else { text.Append(u"：还需 "); text.AppendNumber(days); text.Append(u" 天"); }
        text.Append(NetInt(dirt, 0x38) == 1 ? u"\n已浇水" : u"\n未浇水");
    } else if (crop) text.Append(u"枯萎的作物");
    void* fertilizer = Read<void*>(dirt, 0x40);
    void* id = fertilizer ? Read<void*>(fertilizer, 0x58) : nullptr;
    if (id && !Equals(id, u"") && !Equals(id, u"0")) {
        if (text.Size()) text.Append(u"\n");
        text.Append(u"肥料："); ItemName(text, id);
    }
}
void TreeInformation(void* tree, TextBuffer<2048>& text) {
    void* idField = Read<void*>(tree, 0x50);
    void* id = idField ? Read<void*>(idField, 0x58) : nullptr;
    const char16_t* name = u"树木";
    if (Equals(id,u"1")) name = u"橡树";
    else if (Equals(id,u"2")) name = u"枫树";
    else if (Equals(id,u"3")) name = u"松树";
    else if (Equals(id,u"6") || Equals(id,u"9")) name = u"棕榈树";
    else if (Equals(id,u"7")) name = u"蘑菇树";
    else if (Equals(id,u"8")) name = u"桃花心木树";
    else if (Equals(id,u"10")) name = u"绿雨树（蓬松型）";
    else if (Equals(id,u"11")) name = u"绿雨树（阔叶型）";
    else if (Equals(id,u"12")) name = u"绿雨树（蕨叶型）";
    else if (Equals(id,u"13")) name = u"神秘树";
    text.Append(name);
    if (NetBool(tree, 0x68)) { text.Append(u"（树桩）"); return; }
    const int stage = NetInt(tree, 0x48);
    if (stage < 5) {
        text.Append(u"\n生长阶段："); text.AppendNumber(stage); text.Append(u" / 5");
        if (NetBool(tree,0x98)) text.Append(u"\n已施树肥");
    }
}
void FruitTreeInformation(void* tree, TextBuffer<2048>& text) {
    Append(text, Call<void*>(0x1A5AA70, tree));
    const int days = NetInt(tree,0x58);
    if (NetBool(tree,0x88)) { text.Append(u"（树桩）"); return; }
    if (days > 0) {
        text.Append(u"\n距离成熟："); text.AppendNumber(days); text.Append(u" 天");
    } else {
        text.Append(u"\n已成熟");
        const int lightning = NetInt(tree,0x70);
        if (lightning > 0) {
            text.Append(u"；雷击恢复还需 "); text.AppendNumber(lightning); text.Append(u" 天");
        }
        if(!Call<bool>(0x1A5B690,tree)) text.Append(u"\n当前地点不在结果季");
    }
    void* data=Call<void*>(0x1A58690,tree);
    void* drops=data ? Read<void*>(data,0x20) : nullptr;
    const int dropCount=drops ? Call<int>(0x4C58810,drops) : 0;
    if(dropCount>0 && dropCount<=32) {
        for(int i=0;i<dropCount && i<4;++i) {
            void* drop=Call<void*>(0x4C58818,drops,i);
            if(!drop) continue;
            TextBuffer<128> name;
            if(ItemName(name,Read<void*>(drop,0x18))) {
                text.Append(u"\n果实："); text.Append(name.Data());
                const float chance=Read<float>(drop,0xA0);
                if(chance>=0 && chance<1) { text.Append(u"（"); text.AppendNumber(static_cast<int>(chance*100)); text.Append(u"%）"); }
            }
        }
    }
    void* fruit=Read<void*>(tree,0x68);
    const int count=fruit ? Call<int>(0x3E940F4,fruit) : 0;
    if(count>0 && count<=64) { text.Append(u"\n当前可采摘："); text.AppendNumber(count); text.Append(u" 个"); }
}
void TeaInformation(void* bush, TextBuffer<2048>& text) {
    if (NetInt(bush,0x40) != 3) return;
    text.Append(u"茶树");
    const int age = Call<int>(0x1A4EC10,bush);
    if (age < 20) {
        text.Append(u"\n距离成熟："); text.AppendNumber(20-age); text.Append(u" 天");
    } else if (NetInt(bush,0x50) == 1) text.Append(u"\n茶叶可以采摘");
    else {
        const int day = Static<int>(0xE27F7A8), season = Static<int>(0xE27F720);
        if (season == 3) text.Append(u"\n冬季休眠");
        else if (day < 22) {
            text.Append(u"\n距离采摘季："); text.AppendNumber(22-day); text.Append(u" 天");
        } else text.Append(u"\n采摘季；今日暂无茶叶");
    }
}
void BuildingInformation(void* location,Game::TilePosition tile,TextBuffer<2048>& text) {
    void* building=Call<void*>(0x1438D30,location,tile);
    if(!building) return;
    void* data=Call<void*>(0x1154230,building);
    void* conversions=data ? Read<void*>(data,0x180) : nullptr;
    const int count=conversions ? Call<int>(0x43866D0,conversions) : 0;
    // Match PC SourceChest/DestinationChest selection, not all decorative
    // building chests. Callback-local strings are never retained across frames.
    void* seen[32]{}; unsigned seenCount=0,shown=0;
    if(count>=0 && count<=128) for(int i=0;i<count;++i) {
        void* conversion=Call<void*>(0x43866D8,conversions,i);
        if(!conversion) continue;
        for(unsigned kind=0;kind<2;++kind) {
            void* name=Read<void*>(conversion,kind ? 0x30 : 0x28);
            if(!name) continue;
            void* chest=Call<void*>(0x1154C00,building,name);
            if(!chest) continue;
            bool duplicate=false; for(unsigned j=0;j<seenCount;++j) if(seen[j]==chest)duplicate=true;
            if(duplicate || seenCount>=32) continue;
            seen[seenCount++]=chest;
            const auto items=Game::ChestActions::GetItems(chest);
            if(!items.readable) continue;
            for(unsigned j=0;j<items.count;++j) {
                void* item=items.Get(j); if(!item)continue;
                if(shown++>=8) continue;
                if(text.Size())text.Append(u"\n");
                text.Append(kind ? u"产出：" : u"待加工：");
                Append(text,Virtual<void*>(item,Offsets::DisplayNameSlot));
                text.Append(u" × "); text.AppendNumber(Virtual<int>(item,Offsets::StackSlot));
            }
        }
    }
    if(shown>8)text.Append(u"\n更多物品请打开建筑库存查看");
    const Game::FishPondView pond(building);
    if(pond.IsFishPond()) if(void* item=pond.ReadOutput()) {
        if(text.Size())text.Append(u"\n");
        text.Append(u"鱼塘产出："); Append(text,Virtual<void*>(item,Offsets::DisplayNameSlot));
        text.Append(u" × "); text.AppendNumber(Virtual<int>(item,Offsets::StackSlot));
    }
}
}

void AppendWorldHover(TextBuffer<2048>& text) {
    using namespace Aot;
    auto* location = Game::GameState::GetCurrentLocation();
    auto tile = Static<Game::TilePosition>(0xE27F958);
    if (!location || !(tile.x >= 0 && tile.y >= 0 && tile.x < 32768 && tile.y < 32768)) return;
    void* feature = Game::GameState::GetTerrainFeatureAt(location, tile);
    if (!feature) {
        auto* objects = Call<void*>(0x1425D60,location);
        void* object{};
        if (objects && Call<bool>(Offsets::TryGetObject, objects, tile, &object) &&
            Is(object,0xDE36B40)) {
            // IndoorPot.draw consumers and concrete NetRef getters are checked
            // before use; these references belong to this callback only.
            auto* dirtRef = Read<void*>(object,0x218);
            auto* bushRef = Read<void*>(object,0x220);
            feature = bushRef ? Read<void*>(bushRef,0x58) : nullptr;
            if (!feature && dirtRef) feature = Read<void*>(dirtRef,0x58);
        }
    }
    if (!feature) { BuildingInformation(location,tile,text); return; }
    if (Is(feature,0xDE4FAC0)) CropInformation(feature,text);
    else if (Is(feature,0xDE50908)) TreeInformation(feature,text);
    else if (Is(feature,0xDE4F3A0)) {
        FruitTreeInformation(feature,text);
        // Static bool(Vector2 HFA, GameLocation*), confirmed by 7552574.
        if(NetInt(feature,0x58)>0 && Call<bool>(0x1A5A660,tile,location)) text.Append(u"\n周围有障碍，无法生长");
    }
    else if (Is(feature,0xDE4EC10)) TeaInformation(feature,text);
}
}
