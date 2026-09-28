#pragma once
#include <cstdint>
namespace AutomateLite::UIInfo::Logic {
inline constexpr int ExperienceThresholds[] = {0,100,380,770,1300,2150,3300,4800,6900,10000,15000};
constexpr int LevelFromExperience(int experience) {
    int level = 0;
    while (level < 10 && experience >= ExperienceThresholds[level + 1]) ++level;
    return level;
}
constexpr int CropDays(const int* phases, int count, int phase, int day, bool regrowing) {
    if (!phases || count < 1 || count > 32 || phase < 0 || phase >= count || day < 0) return -1;
    if (regrowing) return day;
    int result = -day;
    for (int i = phase; i < count-1; ++i) {
        if (phases[i] < 0 || phases[i] > 10000) return -1;
        result += phases[i];
    }
    return result > 0 ? result : 0;
}
constexpr bool IsBerrySeason(int season, int day) {
    return (season == 0 && day >= 15 && day <= 18) ||
        (season == 2 && day >= 8 && day <= 11);
}
constexpr bool ScarecrowCovers(int dx, int dy, int radius) {
    return radius > 0 && dx*dx + dy*dy < radius*radius;
}
constexpr int AgingDays(float remaining,float rate) {
    if(!(remaining>=0 && remaining<=100000 && rate>0 && rate<=1000)) return -1;
    const float days=remaining/rate;
    if(!(days<=100000)) return -1;
    const int whole=static_cast<int>(days);
    return whole+(days>whole ? 1 : 0);
}
}
