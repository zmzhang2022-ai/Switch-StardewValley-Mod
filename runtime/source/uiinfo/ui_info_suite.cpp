/* Copyright (C) 2026 zmzhang2022-ai | GPL-2.0-only */
#include "uiinfo/ui_info_suite.hpp"
#include "uiinfo/offsets.hpp"
#include "uiinfo/text_buffer.hpp"
#include "uiinfo/aot.hpp"
#include "uiinfo/world.hpp"
#include "uiinfo/hud.hpp"
#include "uiinfo/items.hpp"
#include "uiinfo/animals.hpp"
#include "uiinfo/menu.hpp"
#include "uiinfo/settings.hpp"
#include "uiinfo/social.hpp"
#include "uiinfo/npc_map.hpp"
#include "uiinfo/shop.hpp"
#include "uiinfo/ranges.hpp"
#include "uiinfo/logic.hpp"
#include "game/offsets.hpp"
#include "game/runtime.hpp"
#include "lib.hpp"
#include "fishing/fishing.hpp"

namespace AutomateLite::UIInfo {
namespace {
namespace GameOffsets = StardewValley::Offsets;
bool g_Enabled{};
bool g_LoggedHud{};


std::uintptr_t Target(std::uintptr_t offset) {
    return exl::util::modules::GetTargetOffset(offset);
}
template <typename T>
T* Field(void* object, std::uintptr_t offset) {
    return reinterpret_cast<T*>(reinterpret_cast<std::uintptr_t>(object) + offset);
}
template <typename T>
T* Storage(std::uintptr_t slot) {
    return *reinterpret_cast<T**>(Target(slot));
}
void* Root(std::uintptr_t slot) {
    auto** storage = Storage<void*>(slot);
    return storage == nullptr ? nullptr : *storage;
}
template <typename Result, typename... Args>
Result Virtual(void* object, std::uintptr_t slot, Args... args) {
    auto* type = *Field<void*>(object, 0);
    auto* table = *Field<void*>(type, 8);
    auto function = *Field<std::uintptr_t>(table, slot);
    auto adjustment = *Field<std::uint8_t>(table, slot + 8);
    return reinterpret_cast<Result (*)(void*, Args...)>(function)(
        Field<void>(object, adjustment), args...);
}

template <std::size_t N>
bool AppendManaged(TextBuffer<N>& buffer, void* text) {
    if (text == nullptr) return true;
    auto length = *Field<std::int32_t>(text, Offsets::StringLength);
    return length >= 0 && buffer.Append(
        Field<char16_t>(text, Offsets::StringCharacters), static_cast<std::size_t>(length));
}
template <std::size_t N>
void* ToManaged(const TextBuffer<N>& text) {
    auto* result = reinterpret_cast<void* (*)(std::int32_t)>(
        Target(Offsets::AllocateString))(static_cast<std::int32_t>(text.Size()));
    if (result == nullptr) return nullptr;
    auto* characters = Field<char16_t>(result, Offsets::StringCharacters);
    for (std::size_t i = 0; i <= text.Size(); ++i) characters[i] = text.Data()[i];
    return result;
}

bool CanDrawWorld(void* game) {
    if (!g_Enabled || game == nullptr ||
        *Field<std::uint8_t>(game, Offsets::TakingMapScreenshot)) return false;
    if (Root(Offsets::HudSuppressionRoot) != nullptr) return false;
    if (reinterpret_cast<std::uint8_t (*)()>(Target(Offsets::GetGameMode))() != 3 ||
        reinterpret_cast<void* (*)()>(Target(Offsets::GetActiveMenu))() != nullptr)
        return false;
    const auto* hud = Storage<std::uint8_t>(Offsets::DisplayHudStorage);
    const auto* event = Storage<std::uint8_t>(Offsets::EventUpStorage);
    const auto* freeze = Storage<std::uint8_t>(Offsets::FreezeControlsStorage);
    const auto* viewportFreeze = Storage<std::uint8_t>(Offsets::ViewportFreezeStorage);
    const auto* hold = Storage<std::int32_t>(Offsets::ViewportHoldStorage);
    return hud && *hud && event && !*event && freeze && !*freeze &&
        viewportFreeze && !*viewportFreeze && hold && *hold <= 0;
}

void DrawWorldInformation(void* game) {
    RefreshSettings();
    if (!CanDrawWorld(game)) { ClearAnimalMarkers(); return; }
    void* batch = Root(Offsets::SpriteBatchRoot);
    if (!batch) return;
    DrawDailyHud(batch);
    Fishing::Draw(batch);
    DrawAnimalMarkers(batch);
    if (!Enabled(Option::CropInformation)) return;
    TextBuffer<2048> text;
    auto* tile = Storage<Game::TilePosition>(Offsets::CursorTileStorage);
    auto* location = Game::GameState::GetCurrentLocation();
    if (!tile || !location || !(tile->x >= 0 && tile->y >= 0 && tile->x < 32768 && tile->y < 32768)) return;
    void* objects = Aot::Call<void*>(GameOffsets::GameLocationGetObjects,location);
    void* object{};
    if (objects && Aot::Call<bool>(Offsets::TryGetObject,objects,*tile,&object) && object) {
        const auto state = Game::ObjectView(object).ReadOutputState();
        if (state.heldObject && (state.readyForHarvest ||
            (state.hasMinutesUntilReady && state.minutesUntilReady > 0))) {
            AppendManaged(text,Virtual<void*>(object,Offsets::DisplayNameSlot));
            text.Append(u"\n");
            AppendManaged(text,Virtual<void*>(state.heldObject,Offsets::DisplayNameSlot));
            if (state.readyForHarvest) text.Append(u"\n已完成，可以收取");
            else if(Aot::Is(object,0xDE2F368)) {
                // Cask.DayUpdate subtracts agingRate from daysToMature once per
                // day. minutesUntilReady is a placeholder (999999) in this type.
                void* daysField=Aot::Read<void*>(object,0x220);
                void* rateField=Aot::Read<void*>(object,0x218);
                const int days=daysField && rateField ? Logic::AgingDays(
                    Aot::Read<float>(daysField,0x58),Aot::Read<float>(rateField,0x58)) : -1;
                if(days>=0) { text.Append(u"\n距离铱星熟成："); text.AppendNumber(days); text.Append(u" 天"); }
            } else {
                text.Append(u"\n剩余："); text.AppendNumber(state.minutesUntilReady / 60);
                text.Append(u" 小时 "); text.AppendNumber(state.minutesUntilReady % 60);
                text.Append(u" 分钟（游戏时间）");
            }
        }
    }
    TextBuffer<2048> terrain;
    AppendWorldHover(terrain);
    if (terrain.Size()) {
        if (text.Size()) text.Append(u"\n");
        text.Append(terrain.Data(),terrain.Size());
    }
    if (!text.Size()) return;
    // Both mouse getters explicitly use UI scale; rectangle dimensions come
    // from the same uiViewport. Never mix the world viewport with UI pixels.
    const int mouseX=Aot::Call<int>(0x13C3920,true), mouseY=Aot::Call<int>(0x13C3A40,true);
    void* viewport=reinterpret_cast<void*>(Target(0xE27F4C8));
    const int width=Aot::Call<int>(0x1D8EF70,viewport), height=Aot::Call<int>(0x1D8EF50,viewport);
    int lines=1;
    for (std::size_t i=0;i<text.Size();++i) if(text.Data()[i]==u'\n') ++lines;
    const int panelWidth=520, panelHeight=lines*26+20;
    int x=mouseX+24, y=mouseY+24;
    if(x+panelWidth>width) x=width-panelWidth-8;
    if(y+panelHeight>height) y=height-panelHeight-8;
    if(x<8) x=8;
    if(y<8) y=8;
    Aot::Box(batch,{x,y,panelWidth,panelHeight},0xE6202018);
    Aot::Text(batch,text,static_cast<float>(x+10),static_cast<float>(y+8),0xFFFFFFFF,0.8f);
    if (!g_LoggedHud) {
        g_LoggedHud=true;
        Logging.Log("[UIInfoSuite2] world information draw reached");
    }
}

HOOK_DEFINE_TRAMPOLINE(UiInfoDrawHudHook) {
    static void Callback(void* game) {
        if(CanDrawWorld(game)) DrawEffectRanges(Root(Offsets::SpriteBatchRoot));
        Orig(game);
        DrawWorldInformation(game);
    }
};
}

bool CanDrawOverlay() { return CanDrawWorld(Aot::Static<void*>(0xE27FB10)); }

void Install() {
    // Fail closed for the added UI when its hook/string-allocation signatures
    // differ. This is a local fingerprint, not a full Build ID gate for old mods.
    if (*reinterpret_cast<const std::uint32_t*>(Target(Offsets::DrawHud)) != 0x6DB63BEFu ||
        *reinterpret_cast<const std::uint32_t*>(Target(Offsets::AllocateString)) != 0xF81E0FF3u) {
        Logging.Log("[UIInfoSuite2] ERROR: AOT signature mismatch; UI disabled");
        return;
    }
    UiInfoDrawHudHook::InstallAtOffset(Offsets::DrawHud);
    InstallExperienceHook();
    InstallItemHooks();
    InstallAnimalHooks();
    InstallMenuHooks();
    InstallSocialHooks();
    InstallMapHook();
    InstallShopHook();
    g_Enabled = true;
    Logging.Log("[UIInfoSuite2] singleplayer candidate enabled: HUD, world, items, animals, settings");
}
}
