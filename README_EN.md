# Stardew Valley Switch Native Mod

A native single-player mod bundle for Stardew Valley on Nintendo Switch: automation, Skull Cavern elevators, four ring slots, UI information, automatic fishing, NPC world-map tracking and faster animations.

[中文](README.md) | [English](README_EN.md) | [Download v13.1.1](https://github.com/zmzhang2022-ai/Switch-StardewValley-Mod/releases/tag/v13.1.1) | [Report an issue](https://github.com/zmzhang2022-ai/Switch-StardewValley-Mod/issues)

## Current release

**v13.1.1 — prerelease.** This release adds FastAnimations to the v13 bundle and fixes the black screen reported after leaving a building in v13.1. Source, build and offline checks have passed. **The fix has not yet been retested on Switch hardware.**

v13.1 incorrectly clamped the fade state to `0–1`. The original game waits for a value above `1.1` or below `-0.1` before running its transition/completion callbacks. v13.1.1 removes that clamp and keeps the original completion flow.

- Use v13.1.1 instead of the faulty v13.1 build.
- The LookupAnything v14 branch was abandoned. **LookupAnything is not included.**
- Features are implemented in native C++ with ARM64 hooks. SMAPI is not required, and PC mod DLLs cannot be loaded directly.

## Compatibility

| Item | Required version |
| --- | --- |
| Platform | Nintendo Switch, with Atmosphère loading this overlay |
| Game | Stardew Valley **1.6.15.3** |
| Title ID | `0100E65002BB8000` |
| Game Build ID | `A5C617C14A7F3F6620B3BC8136965A4822D32B9C` |
| Scope | Single-player; multiplayer adaptation is not supported |

Check the Build ID even when the version number matches. Offsets and ABI assumptions apply only to this specific game build.

## Install or update

1. Download **`fast-animations-singleplayer-merged-v13-1-1.zip`** from the [v13.1.1 release page](https://github.com/zmzhang2022-ai/Switch-StardewValley-Mod/releases/tag/v13.1.1).
2. Exit the game completely and keep your previous mod files for rollback.
3. Extract the archive and merge its **`atmosphere` directory into the SD card root**, replacing `subsdk9` and `main.npdm` together.
4. Start the game and test leaving/re-entering the farmhouse and repeated location transitions before testing other features.

The installed layout must be:

```text
SD:/atmosphere/contents/0100E65002BB8000/exefs/
  main.npdm
  subsdk9
```

Do not copy source, toolchains, tests, documentation, audit records or ELF symbols to the SD card. Do not mix `subsdk9` and `main.npdm` from different releases; restore both when rolling back. The repository's `atmosphere/` files match the installation files in this release archive.

## Included features

This table describes implemented behavior, not a claim that every scenario has passed hardware testing.

| Module | Current scope |
| --- | --- |
| Automate Lite | Automatic machine input/output using connected chests, Wood Paths, Fish Pond output and background processing of loaded maps. Connections use the same tile or four cardinal neighbors, not diagonals. Full chests leave products in place. |
| Skull Cavern Elevator | Stops every five floors, including floors beyond 120; a native scrollable menu follows the deepest floor recorded in the save. |
| Four ring slots | Preserves the original two slots and adds two more, including equipment interaction, drawing and effect synchronization. |
| UI information | Item prices and collection hints, machine timers, crop/tree details, effect ranges, skill progress, luck/weather, daily reminders, animal-petting indicators and social information. |
| Automatic fishing | L3+R3 toggles repeated maximum-power casting, bites, automatic minigame completion, perfect/iridium fish settlement, collection and recasting, with low-stamina food selection. |
| NPC world map | Portraits, overlapping-name lists, birthday/daily-quest hints, locatable horses/children and scheduled merchants. Meeting/progression filters are not added. |
| FastAnimations | Selected daily, tool, fishing, transport and menu animations default to 2x, including weapons and slingshots. Settings allow disabling it or selecting 3x; food/drink confirmation is skipped while enabled. |

Automation uses original machine recipes, quantities and fuel rules. Existing handlers cover brewing, preserves, smelting, recycling, seed making, cheese, mayonnaise, gem duplication, aging, dehydrating, smoking fish and passive producers. Map coverage includes loaded locations and instantiated interiors; it does not force-load uninstantiated maps.

### Automatic fishing

- Hold a fishing rod, stand by water and face a valid fishing tile. Press **L3+R3** together to enable it. Release both sticks before pressing again to disable it.
- Fish that enter the minigame are set to iridium quality and use the original perfect-catch settlement. Trash, seaweed and treasure contents are not forced to a quality; manual fishing keeps the original rules.
- It collects treasure that the original game actually generates; **treasure spawn chance is unchanged**. Fish species, quantities and bite waiting time are unchanged.
- Below 10 stamina, it selects stamina-restoring food from the inventory. Fish and cooked dishes can be consumed; Stardrops are excluded.
- No usable food, a full inventory or time at/after 1:00 AM pauses new casts. An existing catch can finish; a full inventory leaves its collection interface available.
- Re-enable after changing locations/tools, sleeping, loading a save or entering an event. There is no automatic bait/tackle refill or pathfinding.

### NPC map and animation speed

- Open the normal world map to see NPC markers. **There is no persistent minimap.**
- Unrestricted visibility means existing, loaded characters with valid original map coordinates. It does not spawn characters or unlock locations. Per-NPC tracking preferences still apply.
- Animation acceleration does not modify the game clock or machine production duration. Manual cast charging, bite waiting and the ordinary fishing minigame are not additionally accelerated.
- Cutscenes keep their original speed, as does the dagger's special multi-hit sequence. Some independent particles and special texture layers remain at normal speed.

## Controls and settings

Open the game menu and click **“UI 信息设置”** (UI information settings) at the lower left. Use up/down to select, left/right to change pages, confirm to toggle and back to save/close. Cursor clicks are also supported. The native settings labels are currently Chinese.

| Setting | Control |
| --- | --- |
| Automatic fishing | L3+R3 together; release both before toggling again |
| Animation speed and food confirmation | “动画加速与免吃喝确认”, enabled by default; disabling restores normal speed and food confirmation |
| Speed multiplier | “动画速度3倍（关闭为2倍）”; off by default means 2x |
| Map tracking master switch | “地图村民位置” in UI settings |
| Individual NPC tracking | “地图 ✓ / ×” on the social page |
| Range display | Optional hold-L3 mode; holding is not required by default |

UI/animation preferences are stored per save under SD `/config/uiinfosuite2/`, with two validated configuration slots. The automatic-fishing toggle is not persisted in the save. Existing v13 preferences are preserved; animation speed defaults to 2x during migration.

## Validation and limitations

| Check | v13.1.1 status |
| --- | --- |
| Static and ARM64 compile-time regression checks | Passed for configuration, UI, fishing, NPC map and fade progression |
| Isolated original ARM64 fade branches | Six boundaries and 42 timing cases passed; the full game/callbacks are not executed |
| Local compilation/linking | Passed, with existing framework warnings; not a warning-free build |
| Distribution integrity | NSO segment decompression/hashes, paired files, ZIP and source manifest passed |
| Hook budget | 32 of 40 slots used, eight spare; the animation module adds no hooks |
| Hardware testing after the fix | **Pending** for transitions, input, rendering, performance and combined features |

Modules share the game process. No address overlap is not proof of runtime isolation. Animation acceleration reuses original object updates, which can also advance other frame timers on that object; combat, transport, completion callbacks and large-farm performance need hardware regression testing. Feedback for older builds does not establish complete validation of this bundle.

Multiplayer synchronization, SMAPI/GMCM, all PC-mod extension APIs and arbitrary custom map packs are outside the implemented scope. See the detailed documents below.

## Repository layout

| Path | Contents | Needed on the SD card? |
| --- | --- | --- |
| `assets/` | README assets, including the support QR code | No |
| `atmosphere/` | Paired v13.1.1 `subsdk9` and `main.npdm` | **Yes** |
| `docs/` | Feature documentation, historical design records and build requirements | No |
| `runtime/` | Native C++ source, framework and build configuration | No |
| `tests/` | Compile-time regression test source | No |
| `toolchains/` | Compatibility headers, not a complete compiler toolchain | No |
| `tools/` | Build, static-check, offline-analysis and packaging scripts | No |
| `validation/` | Release test, build and artifact-verification records | No |
| `BUILD_INFO.json` | Source hashes, artifact identity, scope and validation metadata | No |

Fishing, NPC and UI documents retain version numbers and records from their introduction. This README, `BUILD_INFO.json` and the FastAnimations document describe the current combined release.

## Source and development

The actual build entry point is `tools/build_poc_clang.ps1`, using `runtime/source/`. Configure the Windows LLVM/devkitPro paths required by the script. Compiler binaries and original game files are not included. Offline disassembly/emulation checks also require local analysis caches for the target game build and Python dependencies.

- [Build requirements and source layout](docs/BUILD_V13_1_1.md)
- [FastAnimations and the black-screen fix](docs/FAST_ANIMATIONS_SWITCH.md)
- [Automatic fishing](docs/AUTO_FISHING_SWITCH.md)
- [NPC world map](docs/NPC_MAP_LOCATIONS_SWITCH.md)
- [UI information and settings](docs/UIINFO_SUITE2_SWITCH.md)
- [Engineering lessons and open issues](PROJECT_LESSONS.md)

These engineering documents are currently in Chinese. Packaging also requires baseline releases and analysis evidence from the development workspace; it is not dependency-free on a fresh checkout. Version labels are compiled from source. **Do not patch raw bytes in a compressed NSO.** The obsolete binary-branding script has been removed.

## v13.1.1 artifact identity

```text
Release ZIP SHA-256
B0B4FA69E723E67342B297B702A859A4F24464F1F5359BCA2B548B67BF5FF52A

subsdk9 SHA-256
8F43C7ECDFB4055D2EAC3B49B0F22C11A1D771D0754B24F7CC81FD80610C4415

main.npdm SHA-256
F63D42112A3866CF6BF04ABD011F30BB3DF5E852BE244A2200FDDCD9A4898D85

Mod Module ID
995CAEC73CA65CEB3A5D7E682005EBAC245500B1
```

For bug reports, include the mod version, game Build ID, reproduction steps, relevant logs and whether disabling animation acceleration changes the result.

## Credits and license

The native implementation draws on feature ideas from Automate, Skull Cavern Elevator, UI Info Suite 2, Yet Another Fishing Mod, NPC Map Locations and Fast Animations. The runtime framework is based on [exlaunch](https://github.com/shadowninja108/exlaunch). This Switch implementation does not imply support or endorsement from the original PC-mod authors. Third-party files retain their existing license notices.

Copyright © 2026 `zmzhang2022-ai`. Original project mod code is licensed under [GPL-2.0-only](LICENSE). Original game executables and game assets are not distributed in this repository.

## Support the project

Thank you for using, testing and supporting the project. If you do not have a stable income or are experiencing financial hardship, please do not donate; take care of yourself and your family first.

<p align="center"><img src="assets/wechat-reward-code.png" alt="WeChat support QR code" width="360"></p>
