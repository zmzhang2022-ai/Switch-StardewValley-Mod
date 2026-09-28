#include "uiinfo/ranges.hpp"
#include "uiinfo/aot.hpp"
#include "uiinfo/settings.hpp"
#include "uiinfo/logic.hpp"
namespace AutomateLite::UIInfo {
namespace {
using namespace Aot;
enum class Shape { None,Scarecrow,Sprinkler,Bee,Square,Bomb };
struct Range { int x{},y{},radius{}; Shape shape{}; int hundredths{}; };
constexpr unsigned RangeCapacity=4096;
Range g_Others[RangeCapacity]; unsigned g_Count{};
int g_ViewX{},g_ViewY{};
std::uint64_t g_Next{},g_Save{}; TextBuffer<128> g_Location;
constexpr int Columns=128,Rows=80;
std::uint8_t g_Grid[Rows][Columns];
bool InternalName(void* object,const char16_t* name) { return Equals(Virtual<void*>(object,0x360),name); }
Range Describe(void* object,Game::TilePosition tile) {
    Range r{static_cast<int>(tile.x),static_cast<int>(tile.y)};
    if(!Is(object,0xDE2B6D8)) return r;
    if(Call<bool>(0x193BBB0,object)) { r.shape=Shape::Scarecrow; r.radius=Call<int>(0x193BC20,object); }
    else if(Call<bool>(0x1946D40,object)) { r.shape=Shape::Sprinkler; r.radius=Virtual<int>(object,0x9D0); }
    else if(InternalName(object,u"Bee House")) { r.shape=Shape::Bee; r.radius=5; }
    else if(InternalName(object,u"Mushroom Log")) { r.shape=Shape::Square; r.radius=7; }
    else if(InternalName(object,u"Mossy Seed")) { r.shape=Shape::Square; r.radius=5; }
    else if(Enabled(Option::BombRange)) {
        if(InternalName(object,u"Cherry Bomb")) r.hundredths=339;
        else if(InternalName(object,u"Bomb")) r.hundredths=552;
        else if(InternalName(object,u"Mega Bomb")) r.hundredths=745;
        if(r.hundredths) { r.shape=Shape::Bomb; r.radius=(r.hundredths+99)/100; }
    }
    if(r.radius<0 || r.radius>32) r.shape=Shape::None;
    return r;
}
bool Covers(const Range& r,int dx,int dy) {
    const int d=dx*dx+dy*dy;
    switch(r.shape) {
    case Shape::Scarecrow: return Logic::ScarecrowCovers(dx,dy,r.radius);
    case Shape::Sprinkler: return r.radius==0 ? d==1 : (dx||dy);
    // UIIS 2.3.7 GetDistanceArray: circle 4.19 plus cardinal extension to 5.
    case Shape::Bee: return d*10000<=419*419 || ((dx==0 || dy==0) && d<=25);
    case Shape::Square: return true;
    case Shape::Bomb: return d*10000<=r.hundredths*r.hundredths;
    default: return false;
    }
}
void Add(const Range& r,int originX,int originY,int width,int height,std::uint8_t value) {
    if(r.shape==Shape::None) return;
    const int radius=r.shape==Shape::Sprinkler && r.radius==0 ? 1 : r.radius;
    for(int dy=-radius;dy<=radius;++dy) for(int dx=-radius;dx<=radius;++dx) {
        const int x=r.x+dx-originX,y=r.y+dy-originY;
        if(x<0 || y<0 || x>=width || y>=height || !Covers(r,dx,dy)) continue;
        g_Grid[y][x]|=value;
    }
}
void RefreshOthers(void* location) {
    const auto save=Static<std::uint64_t>(0xE27FA00),now=Milliseconds();
    auto* viewport=reinterpret_cast<void*>(Address(0xE27F4B8));
    const int vx=Call<int>(0x1D8EF10,viewport)/64,vy=Call<int>(0x1D8EF30,viewport)/64;
    const int vw=Call<int>(0x1D8EF70,viewport)/64+2,vh=Call<int>(0x1D8EF50,viewport)/64+2;
    void* name=Call<void*>(0x1425CA0,location);
    if(save==g_Save && vx==g_ViewX && vy==g_ViewY && Equals(name,g_Location.Data()) && now<g_Next) return;
    g_ViewX=vx; g_ViewY=vy;
    g_Save=save; g_Next=now+250; g_Location={}; Append(g_Location,name); g_Count=0;
    const auto objects=Game::GameState::GetObjects(location);
    auto* dictionary=Call<void*>(0x1425D60,location);
    if(objects.readable && dictionary) for(unsigned i=0;i<objects.count && i<16384 && g_Count<RangeCapacity;++i) {
        if(!objects.IsActive(i)) continue;
        auto tile=objects.GetKey(i); void* object{};
        if(!(tile.x>=0 && tile.y>=0 && tile.x<32768 && tile.y<32768)) continue;
        if(tile.x<vx-32 || tile.y<vy-32 || tile.x>vx+vw+32 || tile.y>vy+vh+32) continue;
        // Backing values are network wrappers. Use the public TryGetValue;
        // do not pass the raw array value to Object methods.
        if(!Call<bool>(Offsets::TryGetObject,dictionary,tile,&object)) continue;
        const auto range=Describe(object,tile);
        if(range.shape!=Shape::None && range.x+range.radius>=vx-1 && range.y+range.radius>=vy-1 &&
            range.x-range.radius<=vx+vw && range.y-range.radius<=vy+vh) g_Others[g_Count++]=range;
    }
    auto buildings=Game::GameState::GetBuildings(location);
    if(buildings.readable) for(unsigned i=0;i<buildings.count && g_Count<RangeCapacity;++i) {
        void* building=buildings.Get(i); if(!building) continue;
        auto* field=Read<void*>(building,0x88);
        if(field && Equals(Read<void*>(field,0x58),u"Junimo Hut"))
            g_Others[g_Count++]={NetInt(building,0x40)+1,NetInt(building,0x48)+1,8,Shape::Square};
    }
}
}
void DrawEffectRanges(void* batch) {
    using namespace Aot;
    if(!Enabled(Option::EffectRanges)) return;
    const bool held=Call<bool>(0xD4220,reinterpret_cast<void*>(Address(0xE27F3A4)),0x40);
    if(Enabled(Option::HoldRanges) && !held) return;
    void* location=Game::GameState::GetCurrentLocation(); void* farmer=Game::GameState::GetPlayer();
    if(!location || !farmer) return;
    auto tile=Static<Game::TilePosition>(0xE27F958);
    if(!(tile.x>=0 && tile.y>=0 && tile.x<32768 && tile.y<32768)) return;
    void* dictionary=Call<void*>(0x1425D60,location); void* object{};
    if(dictionary) Call<bool>(Offsets::TryGetObject,dictionary,tile,&object);
    Range current=Describe(object,tile);
    if(current.shape==Shape::None) {
        void* item=Call<void*>(0x1318FD0,farmer);
        if(Is(item,0xDE2B6D8) && Virtual<bool>(item,0x190)) {
            current=Describe(item,tile);
            if(current.shape!=Shape::None) {
                tile=Call<Game::TilePosition>(0x13DA7A0);
                auto* checking=reinterpret_cast<std::uint8_t*>(Address(0xE27FA50));
                const auto previous=*checking;
                *checking=!Call<bool>(0x13DA6B0);
                auto pixel=Call<Game::TilePosition>(0x1AF70B0,farmer,location,item,
                    static_cast<int>(tile.x)*64,static_cast<int>(tile.y)*64);
                *checking=previous;
                current.x=static_cast<int>(pixel.x)/64; current.y=static_cast<int>(pixel.y)/64;
            }
        }
    }
    void* building=Call<void*>(0x1438D30,location,Static<Game::TilePosition>(0xE27F958));
    if(building) {
        auto* field=Read<void*>(building,0x88);
        if(field && Equals(Read<void*>(field,0x58),u"Junimo Hut"))
            current={NetInt(building,0x40)+1,NetInt(building,0x48)+1,8,Shape::Square};
    }
    if(current.shape==Shape::None && !Enabled(Option::AllRanges)) return;
    RefreshOthers(location);
    auto* viewport=reinterpret_cast<void*>(Address(0xE27F4B8));
    const int vx=Call<int>(0x1D8EF10,viewport),vy=Call<int>(0x1D8EF30,viewport);
    const int vw=Call<int>(0x1D8EF70,viewport),vh=Call<int>(0x1D8EF50,viewport);
    const int originX=vx/64-1,originY=vy/64-1;
    int width=vw/64+4,height=vh/64+4;
    if(width>Columns)width=Columns; if(height>Rows)height=Rows;
    if(width<=0 || height<=0) return;
    for(int y=0;y<height;++y) for(int x=0;x<width;++x) g_Grid[y][x]=0;
    for(unsigned i=0;i<g_Count;++i) {
        auto& r=g_Others[i];
        if(current.shape!=Shape::None && r.x==current.x && r.y==current.y) continue;
        if(Enabled(Option::AllRanges) || (current.shape==r.shape &&
            (current.shape==Shape::Sprinkler || current.shape==Shape::Scarecrow))) Add(r,originX,originY,width,height,2);
    }
    Add(current,originX,originY,width,height,1);
    const auto local=Call<Game::TilePosition>(0x13E6E80,Game::TilePosition{static_cast<float>(originX*64),static_cast<float>(originY*64)});
    const auto base=Call<Game::TilePosition>(0x1AFDBB0,local);
    const auto next=Call<Game::TilePosition>(0x1AFDBB0,Game::TilePosition{local.x+64,local.y+64});
    const float scale=next.x-base.x;
    if(!(scale>0 && scale<512)) return;
    for(int y=0;y<height;++y) for(int x=0;x<width;) {
        const auto value=g_Grid[y][x];
        if(!value) { ++x; continue; }
        int end=x+1; while(end<width && g_Grid[y][end]==value) ++end;
        const int left=static_cast<int>(base.x+x*scale),top=static_cast<int>(base.y+y*scale);
        const int right=static_cast<int>(base.x+end*scale),bottom=static_cast<int>(base.y+(y+1)*scale);
        // Premultiplied colors: selected green, existing blue, overlap amber.
        const std::uint32_t color=value==1 ? 0x50205020 : value==2 ? 0x50382010 : 0x50184050;
        Box(batch,{left,top,right-left,bottom-top},color); x=end;
    }
}
}
