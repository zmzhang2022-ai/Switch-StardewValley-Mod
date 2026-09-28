#pragma once
#include "uiinfo/text_buffer.hpp"
namespace AutomateLite::UIInfo {
void AppendItemInformation(void* item, TextBuffer<2048>& text);
void InstallItemHooks();
}
