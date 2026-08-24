# Stardew Valley Switch 原生 Mod：已实现功能

更新时间：2026-08-24  
当前版本：v9 Preserves Jar + Geode Crusher Performance  

## 1. 目标游戏版本

```text
游戏：Stardew Valley / 星露谷物语
版本：1.6.15.3
Title ID：0100E65002BB8000
Game Build ID：A5C617C14A7F3F6620B3BC8136965A4822D32B9C
```

所有函数 offset、虚函数槽位和 ABI 结论仅适用于上述 Build ID。游戏更新或 Build ID
变化后必须重新分析，不能直接复用。

## 2. 注入与部署架构

当前实现采用：

```text
Atmosphère
  + exlaunch/subsdk runtime injection
  + ARM64 function hooking
  + 原生 C++ Automate Lite
```

原始 `exefs/main` 没有被永久修改。Mod 通过 `subsdk9` 注入，并配套使用已经补充所需
SVC 权限的 `main.npdm` overlay。

部署结构：

```text
atmosphere/
└─ contents/
   └─ 0100E65002BB8000/
      └─ exefs/
         ├─ subsdk9
         └─ main.npdm
```

## 3. Automate Lite 网络

当前支持以下连接形式：

```text
[箱子]—[机器]
[箱子]—[木头小径]—[机器]
[箱子]—[机器]—[机器]
[箱子]—[木头小径]—[机器]—[机器]
```

连接和分组规则：

- 箱子、机器、鱼塘和木头小径被索引为自动化实体；
- 同一格覆盖的实体会进入同一个 flood fill；
- 实体通过上、下、左、右四个方向连接，不使用斜向连接；
- 机器本身可以继续连接下一台机器；
- 同一连接组可以包含多个箱子和多台机器；
- 每个 Automate Tick 先收取成品，再尝试下一轮进料；
- 箱子满时不清空机器，成品保留在原机器中；
- 不通过直接写 timer、heldObject 或库存 stack 模拟交易。

关键游戏原始接口：

```text
Chest.AddItem(...)
Object.AttemptAutoLoad(IInventory, Farmer)
Object/Machine 原始 output lifecycle
```

输入的有效物品、配方、数量和燃料要求由游戏原版机器数据决定。

## 4. 已支持机器

| 机器 | 自动进料 | 自动出料 | 说明 |
|---|---:|---:|---|
| 压酪机 / Cheese Press | 是 | 是 | 原版配方判断牛奶类型 |
| 回收机 / Recycling Machine | 是 | 是 | 原版配方处理可回收垃圾 |
| 太阳能板 / Solar Panel | 不需要 | 是 | 自主产出型机器 |
| 完美雕像 / Statue of Perfection | 不需要 | 是 | 自主产出型机器 |
| 宝石复制机 / Crystalarium | 是 | 是 | 使用原版宝石输入和连续产出逻辑 |
| 小桶 / Keg | 是 | 是 | 使用原版 Data/Machines 配方 |
| 无尽财富之雕像 / Statue of Endless Fortune | 不需要 | 是 | 自主产出型机器 |
| 木桶 / Cask | 是 | 是 | 由原版逻辑判断可陈酿物品 |
| 树液采集器 / Tapper | 不需要 | 是 | 收取后调用原版 Tree product refresh |
| 烘干机 / Dehydrator | 是 | 是 | 使用原版多物品配方 |
| 熏鱼机 / Fish Smoker | 是 | 是 | 输入与燃料由原版逻辑处理 |
| 熔炉 / Furnace | 是 | 是 | 原版逻辑处理矿石和煤炭 |
| 真正完美的雕像 / Statue of True Perfection | 不需要 | 是 | 自主产出型机器 |
| 种子生产机 / Seed Maker | 是 | 是 | 支持原版随机种子输出 |
| 蛋黄酱机 / Mayonnaise Machine | 是 | 是 | 原版配方判断蛋类 |
| 避雷针 / Lightning Rod | 不需要 | 是 | 自主产出型机器 |
| 重型树液采集器 / Heavy Tapper | 不需要 | 是 | 树木产物刷新走原版接口 |
| 重型熔炉 / Heavy Furnace | 是 | 是 | 使用原版批量熔炼配方 |
| 罐头机 / Preserves Jar | 是 | 是 | `(BC)15`，v9 新增 |
| 晶球破开器 / Geode Crusher | 是 | 是 | `(BC)182`，v9 新增；1.6 不再需要煤炭 |

