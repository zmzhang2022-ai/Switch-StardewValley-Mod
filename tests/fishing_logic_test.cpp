#include "fishing/logic.hpp"
using namespace AutomateLite::Fishing;
constexpr bool CheckChord() {
    Chord c;
    if(c.Update(true,false)) return false;
    if(!c.Update(true,true)) return false;
    for(int i=0;i<120;++i) if(c.Update(true,true)) return false;
    if(c.Update(false,true) || c.Update(true,true)) return false;
    c.Update(false,false);
    return c.Update(true,true);
}
static_assert(CheckChord());
static_assert(NeedsFood(9.99f) && !NeedsFood(10) && NeedsFood(-1));
static_assert(!TooLate(2450) && TooLate(2500) && TooLate(2600));
static_assert(!BetterFood(0,0,100));
static_assert(BetterFood(110,150,100) && !BetterFood(150,110,100));
static_assert(BetterFood(100,90,100) && !BetterFood(90,100,100));
static_assert(BetterFood(90,80,100) && !BetterFood(80,90,100));
