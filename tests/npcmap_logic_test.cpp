#include "uiinfo/npc_map_logic.hpp"
using namespace AutomateLite::UIInfo::NpcMapLogic;
// Regression: raw 300x180 map dimensions draw a 1200x720 map. East/south
// positions must not be discarded or scaled twice.
static_assert(OnMap(1100,650,300,180));
static_assert(OnMap(1199,719,300,180));
static_assert(!OnMap(1200,719,300,180));
static_assert(!OnMap(10,720,300,180));
static_assert(!OnMap(-1,1,300,180));
static_assert(!OnMap(1,1,0,180));
static_assert(!OnMap(1,1,8193,180));
static_assert(!OnMap(__builtin_nanf(""),1,300,180));
static_assert(!OnMap(__builtin_inff(),1,300,180));
static_assert(Near(0,0,32,0) && !Near(0,0,33,0));
static_assert(Near(100,100,100,100));
static_assert(ValidSource(180,490,14,18));
static_assert(!ValidSource(0,0,0,24));
static_assert(Clamp(1000,8,728)==728 && Clamp(-50,8,728)==8);
static_assert(TooltipRows(160)==1 && TooltipRows(720)==8);
