# UI Info Suite 2 — Switch 单人移植与验收

2026-09-16：v13 已用 NPC Map Locations 大地图实现替换下文历史首字标记方案，新增头像、重叠名单及任务/生日提示；无小地图。当前范围与验证见 [NPC 地图完整设计](NPC_MAP_LOCATIONS_SWITCH.md)。下文保留 v11.1 的功能与历史验收记录。

更新：2026-09-14。版本：Automate Lite v11.1 + UI Info Suite 2 Singleplayer。

v11 已被用户日志确认启动崩溃：共享 Hook 池不足，在安装第 22 个 Hook 时中止。v11.1 将池扩为 8 KB（40 槽），修正边界并增加构建前预算检查；功能代码保持原样。**v11.1 真机验证未执行**；2026-09-12 用户“全部可以”的反馈只覆盖此前 B1。下表“已接入”表示代码实现与本地检查，不等于逐项真机验收通过。

## 范围与版本

- 以本地 UI Info Suite 2 v2.3.7 的原版单人功能为目标，合并同一个 subsdk9，保留 Automate、木头小径连接、鱼塘出料、骷髅洞穴电梯、四戒指。
- 固定 Switch 游戏 1.6.15.3，Title ID `0100E65002BB8000`，main Build ID `A5C617C14A7F3F6620B3BC8136965A4822D32B9C`。
- 用户明确不做联机；好友玩家位置、主客机适配及联机测试不在范围内。
- 原生 C++/ARM64 AOT 实现，不加载 SMAPI。GMCM、Custom Bush、Level Extender、Deluxe Journal 等其他 PC Mod 接口不属于原版单人功能。
- 使用游戏现有字体、贴图和音效。中文文字 HUD、姓名首字地图标记、浮动升级文字替代 PC 的图标/头像/升级素材，不是逐像素复刻。

## 功能覆盖

以下均已接入；各行新增行为均待真机验证。源码位于 `runtime/source/uiinfo/`。

| 功能 | 当前实现 | 源码 |
| --- | --- | --- |
| 虚拟光标 | 原版右摇杆鼠标和 currentCursorTile，世界悬停跟随光标 | ui_info_suite.cpp |
| 物品售价 | 通用 drawToolTip 追加单价和 64 位整组售价，使用实际售卖虚方法，包含品质、职业、利润修正 | items.cpp |
| 收集提示 | 博物馆尚未捐赠、首次出货、社区中心献祭需要 | items.cpp |
| 普通机器 | 当前产物、剩余游戏小时/分钟、完成提醒 | ui_info_suite.cpp |
| 熟成桶 | daysToMature / agingRate 向上取整，显示距铱星熟成天数 | ui_info_suite.cpp、logic.hpp |
| 作物/种植盆 | 实际 phaseDays、再生倒计时、浇水、肥料、枯萎、盆内作物/茶树 | world.cpp |
| 树木/茶树 | 树种、阶段、树桩、树肥，含绿雨树/神秘树；茶树成长与采摘季 | world.cpp |
| 果树 | 成熟天数、生长阻挡、雷击恢复、结果季、果实名称和当前数量 | world.cpp |
| 建筑产物 | 按 ItemConversions 的 SourceChest/DestinationChest 显示待加工和产出物品；鱼塘产出 | world.cpp |
| 作用范围 | 稻草人、普通/加压洒水器、蜂房、祝尼魔屋、蘑菇树桩、苔藓种子、三种炸弹；手持放置/悬停、重叠着色 | ranges.cpp |
| 范围控制 | 炸弹独立开关、附近全部范围、按住左摇杆才显示；默认无需按住 | settings.cpp、ranges.cpp |
| 经验 | 五技能 XP、等级进度、获得量、淡出、工具选择技能、升级浮动提示和原版 newRecipe 音效 | hud.cpp |
| 运势/天气 | 实际每日运势及四位小数选项、星露谷明日天气、已有姜岛天气记录 | hud.cpp |
| 日常提醒 | 生日/满好感隐藏/今日礼物、货车/访问隐藏、电视未学菜谱、工具/住宅/罗宾施工、莓果/榛子季 | hud.cpp |
| 动物/宠物 | 未抚摸标记、满好感隐藏、产物、悬停好感与名称 | animals.cpp |
| 社交 | 实际好感、按列填充未满心、今日/每周礼物、逐 NPC 地图追踪 | social.cpp |
| NPC 地图 | 原版区域坐标映射、姓名首字标记、重叠悬停列出姓名和礼物状态 | npc_map.cpp |
| 商店 | 种子和果树苗的普通品质收成售价、作物产量/再生间隔；不预测混合种子 770 | shop.cpp |
| 日历/委托 | 面板打开原版 Billboard 日历/每日委托 | menu.cpp |
| 配置 | 29 个开关、每存档独立、默认继承、逐 NPC 追踪、SD 双槽校验恢复 | settings.cpp、settings_format.hpp |

