#include "common.hpp"

#include "program/setting.hpp"
#include <string>

constexpr const int ModuleNameLength = std::char_traits<char>::length(EXL_MODULE_NAME);

struct ModuleName {
    int unknown;
    int name_length;
    char name[ModuleNameLength + 1];
};

// Clang may discard this internal-linkage const object before the linker sees
// the KEEP rule. The NSO loader/exlaunch expects this header at the beginning
// of rodata, so force it to be emitted for both GCC and Clang builds.
__attribute__((used, section(".nx-module-name")))
const ModuleName s_ModuleName = {.unknown = 0, .name_length = ModuleNameLength, .name = EXL_MODULE_NAME};
