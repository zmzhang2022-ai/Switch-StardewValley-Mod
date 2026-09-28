#include "uiinfo/logic.hpp"
#include "uiinfo/settings_format.hpp"
using namespace AutomateLite::UIInfo;
constexpr bool ExperienceEdges() {
    for(int level=1;level<=10;++level) {
        const int threshold=Logic::ExperienceThresholds[level];
        if(Logic::LevelFromExperience(threshold-1)!=level-1 ||
           Logic::LevelFromExperience(threshold)!=level) return false;
    }
    return Logic::LevelFromExperience(-1)==0 && Logic::LevelFromExperience(2147483647)==10;
}
constexpr bool CropEdges() {
    constexpr int phases[]={1,2,2,99999};
    constexpr int bad[]={1,-1,99999};
    return Logic::CropDays(phases,4,0,0,false)==5 &&
        Logic::CropDays(phases,4,1,1,false)==3 &&
        Logic::CropDays(phases,4,3,0,false)==0 &&
        Logic::CropDays(phases,4,3,4,true)==4 &&
        Logic::CropDays(phases,4,3,0,true)==0 &&
        Logic::CropDays(phases,4,4,0,false)==-1 &&
        Logic::CropDays(nullptr,4,0,0,false)==-1 &&
        Logic::CropDays(bad,3,0,0,false)==-1;
}
constexpr bool ConfigFaults() {
    constexpr std::uint64_t save=0xFEDCBA9876543210ULL;
    auto valid=Config::Make(save,0xFFFFFFFFu,0x12345678);
    if(!Config::Valid(valid,save) || Config::Valid(valid,save+1)) return false;
    valid.words[6]=1; valid.hidden[0][0]=u'阿'; valid.hidden[0][1]=u'比';
    valid.words[7]=Config::Checksum(valid);
    if(!Config::Valid(valid,save)) return false;
    auto damaged=valid; damaged.hidden[0][0]^=1;
    if(Config::Valid(damaged,save)) return false;
    damaged=valid; damaged.words[3]^=1;
    if(Config::Valid(damaged,save)) return false;
    damaged=valid; damaged.words[6]=65; damaged.words[7]=Config::Checksum(damaged);
    if(Config::Valid(damaged,save)) return false;
    damaged=valid; for(auto& c:damaged.hidden[0]) c=u'x';
    damaged.words[7]=Config::Checksum(damaged);
    return !Config::Valid(damaged,save) && Config::Newer(0,0xFFFFFFFFu) &&
        !Config::Newer(0xFFFFFFFFu,0) && !Config::Newer(3,3) && !Config::Newer(0x80000000u,0);
}
static_assert(ExperienceEdges());
static_assert(CropEdges());
static_assert(Logic::AgingDays(55,2)==28 && Logic::AgingDays(56,2)==28);
static_assert(Logic::AgingDays(0,2)==0 && Logic::AgingDays(3,0)==-1);
static_assert(Logic::IsBerrySeason(0,15) && !Logic::IsBerrySeason(0,19));
static_assert(Logic::IsBerrySeason(2,11) && !Logic::IsBerrySeason(2,12));
static_assert(Logic::ScarecrowCovers(7,0,8) && !Logic::ScarecrowCovers(8,0,8));
static_assert(!Logic::ScarecrowCovers(7,7,8));
static_assert(sizeof(Config::Record)==5152);
static_assert(ConfigFaults());