旧背包售价追加入口已从 `runtime/source/hooks/wear_more_rings.cpp` 移除，避免与通用提示重复；四戒指装配、点击、保存、绘制逻辑保留。

## 操作与配置

1. 世界中用原版右摇杆移动光标到目标。某控制设置若不移动光标，记录该设置和场景后排查，不以静态输入链代替手感验收。
2. 打开游戏主菜单，用光标点击左下“UI 信息设置”。拖着物品或当前页面不允许关闭时，先放回物品；入口遵循原版 readyToClose。
3. 方向键上下选择、左右翻页、确认键切换、返回键保存关闭，也支持光标点击。面板底部显示保存结果。
4. 最后一页提供新存档默认设置、日历、每日委托、保存返回。日历开关关闭时显示关闭状态。
5. 社交页右侧“地图 ✓ / ×”切换村民追踪并立即保存。
6. 范围默认自动显示。启用按键选项后按住 L3；附近全部范围由独立选项控制。L3 是新增可选操作，真机冲突检查未执行。

配置在 SD 卡 `/config/uiinfosuite2/`，不写游戏存档 NetFields。存档 ID 的 16 位十六进制加 `-a.cfg` / `-b.cfg` 为双槽；全零 ID 为新存档默认。格式 v2，每条 5152 字节，含开关、ID、序号、校验、最多 64 个隐藏 NPC 完整内部名。名字最多 39 个 UTF-16 字符，超限拒绝，不截断误匹配。

保存写另一槽，Flush 后重新读取确认；失败保留旧有效槽。缺失/损坏选最新有效记录，否则回到默认。SD 挂载失败允许临时调整，面板显示仅在内存保留。**实际 SD 挂载、文件写入、退出重启恢复未执行**。

## 调用链与证据

PC 参考为 `PCmod/UIInfoSuite2/UIInfoSuite2.dll`，SHA-256 `B7C691B546724B71E604CD64C03A1F101C92CDC1E30F43D5BFC32E12A0B76239`。原作者 Annosz，manifest 2.3.7；没有 C# 工程。PC 引用游戏 1.6.14.24317，不能照搬对象布局。

`tools/extract_aot_metadata.py` 限制元数据工厂的外部执行入口，离线解码名称、字段、invoker；`tools/inspect_uiinfo_aot.py` 先核对 main Build ID、缓存段哈希，再查询指令。元数据并非 ABI 的充分证明，另核对实际 invoker/消费者。