其中罐头机和晶球破开器已经完成源码、ELF、AArch64 和 NSO 静态验证，真实连续进料、
完成出料及箱满保护仍需在 Switch 真机完成闭环。

## 5. 木头小径 Connector

木头小径作为 Automate 网络连接器。

识别链：

```text
GameLocation.terrainFeatures raw field
  → NetVector2Dictionary +0x210/+0x218 virtual unwrap
  → TerrainFeature*
  → is Flooring
  → Flooring.GetData()
  → FloorsAndPathsData.ItemId
  → "405" 或 "(O)405"
```

terrainFeatures 的 concrete dictionary values 是 `NetRef<TerrainFeature>` 风格的网络字段
包装器，不能直接作为 `TerrainFeature*` 使用。当前实现复现游戏原版虚函数解包流程。

每一格木头小径作为独立 Connector 节点，通过同格和上下左右 flood 连接箱子与机器。

## 6. 鱼塘

鱼塘实现为输出型 Automate 机器：

- 自动识别 Fish Pond building；
- 按鱼塘完整 TileArea 加入自动化索引；
- 自动读取已经生成的鱼塘产物；
- 自动把鱼籽等产物放入连接箱子；
- 箱子满时保留鱼塘产物；
- 支持直接相邻和木头小径连接；
- 不自动向鱼塘投入鱼或任务物品。

## 7. 全地图与室内更新

Automate 不限于玩家附近区域。

覆盖范围：

- 当前玩家所在 Location；
- `Game1.locations` 中所有已加载根地图；
- 农舍、小屋等已经实例化的建筑室内；
- 玩家不在场时仍可进行跨地图自动进料和出料。

Mod 不会仅为了自动化而主动加载尚未加载的 Location。

## 8. 性能调度

旧实现曾每 30 Tick 在一个主线程帧中扫描全部地图，造成周期性卡顿。v8 起改为：

```text
当前地图：每 30 Tick 扫描
后台地图：每 4 Tick 轮询一个非当前 Location
```

性能优化包括：

- 全地图工作跨帧轮询，仍保持完整覆盖；
- 后台状态只保存 root/interior 索引，不跨 Tick 保存托管 `GameLocation*` 裸指针；
- terrain unwrap 虚函数由“每个 feature 解析一次”改为“每张地图解析一次”；
- flood tile 去重从线性 `std::find` 改为开放寻址哈希，期望复杂度 O(1)；
- 沙漠电梯保持 30 Tick 周期，但与当前地图 Automate 扫描错开 15 Tick；
- 长 heartbeat 日志约每 1800 Tick 输出一次，并避开 Automate 扫描帧。

v8 的周期性卡顿优化已经由 Switch 真机确认成功。

## 9. 沙漠矿井电梯

当前包含：

- 沙漠矿井大厅电梯；
- 在对应地图位置放置电梯图标；
- 在符合条件的 Skull Cavern 楼层生成电梯；
- 支持 120 层以上；
- 每 5 层作为一个电梯层；
- 使用可滚动的原生菜单显示深层楼层；
- 使用游戏保存的最深楼层决定可选范围；
- 与 Automate 共存于同一个 `subsdk9`。

## 10. 四戒指

当前包含：

- 保留原版两个戒指栏位；
- 新增两个戒指栏位；
- 总计四个戒指栏位；
- 使用 Layout 2 布局；
- 避免与帽子、衣服、裤子、鞋子及战斗宠物区域重叠；
- 支持戒指放入、取出、绘制、悬停和效果同步；
- 与 Automate、鱼塘和沙漠矿井电梯共存。

