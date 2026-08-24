# Stardew Valley Switch Native Mod

[中文](#中文) | [English](#english)

> A native Nintendo Switch mod for Stardew Valley, built with Atmosphère,
> exlaunch/subsdk runtime injection, ARM64 hooks, and C++.

## 中文

### 项目简介

这是针对 Nintendo Switch 版《Stardew Valley / 星露谷物语》重新实现的原生 Mod。
项目不移植 SMAPI，也不直接运行 PC 版 Mod DLL，而是通过 Atmosphère 运行时注入和
ARM64 Hook 实现轻量级 Automate、沙漠矿井电梯及四戒指等功能。

目标版本：

```text
游戏版本：Stardew Valley 1.6.15.3
Title ID：0100E65002BB8000
Game Build ID：A5C617C14A7F3F6620B3BC8136965A4822D32B9C
```

所有函数 offset、虚函数槽位与 ABI 结论仅适用于上述 Build ID。游戏更新后不能直接
复用，必须重新分析。

### 部署方法

将仓库中的 `atmosphere` 目录复制到 SD 卡根目录，合并后结构应为：

```text
atmosphere/
└─ contents/
   └─ 0100E65002BB8000/
      └─ exefs/
         ├─ main.npdm
         └─ subsdk9
```

原始 `exefs/main` 不会被永久修改。`subsdk9` 负责运行时注入，`main.npdm` overlay
提供 Mod 所需的 SVC 权限。

### Automate Lite

支持下列自动化网络：

```text
[箱子]—[机器]
[箱子]—[木头小径]—[机器]
[箱子]—[机器]—[机器]
[箱子]—[木头小径]—[机器]—[机器]
```

- 箱子、机器、鱼塘和木头小径会被加入自动化网络；
- 使用同格及上下左右四向连接，不使用斜向连接；
- 机器可以继续连接其他机器；
- 一个网络可以包含多个箱子与多台机器；
- 每次更新先收取成品，再尝试下一轮进料；
- 箱子已满时不会清空机器，成品保留在原机器中；
- 自动化调用游戏原始 `Chest.AddItem(...)`、`Object.AttemptAutoLoad(...)` 和机器
  output lifecycle，不直接硬写 timer、`heldObject` 或库存 stack；
- 有效原料、配方、数量与燃料要求由游戏原版机器数据决定。

### 支持的机器

| 机器 | 自动进料 | 自动出料 | 说明 |
|---|---:|---:|---|
| 压酪机 | 是 | 是 | 原版逻辑判断牛奶类型 |
| 回收机 | 是 | 是 | 原版逻辑处理可回收垃圾 |
| 太阳能板 | 不需要 | 是 | 自主产出型机器 |
| 完美雕像 | 不需要 | 是 | 自主产出型机器 |
| 宝石复制机 | 是 | 是 | 原版宝石输入及连续生产逻辑 |
| 小桶 | 是 | 是 | 原版 `Data/Machines` 配方 |
| 无尽财富之雕像 | 不需要 | 是 | 自主产出型机器 |
| 木桶 | 是 | 是 | 原版逻辑判断可陈酿物品 |
| 树液采集器 | 不需要 | 是 | 收取后调用原版树木产物刷新 |
| 烘干机 | 是 | 是 | 原版多物品配方 |
| 熏鱼机 | 是 | 是 | 原版逻辑处理输入和燃料 |
| 熔炉 | 是 | 是 | 原版逻辑处理矿石和煤炭 |
| 真正完美的雕像 | 不需要 | 是 | 自主产出型机器 |
| 种子生产机 | 是 | 是 | 支持原版随机种子输出 |
| 蛋黄酱机 | 是 | 是 | 原版逻辑判断蛋类 |
| 避雷针 | 不需要 | 是 | 自主产出型机器 |
| 重型树液采集器 | 不需要 | 是 | 原版树木产物刷新 |
| 重型熔炉 | 是 | 是 | 原版批量熔炼配方 |
| 罐头机 | 是 | 是 | `(BC)15`，v9 新增 |
| 晶球破开器 | 是 | 是 | `(BC)182`，v9 新增；1.6 不再需要煤炭 |

罐头机与晶球破开器已经通过源码、ELF、AArch64 和 NSO 静态验证；连续进料、完成
出料及箱满保护仍需在 Switch 真机完成闭环验证。

### 木头小径连接器

木头小径物品 ID `405` / `(O)405` 可作为 Automate 网络连接器。每格小径是一个独立
节点，通过同格及上下左右 flood fill 连接箱子与机器。

terrain feature 的 concrete dictionary value 是类似 `NetRef<TerrainFeature>` 的网络
字段包装器。当前实现复现游戏原版虚函数解包流程，再通过 `Flooring.GetData()` 验证
`FloorsAndPathsData.ItemId`。

### 鱼塘

- 鱼塘作为自动出料机器；
- 按完整 TileArea 加入自动化索引；
- 鱼籽等已经生成的产物会自动进入连接箱子；
- 箱子满时保留鱼塘产物；
- 支持直接相邻及木头小径连接；
- 不自动投入鱼或任务物品。

### 全地图与性能

Automate 覆盖当前 Location、`Game1.locations` 中所有已加载根地图，以及农舍、小屋等
已经实例化的建筑室内。玩家不在场时仍可跨地图自动进料和出料，但 Mod 不会为了
自动化主动加载尚未加载的 Location。

性能调度：

```text
当前地图：每 30 Tick 扫描
后台地图：每 4 Tick 轮询一个非当前 Location
```

全地图工作被分散到多个帧；terrain unwrap 每张地图只解析一次；flood tile 去重使用
开放寻址哈希；后台状态不跨 Tick 保留托管 `GameLocation*` 裸指针。v8 的周期性卡顿
优化已由 Switch 真机确认成功。

### 沙漠矿井电梯

- 沙漠矿井大厅与符合条件的 Skull Cavern 楼层会生成电梯和图标；
- 支持 120 层以上；
- 每 5 层作为一个电梯层；
- 使用可滚动的原生菜单；
- 根据存档记录的最深楼层决定可选范围；
- 与 Automate 共存于同一个 `subsdk9`。

### 四戒指

- 保留原版两个戒指栏位并新增两个，总计四个；
- 使用 Layout 2，避免与帽子、衣服、裤子、鞋子和战斗宠物区域重叠；
- 支持戒指放入、取出、绘制、悬停与效果同步；
- 与 Automate、鱼塘及沙漠矿井电梯共存。

---

## English

### Overview

This is a native mod for the Nintendo Switch version of Stardew Valley. It does
not port SMAPI or run PC mod DLLs. Instead, it implements lightweight Automate,
a Skull Cavern Elevator, four ring slots, and related features through
Atmosphère runtime injection and ARM64 hooks.

Target build:

```text
Game version: Stardew Valley 1.6.15.3
Title ID: 0100E65002BB8000
Game Build ID: A5C617C14A7F3F6620B3BC8136965A4822D32B9C
```

All function offsets, virtual-function slots, and ABI findings apply only to
this Build ID. They must be re-analyzed after a game update.

### Installation

Copy the repository's `atmosphere` directory to the root of the SD card and
merge it so the final layout is:

```text
atmosphere/
└─ contents/
   └─ 0100E65002BB8000/
      └─ exefs/
         ├─ main.npdm
         └─ subsdk9
```

The original `exefs/main` is not permanently modified. `subsdk9` performs the
runtime injection, while the `main.npdm` overlay supplies the SVC permissions
required by the mod.

### Automate Lite

Supported network layouts:

```text
[Chest]—[Machine]
[Chest]—[Wood Path]—[Machine]
[Chest]—[Machine]—[Machine]
[Chest]—[Wood Path]—[Machine]—[Machine]
```

- Chests, machines, Fish Ponds, and Wood Paths participate in automation networks.
- Same-tile and four-directional connections are used; diagonal connections are not.
- Machines can connect onward to other machines.
- One network may contain multiple chests and machines.
- Each update collects finished products before attempting the next input cycle.
- If every chest is full, the finished product remains in its machine.
- Automation calls the original `Chest.AddItem(...)`, `Object.AttemptAutoLoad(...)`,
  and machine output lifecycle instead of directly writing timers, `heldObject`,
  or inventory stacks.
- The original game data determines valid inputs, recipes, quantities, and fuel.

### Supported Machines

| Machine | Auto-input | Auto-output | Notes |
|---|---:|---:|---|
| Cheese Press | Yes | Yes | Original logic selects the milk type. |
| Recycling Machine | Yes | Yes | Original logic handles recyclable trash. |
| Solar Panel | Not required | Yes | Self-producing machine. |
| Statue of Perfection | Not required | Yes | Self-producing machine. |
| Crystalarium | Yes | Yes | Original gem-input and continuous-production logic. |
| Keg | Yes | Yes | Original `Data/Machines` recipes. |
| Statue of Endless Fortune | Not required | Yes | Self-producing machine. |
| Cask | Yes | Yes | Original logic selects products that can be aged. |
| Tapper | Not required | Yes | Refreshes the original tree product after collection. |
| Dehydrator | Yes | Yes | Original multi-item recipe. |
| Fish Smoker | Yes | Yes | Original logic handles input and fuel. |
| Furnace | Yes | Yes | Original logic handles ore and coal. |
| Statue of True Perfection | Not required | Yes | Self-producing machine. |
| Seed Maker | Yes | Yes | Supports the original randomized seed output. |
| Mayonnaise Machine | Yes | Yes | Original logic selects the egg type. |
| Lightning Rod | Not required | Yes | Self-producing machine. |
| Heavy Tapper | Not required | Yes | Uses the original tree-product refresh. |
| Heavy Furnace | Yes | Yes | Original batch-smelting recipe. |
| Preserves Jar | Yes | Yes | `(BC)15`, added in v9. |
| Geode Crusher | Yes | Yes | `(BC)182`, added in v9; coal is no longer required in 1.6. |

The Preserves Jar and Geode Crusher have passed source, ELF, AArch64, and NSO
static verification. Continuous input/output and full-chest protection still
require end-to-end verification on real Switch hardware.

### Wood Path Connectors

Wood Paths with item ID `405` / `(O)405` act as Automate network connectors.
Each path tile is an independent node and joins chests and machines through
same-tile and four-directional flood fill.

Concrete `terrainFeatures` dictionary values are network-field wrappers similar
to `NetRef<TerrainFeature>`. The implementation reproduces the original virtual
unwrap flow and validates `FloorsAndPathsData.ItemId` through
`Flooring.GetData()`.

### Fish Ponds

- Fish Ponds are output-only Automate machines.
- Their complete TileArea is added to the automation index.
- Roe and other generated products are moved into connected chests.
- Products remain in the pond when all chests are full.
- Both direct adjacency and Wood Path connections are supported.
- Fish and quest items are not inserted automatically.

### Full-map Coverage and Performance

Automate covers the current Location, every loaded root map in
`Game1.locations`, and instantiated interiors such as Farmhouses and Cabins.
Cross-map input and output continue while the player is elsewhere, but the mod
does not force-load unloaded Locations solely for automation.

Scheduling:

```text
Current map: scan every 30 ticks
Background maps: poll one non-current Location every 4 ticks
```

Full-map work is distributed across frames; terrain unwrap is resolved once per
map; flood-tile deduplication uses open-addressing hashing; and managed
`GameLocation*` raw pointers are not retained across background ticks. The v8
periodic-stutter fix has been confirmed on real Switch hardware.

### Skull Cavern Elevator

- Elevators and icons are generated in the Skull Cavern entrance and eligible floors.
- Floors beyond level 120 are supported.
- One elevator stop is available every five floors.
- Deep floors are shown in a native scrollable menu.
- The selectable range follows the deepest floor recorded in the save.
- The feature coexists with Automate in the same `subsdk9` module.

### Four Ring Slots

- The two original ring slots are preserved and two more are added, for four total.
- Layout 2 avoids overlap with the hat, shirt, pants, boots, and combat-pet areas.
- Ring insertion, removal, drawing, hover behavior, and effect synchronization are supported.
- The feature coexists with Automate, Fish Ponds, and the Skull Cavern Elevator.