| 调用 | 参数与证据 |
| --- | --- |
| HUD 13E7C30 | 完整入口 void(Game1*)，沿用调用方 SpriteBatch，不额外 Begin/End |
| 字符串 | 12340 分配，长度 +10、UTF-16 +14；不用 12CE0 动态 intern |
| 文本 1AED050 | String 重载；1AECD00 是 StringBuilder，禁止替换 |
| 字典 18EAE50 | x0 字典、s0/s1 Vector2、x1 Object**；先解包再识别对象 |
| tooltip 16F91B0 | 静态 12 参数，末四参数经栈传递；ELF 已复查原参数转发 |
| XP 1321E40 | Orig 前后读取本地玩家真实 NetIntArray 增量 |
| 社交 1793420/1792840 | 心形 pageX+316、portraitY+36/+64，行 Y 用原版 getter |
| 地图 1B0E7B0 | x0 location、x1 为 8 字节 Point、x8 为 32 字节 NullablePosition 出参；1B0CC70 返回 HFA Vector2 后加 mapBounds |
| BFF70 绘制 | Rectangle 占 x2/x3，x4 指向 24 字节 NullableRectangle，颜色 x5 |
| 果树阻挡 1A5A660 | 静态 bool(Vector2,GameLocation*)；7552574 确认 s0/s1+x0 |
| 熟成 195ABD0 | +220 daysToMature、+218 agingRate，NetFloat 值 +58，原版每天相减 |
| 建筑 1154C00 | 按转换数据箱名获取真 Chest，用既有 ChestActions.GetItems 读取 |
| TV | 不调用会学菜谱的 199D930；只读 CookingChannel/knowsRecipe；重播计算使用独立日期种子 RNG |
| 姜岛天气 | 不调用缺失时创建记录的 GetWeatherForLocation，只走 TryGetValue 成功分支 |
| 献祭缓存 | CommunityCenter 两个构造函数都刷新 +368；初始化未完成跳过，不创建地图 |
| SD 配置 | 9 个新增 nn::fs 导入均存在于游戏 SDK；原 NPDM ACI0/ACID 文件权限均为 4000000000000000，未新增文件权限 |

本地证据：`analysis/uiinfo-*.json`、`uiinfo-pc-*.il`、`uiinfo-bundle-cache.asm`、`uiinfo-cask-aging.asm`、`uiinfo-collection-tv-rerun.asm`、`uiinfo-map-social-render.asm`、`uiinfo-v11-elf.asm`。

## 本地验证

- `tools/test_uiinfo.ps1`：通过 ARM64 编译期断言，包含文本指针/溢出/64 位价格、经验临界值、作物成熟/再生、熟成取整、季节/稻草人边界、配置损坏/错误 ID/隐藏名/序号回绕。
- `tools/build_poc_clang.ps1`：通过，三个诊断开关 false；仍有原框架 reloc.hpp 返回路径、Hook linkage、汇编无用参数 warning，不称为零 warning。
- 新增导入与 SDK 一致：`analysis/uiinfo-v11-import-audit.json`。NPDM 权限实测字节：`analysis/uiinfo-filesystem-permission-audit.json`；实际读写未执行。
- FruitTree/GameMenu 元数据重提取与旧结果一致，不等于调用游戏方法的运行结果通过。
- NSO text/rodata/data 解压、大小、段哈希全部通过，staging/normal 字节一致：`analysis/uiinfo-singleplayer-nso-audit.json`。
- 原功能源码对比 B1 保留，详见 `analysis/uiinfo-baseline-source-comparison.json`；共享 runtime 与 Automate 调度未修改。
- 真机启动、帧率、布局、摇杆、新功能和 SD 配置恢复：**未执行**。

## 真机验收清单

逐项记录包哈希、存档、地点、缩放设置、操作和结果，不将某项通过扩大为全表通过。

