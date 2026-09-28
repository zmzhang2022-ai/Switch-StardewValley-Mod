#include "fastanimations/logic.hpp"
#include "uiinfo/settings_format.hpp"
using namespace AutomateLite::FastAnimations;
namespace Config=AutomateLite::UIInfo::Config;
constexpr unsigned oldMask=0x12345678u;
static_assert((MigrateMask(oldMask)&0x1FFFFFFFu)==oldMask);
static_assert((MigrateMask(oldMask)&EnabledBit)!=0);
static_assert((MigrateMask(oldMask)&TripleBit)==0);
static_assert(MigrateMask(ConfigMarker|oldMask)==(ConfigMarker|oldMask));
static_assert(MigrateMask(ConfigMarker|TripleBit)==(ConfigMarker|TripleBit));
constexpr auto off=Config::Make(42,1,ConfigMarker|oldMask);
static_assert(Config::Valid(off,42));
static_assert((MigrateMask(off.words[3])&EnabledBit)==0);
static_assert(!Config::Valid(off,43));
static_assert(ExtraUpdates(false,true)==0 && ExtraUpdates(true,false)==1 && ExtraUpdates(true,true)==2);
static_assert(ReducePause(8,16)==0 && ReducePause(0,16)==0 && ReducePause(-1,16)==-1);
static_assert(ReducePause(33,16)==17);
// Model original ordering: completion check, original animation step, mod step.
// Regress both callback thresholds, including values already beyond them.
static_assert(AdvanceFade(1.09f,true,16,1)>1.1f);
static_assert(AdvanceFade(-0.09f,false,16,1)<-0.1f);
static_assert(AdvanceFade(1.12f,true,16,1)>1.12f);
static_assert(AdvanceFade(-0.12f,false,16,1)<-0.12f);
constexpr bool FadeCompletes(bool fadeIn,int elapsed,unsigned extra) {
    float alpha=fadeIn ? 0.0f : 1.0f;
    for(int frame=0;frame<1000;++frame) {
        if(fadeIn ? alpha>1.1f : alpha<-0.1f) return true;
        alpha=AdvanceFade(alpha,fadeIn,elapsed,1); // vanilla update
        alpha=AdvanceFade(alpha,fadeIn,elapsed,extra);
    }
    return false;
}
constexpr bool AllFadeCases() {
    constexpr int intervals[]={1,8,16,17,33,50,100};
    for(int elapsed:intervals)
        for(unsigned extra=0;extra<=2;++extra)
            if(!FadeCompletes(true,elapsed,extra) || !FadeCompletes(false,elapsed,extra)) return false;
    return true;
}
static_assert(AllFadeCases());
