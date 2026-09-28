#pragma once
#include <cstdint>
namespace AutomateLite::UIInfo::Config {
// Fixed little-endian words, with no compiler-dependent padding on disk.
inline constexpr unsigned HiddenCapacity=64,NameCapacity=40;
struct Record { std::uint32_t words[8]{}; char16_t hidden[HiddenCapacity][NameCapacity]{}; };
constexpr std::uint32_t Checksum(const Record& record) {
    std::uint32_t hash=2166136261u;
    for(unsigned i=0;i<7;++i) for(unsigned j=0;j<4;++j) {
        hash^=(record.words[i]>>(j*8))&255u; hash*=16777619u;
    }
    for(auto& name:record.hidden) for(char16_t c:name) {
        hash^=c&255; hash*=16777619u; hash^=c>>8; hash*=16777619u;
    }
    return hash;
}
constexpr Record Make(std::uint64_t save,std::uint32_t sequence,std::uint32_t mask) {
    Record r{{0x32495553u,2,sequence,mask,static_cast<std::uint32_t>(save),
        static_cast<std::uint32_t>(save>>32),0,0}};
    r.words[7]=Checksum(r); return r;
}
constexpr bool Valid(const Record& r,std::uint64_t save) {
    if(r.words[6]>HiddenCapacity) return false;
    for(unsigned i=0;i<r.words[6];++i) {
        if(!r.hidden[i][0]) return false;
        bool terminated=false;
        for(char16_t c:r.hidden[i]) if(!c) terminated=true;
        if(!terminated) return false;
    }
    return r.words[0]==0x32495553u && r.words[1]==2 &&
        r.words[4]==static_cast<std::uint32_t>(save) && r.words[5]==save>>32 &&
        r.words[7]==Checksum(r);
}
constexpr bool Newer(std::uint32_t a,std::uint32_t b) {
    return a!=b && static_cast<std::uint32_t>(a-b)<0x80000000u;
}
}