- [ ] 启动、读档、睡觉保存、退出重进、版本日志。
- [ ] Automate 原机器、木头小径四方向连接、鱼塘、电梯、四戒指装卸/属性/重载。
- [ ] 背包/箱子/商店/戒指提示不重复；品质、职业、堆叠售价正确。
- [ ] 博物馆/献祭/出货前后更新；Joja 或社区完成存档不误报。
- [ ] 普通/再生/加速作物、种植盆、茶树、绿雨树、果树阻挡/结果季/雷击、熟成桶。
- [ ] 磨坊输入输出、祝尼魔库存、鱼塘、多物品不越界。
- [ ] 普通/高级稻草人、四向/品质/铱金/加压洒水器、蜂房、树桩、苔藓种子、祝尼魔屋、三种炸弹范围。
- [ ] 手持位置与原版放置预览一致；重叠、全部、L3 可选控制、大农场帧率。
- [ ] 五技能 XP、升级提示音效、工具切换、淡出/常显、满级。
- [ ] 周日/周三已学/未学菜谱、生日礼物/满好感、货车访问、天气、升级施工、季节提醒。
- [ ] 室内外动物和猫狗、抚摸前后/满好感/产物、屏幕边缘悬停。
- [ ] 社交部分心/礼物/追踪；地图室内外/姜岛/重叠人物。
- [ ] 商店种子/果树苗、职业/利润率、混合种子不编造收成。
- [ ] 拖物品时不切换面板；日历/委托可退出；方向键/确认/返回/分页。
- [ ] 开关逐项生效、退出重进保存、两存档隔离、新存档继承默认。
- [ ] 备份配置后模拟最新槽损坏，旧槽恢复；SD 不可写提示。
- [ ] 掌机/底座、UI/地图缩放、事件、截图、隐藏 HUD、过图，无新增错误或遮挡。

## 已知边界

- 这是待真机验收候选，全套代码接入不能表述为移植已经全部验收。
- 只遍历已加载地点和已实例化室内，不为显示创建地图；无原版映射的 NPC 跳过。地图/动物各 128 标记，隐藏 NPC 64 个，针对原版单人。
- 日常缓存 1 秒、地图/商店 0.5 秒、范围 0.25 秒；只缓存原生复制值，无跨回调托管裸指针。
- 范围最多绘制 128×80 视口网格、4096 个可见相关范围；极端缩放/异常数量不保证全部覆盖。
- 建筑最多显示 8 个物品，更多提示打开库存；果树最多列 4 种果实。其他 Mod 条件掉落不在范围。
- 日常 HUD、地图首字和提示框的遮挡与字体效果待真机；视口高度小于 360 时不打开面板以避免无效布局。
- 升级用原版音效而非 PC LevelUp.wav；心形精度为贴图列，不足一列的进度不绘制。
- 手持范围会临时设置并恢复原版放置检查标志；游戏调用发生异常时的传播路径仍需真机日志确认。
- 新 UI 检查 Hook 局部指纹；旧 Mod 完整运行期 Build ID 门控仍是手册开放风险，不能用于其他游戏版本。

## 交付与回退

修复候选：`deploy/uiinfo-suite2-singleplayer-merged-v11-1.zip`。退出游戏后仅合并包内 `atmosphere` 到 SD 根目录，成对安装 subsdk9/main.npdm。源码、symbols、校验文件不要放进 exefs。旧 v11 包保留作崩溃证据，不再安装。

- NSO SHA-256：`9ABE49AE7526BEBA703D7F54126F5EC8A3ECD588BC69F4D797BF26A49B2D5786`。
- Module ID：`730AAC884C1050A82E1349A2872596F88DC7629F`。
- NPDM SHA-256：`F63D42112A3866CF6BF04ABD011F30BB3DF5E852BE244A2200FDDCD9A4898D85`。
- 以 BUILD_INFO.json 与独立 ZIP 哈希核对。构建打包未写实际 SD 卡。

B1 回退包 `deploy/uiinfo-suite2-b1-merged-v10.zip` 保留原样；备份在 `analysis/uiinfo-b1-tested-baseline-20260912/`。回退时成对恢复 B1 的 subsdk9/main.npdm，v11 配置可保留。禁止使用历史 F2473DF7 损坏产物或对压缩 NSO 再定点打标。
