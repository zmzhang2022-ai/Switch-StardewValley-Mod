#pragma once
namespace AutomateLite::UIInfo::NpcMapLogic {
constexpr bool ValidBounds(int width,int height) { return width>0 && height>0 && width<=8192 && height<=8192; }
// MapPage dimensions are texture pixels, drawn at 4x. Position is ALREADY 4x.
constexpr bool OnMap(float x,float y,int width,int height) {
    return ValidBounds(width,height) && x>=0 && y>=0 && x<width*4 && y<height*4;
}
constexpr bool Near(float x,float y,float a,float b) { return (x-a)*(x-a)+(y-b)*(y-b)<=32*32; }
constexpr bool ValidSource(int x,int y,int w,int h) { return x>=0 && y>=0 && w>0 && h>0 && w<=128 && h<=128; }
constexpr int Clamp(int value,int low,int high) { return high<low ? low : value<low ? low : value>high ? high : value; }
constexpr unsigned TooltipRows(int height) { return static_cast<unsigned>(Clamp((height-72)/56,1,8)); }
}
