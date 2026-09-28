#pragma once

namespace AutomateLite::UIInfo {
// Owns only dedicated UI hooks; existing inventory/ring hooks stay single-owner.
void Install();
bool CanDrawOverlay();
}
