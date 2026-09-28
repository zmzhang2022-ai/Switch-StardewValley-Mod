#pragma once
#include <cstdint>
namespace AutomateLite::UIInfo {
enum class Option : unsigned {
    ExperienceBar, ExperienceFade, ExperienceGain, LevelUp,
    ItemInformation, CropInformation, EffectRanges, BombRange,
    Luck, ExactLuck, Weather, Birthday, HideFullBirthday,
    Merchant, HideVisitedMerchant, Recipes, ToolUpgrade, BuildingProgress,
    Berries, Hazelnuts, Animals, HideFullAnimals, Hearts, Gifts,
    NpcMap, HarvestPrices, Calendar, AllRanges, HoldRanges, FastAnimations, FastAnimationsTriple, Count
};
bool Enabled(Option option);
void Toggle(Option option);
void RefreshSettings();
bool SaveSettings(bool asDefaults = false);
const char16_t* OptionLabel(unsigned index);
const char16_t* SettingsStatus();
bool IsNpcTracked(void* internalName);
void ToggleNpcTracking(void* internalName);
}
