# Engineering Lessons / 项目经验规则

复盘日期：2026-09-05。用途：在继续开发前快速定位历史误区，阻止重复猜测、错误部署和未经验证的成功声明。

## 当前 P0：先读这一页

**2026-09-17 用户报告 v13.1 出门黑屏，停止推荐旧包；v13.1.1 为本地修复候选：**已定位明确代码缺陷：新增 Fade 每帧将 alpha 限制在 [0,1]，而原版完成条件是严格 >1.1 / <-0.1，导致切图/淡出回调不可达。问题38记录原版 ARM64 分支隔离复现及修复。仅改动画模块实现、辅助函数与版本号，其他258个源码文件不变。修复 NSO SHA-256 `8F43C7ECDFB4055D2EAC3B49B0F22C11A1D771D0754B24F7CC81FD80610C4415`，Module ID `995CAEC73CA65CEB3A5D7E682005EBAC245500B1`。6个分支边界、42组计时回归、既有静态检查、ARM64构建及NSO三段校验通过；**修复后真机复测未执行**，未收到新故障日志，不排除并存问题。旧 v13.1 包/ELF保留作证据，下条为其交付时记录，已被本条故障反馈取代。

**2026-09-17 FastAnimations v13.1 本地候选，Lookup v14 已作废：**旧 v14 包仅保留历史证据，不作为当前交付。核对 v13/v14 包内全部源码 SHA 后恢复 v13 的 257 个文件；Lookup 新增源码移至 `analysis/fastanimations-v13-baseline/obsolete-v14-lookup/`。新增动画模块仅改 4 个已有源码文件、新增 4 个，253 个 v13 文件不变。默认 2 倍，包含武器/弹弓；设置页开关和 3 倍选项，跳过吃喝确认，剧情保持原速。静态检查、ARM64 构建、NSO 三段解压/哈希及 build/staging 一致性通过。最终 NSO SHA-256 `EB390AAE9F260E2A9CDD4A633F1D81DA6FE6746460E435BDF17D3F26C91C15BE`，Module ID `61929B058A6B1289D5A8214A7FA3064F9FF35759`；无新增 Hook，32/40 槽、余 8。**真机验证未执行，不能宣称运行时绝对无冲突。**详见问题37和 `docs/FAST_ANIMATIONS_SWITCH.md`。

**2026-09-17 Lookup 查询百科 v14 本地候选：**用户选择全部推荐，新增16类查询、ZL+R3、原生菜单暂停与分类浏览。详见问题36及 `docs/LOOKUP_ANYTHING_SWITCH.md`。NSO SHA-256 `E8856FA6B8F27A40463CC01FF1DD17C5B2F2EA0CEB158D00C6D62962442CDE11`，Module ID `31724FF7C8D698CA96F9B781E5DCB56D5EEFAD69`。静态、ARM64构建、三段NSO校验通过；保守Hook计数仍为32/40，余8，未新增Hook。**真机启动、实体按键映射、界面、性能和全部组合回归未执行。**实现字段及PC扩展功能差异以功能账本为准；不得宣称PC版全部扩展字段已移植。独立包保留v13回退。

**2026-09-16 NPC Map Locations 大地图 v13 本地候选：**用户确认不做小地图、允许提前显示隐藏人物；替换旧地图首字标记，复用原版地图坐标/头像绘制阶段。详见问题35及 `docs/NPC_MAP_LOCATIONS_SWITCH.md`。NSO SHA-256 `E435DA1A84403529A3497DB5B7E84D2629342184DE6D47EE0CF1EDBEE48E6578`，Module ID `D039A0E5817D2433A707C88B397A086039266FBB`。静态、ARM64构建、三段NSO校验通过；32/40槽，余8。**真机启动、头像/缩放/性能及组合回归未执行。**v12.1完整包保留，不扩大旧真机反馈范围。

**2026-09-16 自动钓鱼 v12.1 修复候选：**用户报告v12抛竿距离不足、没有高品质鱼，其余功能完好；这是整体反馈，不代表所有边界场景逐项测试通过。详见问题34。
本地v12.1 NSO SHA-256 `E7DFF1CBB60A9E62D657AB08E56EC3A9D5D4EF8224B43311B6F996DE7A94262C`，Module ID `4C05CB5694E9F29406AB4E68F91600874CE2C376`。
静态、编译、NSO三段校验通过；新增抛竿Hook后31/40槽。**v12.1两项修复真机复测未执行。**旧v12完整包仍保留。

**2026-09-15 新增自动钓鱼 v12 本地候选：**基于 v11.1，保留旧功能，复用 Tick/HUD，新增一个 BobberBar.update Hook。
保守预算30/40，余10槽；静态、ARM64编译、NSO三段解压/哈希通过，SDK导入与备份v11.1一致。
NSO SHA-256 `4764A90947872D91AA1D38795FAB4A6EC58AC21AA622A8D9B19D69801F7DEA72`，Module ID `6F0554D7363E61C839B67741D21586C34ED9F61B`。
设计/状态/完整验收见 `docs/AUTO_FISHING_SWITCH.md`，最终包 `deploy/auto-fishing-singleplayer-merged-v12.zip` 的 BUILD_INFO 为交付身份。
v12交付时真机验证未执行；之后用户报告除抛竿距离和鱼品质外其余功能完好，具体边界场景未逐项反馈。本条不替代以下 v11/v11.1 故障记录。

**2026-09-14 崩溃更新：v11 已由用户日志确认在启动安装第 22 个 Hook 时中止，不再是可用候选。详见问题 33。**
旧 v11 ZIP/ELF 保留作证据；后续使用独立 v11.1 修复包。新增 Hook 必须检查整个合并模块的 trampoline 池容量和严格边界；构建入口现已强制执行 `tools/check_hook_budget.py`。v11.1 真机验证未执行。

**2026-09-12 更新：用户反馈 UI Info Suite 2 B1 当前测试全部可用；继续完成其余单人功能，明确不做联机适配。**

- B1 已测试基线 SHA-256：`922844B6BA5A0AE7C969A57BF5354BE872ACFDFCCFC595888A64625DB1EC19A8`。
- 当前修复候选 v11.1：NSO SHA-256 `9ABE49AE7526BEBA703D7F54126F5EC8A3ECD588BC69F4D797BF26A49B2D5786`，Module ID `730AAC884C1050A82E1349A2872596F88DC7629F`；静态、构建、NSO 校验通过，真机未执行。包路径 `deploy/uiinfo-suite2-singleplayer-merged-v11-1.zip`。
- 失败 v11 的身份：NSO SHA-256 `8C064037D539C029037B49E863D6B20230333A11EE768A97FE86E43239B751DF`，Module ID `8ABDD21501F1FE60B760D66167684FB3CFCB8237`；其构建/NSO 校验成功不代表启动成功，已由日志推翻可用性。
- 原有功能源码保留，新增背包售价、机器悬停时间；完整功能账本见
  `docs/UIINFO_SUITE2_SWITCH.md`。B1 已获用户整体真机反馈；未实现功能及未逐项反馈的边界场景不能据此宣称通过。
- 本轮修改前的 B1 源码、NSO、ELF 及功能账本备份：`analysis/uiinfo-b1-tested-baseline-20260912/`。
- 产物校验以 `analysis/uiinfo-batch1-nso-audit.json` 和最终包 `BUILD_INFO.json` 为准。
- 旧源码、ELF、未损坏 v9、损坏版本及被静态复查拒绝的首轮 UI 候选已保留于
  `analysis/uiinfo-batch1-baseline-20260905/`；corrupt/rejected 文件不能安装。

**以下为 2026-09-05 发现的历史打标损坏证据；不得重新采用该二进制补丁方法。**

- 文件：`atmosphere/contents/0100E65002BB8000/exefs/subsdk9`
- SHA-256：`F2473DF7F60872D318D3F848B816B838A27A7D38D84CA54B3E528DC89E87DE6B`
- 2026-09-05 只读复验：text/data 正常，rodata 解压报 `invalid LZ4 match offset 0x0`。
- 未打标参考 `runtime/build-clang/subsdk9-normal` 的三段解压和哈希均通过，SHA-256 为
  `6BB58CEDDE04BE1517F74524E7F17B7C0CE594EC4EFB6382E6B131AC2CF6679D`。
- 两者大小、NSO 头、Module ID 相同，不能据此认为运行内容未受影响。详见问题 1。
- 2026-09-05 初始复盘时只整理经验、未覆盖部署；随后用户授权 UI 移植，才在
  保留旧证据后源码重建。修复的是本地候选产物，SD 卡和真机状态仍未确认。

**另有历史审查遗留风险**：运行前没有真正校验 Build ID；地图集合仍含启发式解析；
出料提交后失败缺少回滚；保存/联机主机门控未闭合。详见问题 23–26。
这些是源码风险，不等于已经发生过存档损坏或联机复制。

快速导航：

| 工作类型 | 先读 |
| --- | --- |
| 启动、构建、NSO、NPDM | 问题 1–5、20；Rule 1–3、10 |
| 机器输入/输出、箱子 | 问题 6–10、25；Rule 4–6 |
| 木头小径、地形、GC、性能 | 问题 11–13、19；Rule 4、7、9 |
| 电梯、戒指 UI、Hook | 问题 14–18、33；Rule 3、8；共享池预算 |
| UI Info Suite 2 移植、文本、重载、虚拟光标 | 问题 27–32；Rule 3/4/8/9；docs/UIINFO_SUITE2_SWITCH.md |
| 状态不明、旧日志、接续项目 | 问题 20–26；证据边界、Debug SOP、Checklist |

## 证据范围与结论边界

本次阅读了原开发任务 `01a00dbf-127e-7010-b986-bba8c1ea4208` 可返回的 87 个 turn
（包含空白、中断 turn 和本次请求的前一次中断，不是 87 次成功开发），以及当前源码、
构建脚本、配置、HANDOFF、analysis/docs、部署清单和本地 Git 状态。

证据分层：

- **现场复验**：本次实际读取文件/执行只读检查得到的事实。
- **历史用户反馈**：对话中明确报告成功、失败或崩溃。只覆盖反馈涉及的版本与场景。
- **历史分析记录**：此前助手在 HANDOFF、反汇编记录、对话中写下的定位；不是本次真机复测。
- **源码风险/推断**：当前存在的危险结构及其可能结果；没有硬件复现就不写成已发生事故。

本次未在项目目录和原指定 Temp 目录找到 `*0100e65002bb8000.log` 原始崩溃文件。
以下日志编号和寄存器来自历史对话及项目保存的摘录；无法从现有信息重新确认每份原始日志全文。
旧的反汇编文件可作为补充，但本轮没有重新逆向所有地址。

Git 边界：根目录不是仓库；`runtime/.git` 是 shallow repository，仅有可见提交
`f698816 Oops`（2025-05-30）。10 个框架文件有工作区修改，automate/game/hooks/elevator/rings
等核心新增目录为未跟踪文件。因此无法从现有本地 Git 信息确认每次修复的独立 commit、
作者归属和准确先后。公开仓库提交号来自历史对话，本轮未联网复验远端。
失败尝试的时间顺序主要依据对话和 HANDOFF 后续章节，不把上游 commit 当作项目修复提交。

“根本原因”中对工程决策的解释是基于这些技术事实的归纳，不推测当时的心理状态。
没有证据的失败尝试或成功结果统一写“无法从现有信息确认”。

## 项目真实入口与特有约束

目标游戏：Switch Stardew Valley `1.6.15.3`，Title ID `0100E65002BB8000`，
main Build ID `A5C617C14A7F3F6620B3BC8136965A4822D32B9C`。
本轮原始 `exefs/main` SHA-256 仍为
`15C950769FA6A71E659549163066DACB36E838C8FD9CFD5E595F84D3F6B05DD0`。
所有本文 main offset、字段和槽位都只属于此版本。

```text
tools/build_poc_clang.ps1
  -> runtime/source/**/*.cpp / .c / .s
  -> runtime/build-clang/*.o -> automate_lite.elf -> elf2nso -> subsdk9
  -> patch_npdm_syscalls.py(exefs/main.npdm) -> main.npdm overlay
  -> atmosphere/contents/0100E65002BB8000/exefs/ + variant deploy

runtime/source/program/main.cpp::exl_main
  -> exlaunch initialize -> install Game.Tick / elevator / rings hooks
GameTickHook::Callback at main+0x7F890
  -> Orig(game)
  -> AutomateManager::Update
  -> SkullCavernElevator::Update
AutomateManager
  -> current location / background root + instantiated interiors
  -> Object + Chest + terrain unwrap / Flooring + Building / FishPond
  -> same-tile + four-direction groups -> output lifecycle -> AttemptAutoLoad
```

| 路径 | 实际作用与限制 |
| --- | --- |
| `runtime/source/game/runtime.cpp/.hpp` | 共享的游戏访问层；修改会影响多个功能 |
| `runtime/source/game/offsets.hpp` | 真正参与构建的版本专用 offset 表 |
| `src/game/offsets.hpp` | 早期分析副本；当前构建脚本不编译根目录 src |
| `PCmod/Automate/Automate.dll` 等 | 当前 PC 参考 DLL 所在位置，用于 IL 语义对照，不是 Switch 运行入口 |
| `exefs/` | 原始游戏二进制；只读分析，不永久修改 main |
| `tools/`、`analysis/` | NSO/IL/反汇编工具与历史证据；XREF 是候选，不能代替函数边界证明 |
| `runtime/config.mk`、`misc/npdm-json/application.json` | 框架配置，不等于当前 PowerShell 构建所有实际参数 |
| `runtime/config.json` | 仍含旧模板 Title ID；当前 PowerShell 构建不读取它 |
| `atmosphere/` | 可变 staging；每次构建任何 variant 都会覆盖 |
| `deploy/`、`Switch_mod/`、压缩包 | 历史副本；目录名不是版本证明，各自重新算哈希 |

当前是原生 C++/ARM64 Brute AOT 访问路线，不加载 SMAPI DLL。不同地图分别构建库存网络；
“全图”指已加载根地图和已实例化室内的轮询，不主动加载未知地图，也不跨地图合并箱子库存。
鱼塘只出料、按建筑矩形占地参与连接；木头小径只是 Connector。
机器白名单现有 20 类普通 Object，另含 Fish Pond；Crab Pot 不在普通支持范围。

## 一、项目历史问题

### 问题 1：等长版权补丁破坏压缩 NSO，曾被误判为不影响使用（P0，现场确认，交付未修复）

**现象**  
当前三个已打标副本（staging、v9 deploy 根目录、build-clang/subsdk9）同为
`F2473DF7...`。完整检查到 rodata 时失败，而此前对话声称“大小、Module ID 和加载段不变”。
本轮未启动 Switch，实际主机报错形式无法从现有信息确认。

**直接原因**  
`tools/patch_binary_provenance.ps1` 在文件 offset 40649/40947
（`0x9EC9/0x9FF3`）替换并零填充 ASCII 字符串。两处均位于压缩 rodata 文件区间
`[0x9EA0,0xD458)`，不是独立于加载段的尾部元数据。LZ4 流被改变，段摘要也未重新生成。

**根本原因**  
把“字符串可见、替换等长、代码段没变”当成“压缩容器语义不变”，并把部分校验扩展成整体成功。

**错误尝试**  
直接扫描文件中的可见字符串做定点替换；只核对魔数、长度、Module ID 和 text。
这些指标本次全部能保持相同，却无法证明 rodata 可解压。

**最终解决方法**  
历史上未真正解决。已确认可用的完整性参考为 `runtime/build-clang/subsdk9-normal`
（未打标 v9），三段均通过。后续应从已有 `project_metadata.hpp` 和
`program/main.cpp` 的源码标识正常编译，经 ELF/elf2nso 重新生成 NSO；
不要再调用旧定点补丁。紧急回退可选该未打标参考，但它不代表 v9 新机器已真机验收。
本次未覆盖 staging，此问题保持开放。

**为什么这个方案有效**  
常规构建让字符串进入未压缩的链接输入，由打包器生成一致的压缩段、长度和哈希。
本轮在内存中对完好参考重放两项旧补丁，结果与损坏 staging 逐字节相同，因果链已复现。

**以后如何避免**  
Rule 1：任何 NSO 修改后逐段解压并核对所有启用的 SHA-256，检查退出码；
校验失败禁止标记“可部署”。Module ID 不代替完整文件哈希。

**证据**：历史 turn `01a03c56-981b-7661-b4e7-d2d9719e866e`、
`01a03c66-d0ee-7f13-b8ca-9351f118d930`；上述补丁脚本；
[本轮逐段对照 JSON](analysis/project_lessons_nso_audit_2026-09-05.json)。

### 问题 2：Clang 未发射模块名对象，初始化阶段越界崩溃（P0，历史闭环）

**现象**  
阶段 4 普通/proof 都在 `subsdk9+0x5CF8` Data Abort，尚未进入 exl_main 或 Tick 回调。

**直接原因**  
仅靠链接脚本 KEEP，无法阻止 Clang 提前丢弃 internal-linkage 的
`.nx-module-name` 常量；加载器将普通 rodata 当成模块名头。

**根本原因**  
忽略编译器对象发射与链接器 section 保留是两层机制，NSO 段哈希正确也不保证语义布局正确。

**错误尝试**  
初始 PoC 只证明编译/链接/段哈希；首次“进游戏报错”不足以证明 fatal-proof 命中。
之后确实由日志推翻了已进入回调的假设。

**最终解决方法**  
`runtime/source/lib/module.cpp` 给 `s_ModuleName` 加
`__attribute__((used, section(".nx-module-name")))`。
解压 rodata 前缀应为 `00000000 0d000000 6175746f6d6174655f6c697465`，
即长度 13 与 automate_lite。后续 `01787034526` 报告和“确实没有崩溃”反馈确认阶段 4 闭环。

**为什么这个方案有效**  
used 确保对象进入编译产物，KEEP 才有可保留的 section；初始化读到合法名称长度。

**以后如何避免**  
Rule 2：检查解压后的模块名结构及 MOD0/重定位/导入，不只看 NSO 魔数。
当前正常 NSO 的 header_module_name 字段也可能显示 `0x1:0x1 / S`，不能单独据此判坏；
必须检查实际 rodata 名称头。

**证据**：`docs/phase4_verification.md`“初始化崩溃修复”；`runtime` 中 module.cpp 的 Git diff。

### 问题 3：最初指导只部署 subsdk9，遗漏 NPDM 权限覆盖（P0，历史闭环）

**现象**  
模块名问题修复后，报告 `01786948496` 在 `automate_lite+0x7150`
触发 Undefined System Call，仍未执行 Game.Tick。

**直接原因**  
exlaunch 写映射调用 `svcMapProcessMemory (0x74)`，原游戏进程 syscall mask 未授权。

**根本原因**  
我最初明确指导“只复制 subsdk9，不复制 main.npdm”，没有把注入模块与进程权限作为一对交付。
不能将这次遗漏归责为用户不按步骤部署。

**错误尝试**  
仅重建/替换 subsdk9 不会改变进程 NPDM；模块加载成功也不代表有权安装 Hook。

**最终解决方法**  
`tools/patch_npdm_syscalls.py` 校验原始 NPDM SHA 后，在 ACID 和 ACI0
加入 `0x8600000F` descriptor，仅新增 SVC `0x74/0x75`，保留既有身份和权限。
构建脚本成对输出 `subsdk9 + main.npdm`；后来 proof 和 normal 均获成功反馈。

**为什么这个方案有效**  
权限属于被加载进程；限制集和实际 capability 集同时允许映射/解除映射，Hook 初始化才能继续。

**以后如何避免**  
Rule 2/10：按实际 SVC 和原 NPDM 构造最小 overlay，关闭游戏后成对覆盖、重新启动。
不复制宽权限模板 JSON 生成的任意 NPDM。

**证据**：历史 turn `01a00df6-ced4-76f3-b630-5b59c269f61f` 的初始错误指导；
`01a00e74-63e9-7ea1-a735-edff5eeaad3d`；`docs/phase4_verification.md`。

### 问题 4：Windows 工具链不完整，官方包安装失败，转用混合 Clang 构建（P1，历史构建闭环）

**现象**  
历史记录先误以为没有 devkitPro，随后发现已安装 devkitARM/MSYS2，缺少 Switch AArch64 组件；
switch-dev 安装出现 403，不能按原计划完成。

**直接原因**  
历史说明指出旧包数据库指向过期包，同步后下载仍被拒；完整 HTTP/编译 stderr
无法从现有信息确认，不把所有失败统一断定为同一个服务端原因。

**根本原因**  
未先区分“未安装”“已安装但不在 PATH”“缺目标组件”，且依赖标准安装路径能直接工作。
切换 Clang 后还需要处理 GCC 专用类型、属性与 SDK 导入差异。

**错误尝试**  
用旧 pacman 数据库安装 switch-dev，之后同步数据库仍未获得可用下载。
单靠重试没有产出 AArch64 工具链；不能据此要求每次任务重装系统环境。

**最终解决方法**  
`tools/build_poc_clang.ps1` 使用 Windows LLVM clang/lld、项目内 devkitA64
sysbase/elf2nso、devkitARM 的 C++ 13.1.0 头以及 `toolchains/compat/bits`。
当前 Git diff 中，`common.hpp` 以存储用途的 unsigned __int128 代替缺失 float128；
`neon.hpp`/`jit.hpp` 调整属性；`random.cpp` 使用 SplitMix64 finalizer；
`rtld/ModuleObject.cpp` 不再依赖该游戏 SDK 未导出的手动 lookup 指针。

**为什么这个方案有效**  
明确以 `--target=aarch64-none-elf` 编译，并限定各部分 ABI/用途，产生可用 ARM64 NSO。
这些兼容补丁不意味着 ARM32 头可普遍混用于任意 AArch64 工程；float128 别名也仅是寄存器存储。

**以后如何避免**  
Rule 2：先核实脚本的实际二进制、头文件、库和 target，再诊断编译；
迁移机器时重新验证兼容层，不盲改 API 业务代码来掩盖工具链问题。

**证据**：阶段 4 turn 的过程记录、上述脚本与 Git diff。
本轮 python 为 3.8.8，capstone/PyYAML 可导入；LLVM/C++ 头路径存在。
没有本轮重建，因此不能宣称现有全部源码再次编译通过。

### 问题 5：同名 cpp 的对象文件相互覆盖，功能模块未正确链接（P0，历史构建修复）

**现象**  
新增电梯管理器和同名 Hook 后，旧对象文件命名可能覆盖先前编译单元，
历史记录指出出现缺失/未解析的电梯符号。

**直接原因**  
旧脚本用 BaseName.o，`elevator/skull_cavern_elevator.cpp` 与
`hooks/skull_cavern_elevator.cpp` 生成相同路径。

**根本原因**  
用文件短名当作全项目唯一身份；共享 ELF 还可能容纳未解析符号，退出码不足以证明功能全部入包。

**错误尝试**  
保留 BaseName 规则继续构建不会消除碰撞；其他具体重试无法从现有信息确认。

**最终解决方法**  
`tools/build_poc_clang.ps1` 从 sourceRoot 求相对路径，转换分隔符后附加 `.o`；
构建前仅清理 build-clang 顶层旧 `*.o`，最终核验管理器和 Hook 符号共存。

**为什么这个方案有效**  
这两个源码的对象名不再相同，旧平铺文件不再混入链接。
该转义规则理论上仍可能与原本带下划线的路径碰撞，新增路径应检查对象名唯一性。

**以后如何避免**  
Rule 2：构建单元以相对路径标识；新增模块检查 ELF 符号及未解析导入，
不能只确认源码存在或“构建没有报错”。

**证据**：`HANDOFF.md` §13；构建脚本对象命名注释及当前对象文件。

### 问题 6：直接调用 Object 基类 body，绕过机器虚函数分派（P0，普通机器路径已替换；捕蟹笼未验收）

**现象**  
捕蟹笼成品进入玩家背包后在 `main+0x019A0BC8` 崩溃；后来压酪机/种子机等
四项验证也失败。物品先转移不等于收取成功。

**直接原因**  
把 `Object.checkForAction` 基类 body 当作所有派生对象的入口，跳过 override，
在清理时读取捕蟹笼不存在/为空的 `Object+0x218`。投料也曾直调基类 body。

**根本原因**  
找到了函数地址，却未还原具体实例的 method table、this-adjustment 与 invoker 拆包关系。

**错误尝试**  
最早改成 managed invoker 参数块后仍缺真机 ABI 证据；不能因“用了 invoker”自动宣布修复。
直接基类 body、只读 justChecking 猜测也没有建立普通机器完整行为。

**最终解决方法**  
历史虚分派修正记录确认 checkForAction 槽 `+0x6F0/+0x6F8`，
performObjectDropInAction 槽 `+0x5E0/+0x5E8`。
当前普通自动化进一步改为 `MachineActions::AttemptAutoLoad` 和显式输出生命周期，
复用 `TryResolveVirtualMethod`。危险玩家收取/trace 路径受双宏保护，捕蟹笼排除在白名单外。

**为什么这个方案有效**  
虚分派保留对象具体实现并传递正确 adjusted this。当前架构还避免把“玩家收取到背包”
当作“自动输出到连接箱子”的替代。

**以后如何避免**  
Rule 3/4：每个 AOT 入口记录调用类型与真实 caller。捕蟹笼的原失败功能未证明已修好，
以后添加它必须单独对照专用 wrapper 和 reset，不能借普通机器成功推断。

**证据**：`analysis/stage8_notes.md` 虚分派/捕蟹笼段；
`docs/phase8_verification.md`；`runtime/source/hooks/game_update.cpp`。

### 问题 7：把 Chest 返回接口猜成 List/NetList 布局（P0，箱子路径已改；泛化解析仍残留）

**现象**  
报告 `01787377188/01787382636` 涉及无效值 `0x62006e0078`；
也出现不崩溃却把非空箱子判断为空，导致进料候选为零。

**直接原因**  
`TryReadListOrNetField` 的猜测链将非指针字段当列表/NetField，
而 Chest.GetItemsForPlayer 实际返回接口对象。

**根本原因**  
从 PC 类型名或某一集合布局推导所有接口的内存结构；误把可读范围保护当成类型正确性证明。

**错误尝试**  
固定 List 布局、增加多布局/嵌套探测、Chest 字段 fallback、svcQueryMemory 防护先后出现。
范围检查能挡一部分非法访问，不能使错误布局返回正确 Count。

**最终解决方法**  
`runtime/source/game/runtime.cpp::ChestActions::GetItems` 调
`main+0x01960210` wrapper；经 Brute interface dispatcher
`main+0xE30` 解析 ICollection.Count / IList.get_Item。
保留实际 getter 返回的 inventory 交给 AttemptAutoLoad。

**为什么这个方案有效**  
让游戏接口负责当前具体容器的实现、索引和 this 调整，不再靠数组外观猜类型。
该 wrapper 会获取当前玩家 ID，不能擅自把占位参数 0 当普通玩家 ID 修掉。

**以后如何避免**  
Rule 4：先画出返回值到公开对象的全部间接层。该修复只覆盖箱子，
不能声称通用启发式解析已全部移除，参见问题 23。

**证据**：`docs/phase8_verification.md`；上述函数及 TryGetICollectionCount/TryGetIListItem。

### 问题 8：用逐物品投料和手工煤炭结算模拟 Automate（P0，历史反馈支持修复）

**现象**  
多轮反馈“熔炉/重型熔炉可出料但自动进料、链式进料失败”，压酪机 Milk x2 测试也失败。

**直接原因**  
先调用内部 PlaceInMachine，后来改为逐物品 probe/commit performObjectDropInAction，
外部又克隆 Coal 到 Farmer、猜数量、补写 Chest stack，语义并非 PC Automate 的库存级自动加载。

**根本原因**  
在确认参考 DLL 的真实 SetInput 实现前，按玩家交互的表象重写库存事务。

**错误尝试**  
5+1/25+3 等数量修正、同指针改克隆、按 stack 差额结算、添加高级投料函数。
部分修补能消除独立缺陷，却仍绕过 inventory-level 路径；克隆燃料合堆后的撤销还有复制风险。
风险存在的审查记录不等于已发生物品复制。

**最终解决方法**  
解析 PC Automate 2.6.1 `DataBasedObjectMachine.SetInput(IStorage)` IL，
在 `MachineActions::AttemptAutoLoad` 使用虚槽 `+0x810/+0x818`，
以 machine、Chest IInventory、Farmer 调用；成功后停止尝试。
当前管理器不再手工扣 stack 或给玩家暂存煤。

**为什么这个方案有效**  
原游戏负责配方、燃料、数量和扣料，保存机器自身行为与该版本数据规则。
后来用户反馈“室外农场以上功能全部成功”；不能扩大为所有物品、所有存档的证明。

**以后如何避免**  
Rule 5：新增普通数据驱动机器先检查现有白名单/SupportsInput，复用库存级原函数；
示例配方只用于验收，不成为 Mod 的第二套配方系统。

**证据**：`HANDOFF.md` §4.1；`analysis/automate_databased_lifecycle_2026-08-23.md`；
历史 turn `01a02eb6-c371-7cb1-857c-9df6bf1adbd5`。

### 问题 9：只清 heldObject/ready，漏掉机器完整输出生命周期（P0，历史反馈支持修复）

**现象**  
种子制造机无法完成连续输出，压酪机等收取不正常；单一成品移动成功不能保证继续生产。

**直接原因**  
旧 GenericReset 只清部分字段，遗漏 OutputCollected 触发的下一产物、贴图复位、Tapper 续产；
timer-ready fallback/justChecking 猜测也不等价于参考 GetState。

**根本原因**  
把复杂状态机缩成“搬走物品并清零”，没有追踪收取回调、后继输出和物品所有权。

**错误尝试**  
给 Seed Maker 加 timer fallback、改就绪探测、只清两字段；它们没有补齐生命周期。
盲目把所有机器设成可收取还可能提前取走未完成产物。

**最终解决方法**  
`AutomateManager::CollectReadyOutputsToChests` 在 Chest.addItem 前以 Item.getOne
保存样本，箱子完全接受后调用 `ObjectView::CompleteOutputTransfer`：
读取 MachineData，清 held/ready/showNextIndex，经原函数 ResetParentSheetIndex，
TryGetMachineOutputRule(trigger=2) 与虚调用 OutputMachine；Tapper 更新 Tree product。
ready 仅认 heldObject 非空且 readyForHarvest 为真。

**为什么这个方案有效**  
复原“收取完成后接下来发生什么”，稳定样本避免箱子合堆变异原对象影响触发规则。
当前实现仍有提交后失败风险，不能把完整正常路径称为原子事务，见问题 25。

**以后如何避免**  
Rule 6：验证至少两轮、连续多产物、满箱和恢复后续产；明确样本、原堆、剩余堆与所有权。

**证据**：`HANDOFF.md` §4.2、§5；上述管理器及 CompleteOutputTransfer。

### 问题 10：室外成功、Shed 全失效，真正问题在位置枚举（P0，修复已实现，验收边界保留）

**现象**  
用户明确报告相同机器组合在室外农场全部成功、室内小屋全部失败。

**直接原因**  
旧版只枚举 Game1.locations；仅根列表不可读才 fallback currentLocation。
建筑实例的 Shed 室内通常不在根列表，未进入后续识别与连通图。

**根本原因**  
误认为“所有根地图”即“所有活动位置”，没有核对对象归属层次。

**错误尝试**  
沿用根列表成功时不扫 currentLocation 的逻辑；没有证据表明重新改各机器会有效。
其他针对室内的错误方案无法从现有信息确认。

**最终解决方法**  
GameState 加 `GetInstancedBuildingInteriors`，调用 `main+0x0149FE40`；
管理器无条件覆盖 currentLocation，加入 roots/实例化 interiors，并去重。
v8 后改为保留同等范围的分帧轮询，不再同一帧扫描全部地图。

**为什么这个方案有效**  
失踪的对象先进入扫描入口，原来已能工作的机器链才有机会执行。
v8 的整体成功反馈不能替代逐个建筑、人物离场后的单项验收记录。

**以后如何避免**  
Rule 7：场景差异先比较 location 来源、数量和当前/后台覆盖，再改共用机器逻辑。
遍历返回集合的安全性另见问题 23。

**证据**：`analysis/location_interiors_2026-08-23.md`；`HANDOFF.md` §4.3、§26。

### 问题 11：木头小径 v1–v5 多次失败，共同上游对象未解包（P0，v8 综合真机反馈闭环）

**现象**  
鱼塘正常但小径不能连通；改识别方式、metadata、遍历方式后仍失败。
用户确认 Layout 2 已变化，排除了“整个新模块根本没有加载”的解释。

**直接原因**  
`GameLocation.terrainFeatures+0x140` 的 owner 包装器在 `+0x48` 保存 concrete dictionary，
values 是网络字段 raw wrapper，不能直接当 TerrainFeature。少了 owner 的 unwrap 虚调用。

**根本原因**  
只追到了 TryGetValue/数组读取，没有沿原版 caller 继续读到解包和 isinst；
用不同下游算法处理同一个错误输入，造成连续多版无效修改。

**错误尝试**

| 阶段 | 当时改动 | 为什么不是完整修复 |
| --- | --- | --- |
| 首版 | whichFloor 硬编码为旧数据键 "0" | whichFloor 是数据键，且输入本身还是 wrapper |
| GetData/类型候选 | 函数地址指纹，再改 metadata factory + isinst | 尚未得到真正 TerrainFeature；thunk/metadata 问题也缺运行时独立证明 |
| layout2 v2 | 走 placement map 比较 whichFloor | raw wrapper 的 +0x40 仍不是 Flooring.whichFloor |
| v3 | 放弃预枚举，改 TryGetValue tile flood，加入同格实体 | 查询得到的仍是 raw wrapper；改变遍历不能修正类型 |
| v4 | 游戏根槽 metadata + GetData，删除裸指针缓存 | 修掉独立生命周期风险，没有解包 |
| v5 | 双识别 + 恢复正式 Connector 索引 | 两条识别分支共享同一个错误上游，仍失败 |
| v6 | 统一 unwrap | 正确到达分类，但旧 Int32.ToString 错误调用随即崩溃，见问题 13 |
| v7/v8 | unwrap + GetData.ItemId；随后优化性能 | 用户随后确认 v8 成功；没有给出每个边界用例的逐项报告 |

**最终解决方法**  
原版 `main+0x019379D4..0x01937A30` 展示：
TryGetValue → owner method table `+0x210`、adjust `+0x218`
→ `X0=adjusted owner, X1=raw field` → 返回真正 feature → isinst。
`runtime.cpp::UnwrapTerrainFeatureField` 统一供全量 Get、单格 GetTerrainFeatureAt、
Tapper Tree refresh 使用；CollectionView 热路径保存本次调用内解析结果。
分类最终回到 `is Flooring -> GetData -> ItemId == 405/(O)405`。

**为什么这个方案有效**  
修复三条消费者的共同数据来源后，原来的 PC 语义识别链终于拿到正确对象。
GetData/isinst 本身不能据前几版失败被认定为错误方案；失败前提是没有解包。

**以后如何避免**  
Rule 4：同症状多次失败，回到第一个共享输入并增加分层计数。
只有确认 connectorNodes 大于零后，才把主要怀疑转向 flood。

**证据**：`HANDOFF.md` §18–27；历史 turn
`01a03354-f4e1-7231-8601-c5d6b4e1d546`；
用户成功反馈 `01a03386-a34a-7cf2-9e0d-682d997181de`。

### 问题 12：把托管对象裸指针缓存在原生 BSS（P1，缺陷风险修正；独立运行时因果未证明）

**现象**  
小径 v3 持续失败；审查发现 placement-map 返回的托管字符串指针跨 Tick 缓存。

**直接原因**  
原生缓存不参与游戏托管引用更新；若对象被移动，游戏持有强引用能保活，却不能修正 Mod 的旧地址。
本次未找到 GC 移动事件与具体失败同时发生的 trace，不能将其定为那几次失败的唯一原因。

**根本原因**  
混淆“对象还活着”和“对象地址永远不变”，未说明缓存 rooting/pinning 依据。

**错误尝试**  
只去除指针缓存的 v4 仍失败；v5 缓存 ASCII 内容也未解决 wrapper 根因。
历史曾过早把缓存风险称作确定的实际运行根因，本复盘降为有依据的风险与独立缺陷。

**最终解决方法**  
当前 Flooring 每次从游戏根存储读取 metadata；使用 GetData 返回值做当前调用内比较。
v8 后台只保留 root/interior 索引和游标，不跨 Tick 保存 GameLocation 指针。
这不代表全代码所有托管/metadata 缓存都已审计：例如接口 metadata 的全局缓存仍需证明寿命。

**为什么这个方案有效**  
根存储由游戏维护，重取引用或保存原生字节避免复用失效对象地址。

**以后如何避免**  
Rule 9：缓存先写清所有者、有效期、更新条件；代码地址、数值和托管对象分别论证。
“仅当前 Tick”也不是穿越所有分配/GC 安全点的自动证明。

**证据**：`HANDOFF.md` §22–26；`FlooringView::IsFlooring`、
`GetInterfaceMetadata`、后台游标代码。

### 问题 13：把 Int32.ToString 的实例指针传成整数 405（P0，v7 删除，v8 真机成功）

**现象**  
v6 日志 `01787568229`：PC=`main+0x002025E0`，
LR=`FlooringView::IsWoodPath+0x310`，X0=`0x195`，Data Abort。

**直接原因**  
该 AOT body 是实例方法，X0 应指向 Int32 值的存储地址；传数值 405 等于要求解引用低地址。

**根本原因**  
根据方法名猜成静态 ToString(int)，没有核实原 caller 的参数装载和 callee 第一条解引用。

**错误尝试**  
为绕过未证实的类型门控增加 placement-map/ToString 分支；
v6 解决前置 wrapper 后，这条潜伏的 ABI 错误才真正可达。

**最终解决方法**  
v7 从 `FlooringView::IsWoodPath` 删除整个 placement-map/ToString 路径，
保留正确解包和 GetData.ItemId 的 UTF-16 比较。当前此函数不再调用该入口。

**为什么这个方案有效**  
不再生成不必要的托管数字字符串，也不再传错 this。
日志表明程序进入了新的下游位置，应当区分新暴露缺陷与“原修复完全没生效”。

**以后如何避免**  
Rule 3：每次新增 native 调用，记录每个寄存器应放值还是地址，以及 value-type this。
这条规则同样约束电梯等模块中使用同名 API 的其他调用，不推定它们同样出错。

**证据**：`HANDOFF.md` §25；`runtime.cpp::FlooringView::IsWoodPath`。

### 问题 14：把找不到即抛异常的 TileSheet getter 当 null 查询（P0，入口路径已修正）

**现象**  
首版沙漠电梯在 `01787540850` 报告中沿
Update → PlaceElevatorTile → Map.GetTileSheet("mine") → managed exception → nnSdk Abort。

**直接原因**  
SkullCave 入口没有 mine TileSheet；`main+0x012EE1D0` 失败时抛异常，不返回 null。

**根本原因**  
根据 getter 名称猜失败语义，并把随机矿层已有资源的假设套到入口地图。

**错误尝试**  
在入口直接查询 mine。后来的“只写 Action、不显示图标”解除崩溃路径，但未满足显示图标需求，
属于临时降级，不是完整最终功能。

**最终解决方法**  
`runtime/source/elevator/skull_cavern_elevator.cpp` 使用非抛异常查找
`main+0x01DA1F10`；缺表时以 `main+0x01DACEC0` 创建引用
`Maps\Mines\mine` 的 16×18 TileSheet，经 `main+0x01DA1FC0` 加入地图，
放置 Front/Front/Buildings 三格 80/96/112。
后续对话能继续讨论菜单，表明不再停在首次入口崩溃；每个图层/存档场景的验收无法完整确认。

**为什么这个方案有效**  
先判资源存在性，再补齐入口实际缺的依赖，避免把托管异常传播到原生 Hook。

**以后如何避免**  
Rule 3/8：验证未命中、空地图、资源缺失分支；查询函数的返回策略必须来自 callee/caller。

**证据**：`HANDOFF.md` §13.1–13.2；
`analysis/skull_elevator_scroll_icon_2026-08-24.md`。

### 问题 15：把原生电梯 120 按钮窗口当成深层楼层上限（P1，功能实现与边界分开）

**现象**  
用户反馈电梯最多显示 120，之后不可继续选择。旧实现相对 120 对应实际 240。

**直接原因**  
直接复用原版 MineElevatorMenu 的 Math.Min(...,120)，没有提供窗口滚动和目标映射。

**根本原因**  
复用 UI 时只覆盖常见范围，混淆显示相对层数、实际矿层、可达检查点和窗口容量。

**错误尝试**  
原始固定菜单只能访问首个窗口；简单无限增按钮并无实际尝试证据，不写成历史失败。

**最终解决方法**  
保持原按钮窗口，滚轮/L-R/触摸只更新按钮 name；正数目标 enterMine 前加 120，
0 改为返回 SkullCave；选择上限依最低到达层数。
通过 method-table 指针替换短默认输入虚函数，保留普通矿井原行为。
Game1.enterMine 用 `0x013D6C00`，不能混用会解引用 X0 的 MineShaft 实例地址 `0x015F14C0`。

**为什么这个方案有效**  
窗口与范围解耦，复用原生绘制、点击和焦点，同时不创建超屏控件。
历史有综合成功记录，但没有所有深层跳转/普通矿井回归的独立结果。

**以后如何避免**  
Rule 8：UI 范围测试覆盖相对 0/120/125、实际 121/240/245、未到达层和普通矿井；
移植前确认两套坐标/编号语义。

**证据**：`HANDOFF.md` §13、§13.2；历史 turn `01a031d3-c026-7652-bb63-f3b8a57d9c17`。

### 问题 16：InventoryPage.draw Hook 装在函数序言中间（P0，历史后续日志证明修复生效）

**现象**  
四戒指首版报告 `01787549621` 为 Instruction Abort，PC=0、LR=0。

**直接原因**  
旧 offset `0x017074AC` 是序言内 sub sp；真正入口是 `0x01707480`，
前面还有寄存器/LR 保存。trampoline 栈与原版恢复路径错配。

**根本原因**  
用附近指令或片段起点当函数入口，没有完整检查 prologue/epilogue。

**错误尝试**  
在 `0x017074AC` 安装 Hook。构造/点击/悬停实际入口没有同类错误，不应一起乱改。

**最终解决方法**  
`runtime/source/game/offsets.hpp::InventoryPageDraw` 改为 `0x01707480`，
检查最终 ELF 安装常量。下一份报告已加载新 Module ID 且可以回溯到 Draw，
证明序言修复生效，后续是另一处 NetRef 错误。

**为什么这个方案有效**  
完整 trampoline 包含原版保存/恢复约定，回调和原函数各自平衡 SP/LR。

**以后如何避免**  
Rule 3：新 Hook 先读前后完整函数边界；非常短的函数不能未经长度分析直接覆盖 trampoline。

**证据**：`HANDOFF.md` §15–16；`runtime/source/hooks/wear_more_rings.cpp`。

### 问题 17：Farmer 装备字段是 NetRef 指针，却当作内嵌 NetRef（P0，历史显示/布局反馈闭环）

**现象**  
第二份戒指报告 `01787550350`：
WearMoreRings::Draw → Item.drawInMenu wrapper → interface dispatch → Abort。

**直接原因**  
误从 Farmer 字段地址加 0x58 读戒指，把不相关字段当 Item；点击时也会传错 Equip 目标。

**根本原因**  
少读了原版 `ADD field` 后的 `LDR [field]`，把字段地址与字段值混为一谈。

**错误尝试**  
修正 draw Hook 入口只解决栈问题，不能解决对象来源；没有理由因 Draw 崩溃修改已验证的 CombinedRing 链。

**最终解决方法**  
`wear_more_rings.cpp::GetFarmerEquipmentRef` 在读/绘制/左右栏点击统一：
`ref = *(Farmer+0x478/0x480)`，`item = *(ref+0x58)`，
`Farmer.Equip(farmer,replacement,ref)`。
后续用户提供可显示但重叠的界面，证明已越过这两处启动/绘制崩溃。

**为什么这个方案有效**  
保留原对象引用和 Equip 的 onUnequip/NetRef/onEquip 路径，不直接写角色加成。

**以后如何避免**  
Rule 4/8：指针链逐层标注类型；修一条读取链时搜索所有消费者，保持读写/绘制/点击一致。

**证据**：`HANDOFF.md` §16–17；`docs/WEAR_MORE_RINGS_SWITCH.md`。

### 问题 18：戒指布局按臆测锚点移动，覆盖原装备控件（P1，Layout 2 用户明确成功）

**现象**  
先与衣服裤子重叠；改 2×2 后用户要求图 2；首次分栏又覆盖 103/鞋子。

**直接原因**  
把 ID 103 当作最右侧戒指锚点，实际 102/103 原本都在左列；未解锁 trinket 时也不能假定组件存在。

**根本原因**  
依据名称/图像直觉排坐标，未从 InventoryPage 构造函数还原控件 ID、原矩形和导航关系。

**错误尝试**  
固定横移、2×2 暂时布局、直接以“左右原戒指”作双列锚点。
其中 2×2 是中间产品选择，不能全归为技术失败；真正错误是占用原装备区域和锚点语义错误。

**最终解决方法**  
`WearMoreRings::AttachInventoryPage` 按 FindComponentById 定位：
左列 102、新200、鞋104；中列帽101/衣108/裤109；
右列移动103、新201、下移 trinket120。120 未解锁时使用构造证实的 X 偏移。
绘制、点击、悬停、手柄邻接使用同一组重排控件。

**为什么这个方案有效**  
按稳定语义 ID 调整实际控件，命中区域和视觉坐标一起变化。
用户在 `01a03312-4204-7891-9181-e8139c0fe671` 明确确认布局成功。

**以后如何避免**  
Rule 8：画控件 ID/矩形/邻居表，再改布局；验证 locked/unlocked trinket 和手柄导航，
截图只证明视觉，不自动证明装备效果和保存/卸载兼容。

**证据**：`HANDOFF.md` §17、§19–21；`wear_more_rings.cpp` 的 FindComponentById/AttachInventoryPage。

### 问题 19：功能可运行后周期性卡顿，全地图工作集中同一帧（P0，v8 用户明确成功）

**现象**  
v7 不再立即崩溃后，用户报告“每隔一段时间会卡一下”。

**直接原因**  
每 30 Tick 在主线程扫描所有根地图/室内、对象/建筑/地形并重建索引；
每个 feature 的 unwrap 解析重复触发多次 svcQueryMemory；
QueueTileUnique 用 std::find，长网络去重可退化 O(N²)。
电梯同相扫描、300 Tick 长 heartbeat 进一步叠加峰值。

**根本原因**  
把平均扫描频率低等同于单帧成本低；修复 wrapper 后扩大了实际执行工作量，未及时重估热路径。

**错误尝试**  
每 30 Tick 一次全量扫描仍会尖峰；没有证据表明仅拉大周期或删除跨图功能曾被实际采用。

**最终解决方法**  
v8：当前地图每30 Tick；非当前地图每4 Tick 的可用后台相位轮询一张；
unwrap 每个 CollectionView 解析一次；tile 去重用开放寻址哈希与 epoch；
电梯错开15 Tick；heartbeat 每约1800 Tick 且避开扫描相位。
后台仅缓存游标，保留同一套图连接语义。

**为什么这个方案有效**  
分摊主线程峰值并减少重复解析和队列查询。
只证明用户体验改善；原始帧时间统计、毫秒收益、硬件 profiler 数据无法从现有信息确认。
单个特别大的当前地图仍可能耗时，不应声称任意规模都没有卡顿。

**以后如何避免**  
Rule 9：性能验证看每帧最坏路径、系统调用次数和规模复杂度；
跨图覆盖与同一帧全图处理分开设计，优化后验证公平轮询和功能保留。

**证据**：`HANDOFF.md` §26–27；`automate_manager.cpp`；
用户 turn `01a03386-a34a-7cf2-9e0d-682d997181de`。

### 问题 20：旧日志、其他进程日志和 proof 异常混在一起（P1，历史排查流程已建立）

**现象**  
收到 controller/ovlloader 系统进程报告；普通版崩溃被询问是否“预期报错”；
捕蟹笼旧日志早于修复构建，却再次用于判断新版本。

**直接原因**  
未先绑定 Title ID、Module ID、构建/日志时间与对应 ELF；所有 variant 还会覆盖相同 staging。
当前 logger 只有 SvcLogger，不会自动在 SD 卡生成普通文本日志。

**根本原因**  
没有把“实际运行哪一份文件、日志采集通道是什么”作为 Debug 的第一步。

**错误尝试**  
用非游戏日志证明 proof；仅凭“退出游戏”判回调命中；
用 `01787055703` 老记录评判 20:31 后新产物；误期待普通文件日志。

**最终解决方法**  
先过滤 `0100E65002BB8000`，确认 Mod Module ID，
用相同构建 ELF 符号化 PC/LR。只有栈落在预期 AutomateManager proof 调用且原 Tick 已返回，
才证明回调成功。无普通日志时使用已限定的 audit 位图/proof，之后恢复 normal。

**为什么这个方案有效**  
每份反馈绑定唯一可比产物，才能区分未覆盖、旧崩溃、修复生效后暴露的新问题。
本轮问题 1 还证明 Module ID 可在打标后不变，因此必须同时存完整 SHA。

**以后如何避免**  
Rule 10：先确认产物与日志归属，再读崩溃点；日志不存在就明确采集方式，不虚构日志目录。

**证据**：`docs/phase4_verification.md`、
`analysis/pc_automate_logic.md`、`program/loggers.hpp`；
历史 turn `01a00e6f-7a2e-7020-8118-34855ccc182c`。

### 问题 21：候选实现被写成完成，历史“当前状态”互相覆盖（P1，复盘已纠正，旧文档仍有残留）

**现象**  
早期把捕蟹笼“物品入包后崩溃”写成已完成；小径多版先称完成，用户随后继续报告失败。
HANDOFF 前段说无 connector/18 台/全图30 Tick，后段已是小径/v8轮询/v9两台新增。
README“不会直接写 heldObject”的措辞也不精确：当前收取生命周期确实经 NetField setter 清理状态。

**直接原因**  
编译、静态逆向、正常路径实现、真机验收四种状态未分开；
追加记录未废止旧“当前”标题，生成包被定点修改后 BUILD_INFO 也没更新。

**根本原因**  
结果陈述超出证据；把文档当完成记录而不是待验证状态账本。

**错误尝试**  
以符号存在/哈希匹配推导功能成功，以部分功能成功推导全模块成功；
把 v4 的 GC 猜测写成已证实唯一根因。

**最终解决方法**  
已有 docs 曾更正阶段8；本手册按版本与证据重新分层，并在首页列出当前损坏产物与未闭环问题。
旧文档尚未全部重写，后续读取必须结合当前源码、最近反馈和现场校验，不仅按日期选一篇。

**为什么这个方案有效**  
历史尝试保留为证据，当前判断另外注明来源和范围，避免接续时再次启用失败方案。

**以后如何避免**  
Rule 11：报告四层验证状态；不是简单禁止清 heldObject，而是禁止用任意字段写入
替代经证实的生命周期。新发现推翻旧结论时要显式更正。

**证据**：`docs/phase8_verification.md`“重要更正”；
`HANDOFF.md` §3/§18/§26/§27；README 与 CompleteOutputTransfer 对照。

### 问题 22：工作区、Git、运行配置、公开包不是同一份状态（P1，现场确认的维护缺口）

**现象**  
根目录 git status 失败；runtime Git diff 看不到未跟踪核心模块；
根 src 有另一份 offsets，runtime/config.json 仍是旧 Title ID；
多个部署目录/压缩包和文档中的旧路径并存。
历史上传时 Git push 连接被重置，后来改经 GitHub API 上传。

**直接原因**  
这里只在 runtime 初始化过框架的浅克隆；外部工具、根文档与发布包不受该 Git diff 覆盖。
当前 PowerShell 脚本自定 flags、路径和 NPDM patcher，不读取那份旧 config.json。

**根本原因**  
将工作区目录等同于一个完整源码仓库，或把模板配置等同于实际构建输入。
目前没有证据证明曾因改错 src/config.json 导致具体功能失效，不能编造这样的事故。

**错误尝试**  
历史上传普通 Git 推送失败后重试并未成功；最终走 API。
仅依靠 runtime 的 diff/history 进行本次复盘也会漏掉大部分业务代码。

**最终解决方法**  
历史发布经 API 上传后按远端/本地字节哈希核对；本轮只引用其历史结果，未重查远端。
当前检查以构建脚本 sourceRoot/参数为准，Git 使用 `git -C runtime`，
同时列出 untracked 和根目录文件。完整版本管理尚未补齐，本次没有初始化仓库或提交。

**为什么这个方案有效**  
把“源码输入、工作区版本、打包副本、远端结果”分开核验，避免静默漏审。

**以后如何避免**  
Rule 10/11：修改前记录真实入口与 Git 边界；发布按确切目录和哈希核对，
不要自动上传 exefs/main、PC DLL、toolchains 等不在用户交付范围的材料。

**证据**：本轮 git status/log/rev-parse；build_poc_clang.ps1；
历史上传 turn `01a033ad-8d19-7061-a672-43094e13ca13`。

### 问题 23：室内/root 集合仍先走启发式 List 探测（P0，历史审查发现，未证实修复）

**现象**  
此前审查指出空 GameLocation[] 可能被误识别为 List，产生假 location 指针。
本轮看到 GetInstancedBuildingInteriors 仍先 TryReadListOrNetField，失败才 TryReadArray。
实际由此造成崩溃的原始日志无法从现有信息确认。

**直接原因**  
通用函数尝试多种布局与嵌套指针；可读内存不等于正确集合/元素类型。

**根本原因**  
问题7在 Chest 路径修好，但同类启发式思路留在地图消费者；没有按数据源分别建立精确布局契约。

**错误尝试**  
把 Chest 接口读取修复描述成“所有危险布局探测都已删除”会漏掉这里。
没有该处专门修复失败的执行证据。

**最终解决方法**  
未解决。候选方向：从实际数组/非空集合 caller 证实返回形状，
选择精确读取或原集合接口，验证空数组、无建筑、多室内和销毁/切图。
不能盲调换分支顺序就称修好，必须先验证判别方式。

**为什么这个方案有效**  
明确容器契约才能消除“任意可读区域被当列表”的歧义；当前只是修复方向，非已验证实现。

**以后如何避免**  
Rule 4：同类修复检查其他调用方；历史 v8 综合成功不消除未覆盖的空集合边界风险。

**证据**：历史审查 turn `01a02ed4-2646-7140-bac1-6a3091208662`；
`runtime.cpp::GetLoadedLocations/GetInstancedBuildingInteriors/TryReadNestedList`。

### 问题 24：Build ID 只写在常量/日志里，Hook 前没有执行匹配门控（P0，源码风险，未解决）

**现象**  
历史审查已提出；本轮 exl_main 仍直接安装 hooks。
没有游戏更新后真机崩溃证据，不能记为已经发生的版本事故。

**直接原因**  
offsets.hpp 保存 BuildId，日志打印 Project::kBuildId，但没有读取实际游戏身份并比较。

**根本原因**  
把文档中的版本约束当成了代码会自动拒绝不匹配版本的机制。

**错误尝试**  
仅打印预期 Build ID 不能证明加载的是该版本；其他修复尝试无法从现有信息确认。

**最终解决方法**  
未解决。应在任何 Hook 安装前验证真实 Build ID 或足够明确的指令签名，
不匹配时不安装；如何读取目标身份须按现有 runtime 证实，不能虚构 API。
当前先靠人工核验原始 main SHA/ID 控制交付版本。

**为什么这个方案有效**  
验证必须发生在首次按固定地址写入之前，日志常量没有这种保护能力。

**以后如何避免**  
Rule 3/10：区分“目标版本声明”和“运行时门控已实现”，后续支持新版本重新建立 offset/ABI 证据。

**证据**：上述历史审查；`runtime/source/program/main.cpp` 与 `game/offsets.hpp`。

### 问题 25：Chest 接收后生命周期失败仍无完整恢复（P0，源码风险，未解决）

**现象**  
此前审查发现潜在重复/丢物；当前管理器在 Chest.addItem 成功后，
CompleteOutputTransfer 若失败只打印 ERROR 并 return。
实际发生物品复制、丢失或坏档无法从现有信息确认。

**直接原因**  
箱子接收与机器多字段复位/后继输出是多个有副作用的步骤；
后面仍有 SetNetFieldValue/虚槽解析失败返回，没有显式完整回滚或持久故障隔离。

**根本原因**  
完成正常路径和“箱满不清空”后，未把提交后失败状态也当作事务的一部分。

**错误尝试**  
旧 Coal 克隆撤销、差额扣 stack 存在独立事务风险，已被 AttemptAutoLoad 替换；
但这不能解决出料事务。仅增加一条 ERROR 不会撤销已接受的物品。

**最终解决方法**  
未解决。后续先厘清 Chest.addItem 部分接收时返回剩余对象的身份/所有权，
以及每步已提交状态；尽可能前置无副作用的可调用性校验，
按原游戏事务设计恢复或隔离，针对失败阶段验证，不直接猜回滚字段。

**为什么这个方案有效**  
把“未接收、部分接收、完全接收未复位、复位中、完成”分别建模，
才能避免下一次 Tick 重复处理部分提交结果。尚未有对应实现/测试。

**以后如何避免**  
Rule 6：正常满箱、部分堆栈接收与提交后失败分别验证；不要将 native API 调用组合称为天然原子。

**证据**：历史审查 turn `01a02926-972d-7690-b043-0b4907f665c8`、
`01a02ed4-2646-7140-bac1-6a3091208662`；当前管理器与 CompleteOutputTransfer。

### 问题 26：保存/过夜/联机所有权尚未建立可靠状态门控（P0，源码风险，未解决）

**现象**  
历史要求保存/读档/切图验证，但没有完整逐项结果；
本轮 Update 仍按计数调度，未见实际主机/保存状态门控。
player 空只在后续限制输入，输出仍可能被处理。

**直接原因**  
已加载地图指针可得与“当前允许修改世界”不是同一条件；
鱼塘经验给 Game1.player，精确建筑 owner 映射仍未证明。

**根本原因**  
把单机日常正常路径扩大为保存边界与联机兼容；接入原 NetRef 不等于已经验证所有权。

**错误尝试**  
仅依赖 player/currentLocation 非空不足以保证安全修改；
没有证据证明已经尝试并验证过某套 saving/host API。

**最终解决方法**  
未解决。先由目标 AOT 确认世界就绪、保存/切图及主机权限来源，
再建立最小门控和单机/主客机状态转换用例。不能虚构 host offset 或按名称硬调用。

**为什么这个方案有效**  
将“何时可修改、谁有权修改”显式化，防止逻辑在状态重建/所有权不明时运行。

**以后如何避免**  
Rule 6/7：区分功能路径成功、存档边界成功和多人成功；未完成的验收保留未执行状态。

**证据**：`HANDOFF.md` §11、§18；历史审查；`AutomateManager::Update`、
`FishPondView::CompleteOutputTransfer`。

### 问题 27：UI AOT 同名方法、重载和字符串生命周期不能只靠名称判断

**现象**\
2026-09-06 首轮 UI 候选编译通过，但 post-link 与 callee 复查发现
`DrawTextWithShadow` 指向接受 StringBuilder 的 `0x1AECD00`，而新增代码传入 String。
这是已确认的调用类型错误，**没有真机运行或崩溃证据**。首轮 SHA `292E44CF...`
已标 rejected，不作交付。查询物品接口时还发现不同类型都有 salePrice 等同名方法。

**直接原因**\
同名的两个 Utility 重载具有相似寄存器布局，但对象语义不同。`0x1AECD00`
经 `0xC1A60/0xC7600/0x39F8E0` 读取 StringBuilder `+0x18` 内部 String；传入 String
会把字符内容当作对象指针。仅检查两个参数都是 `void*` 或长度都在 `+0x10` 无法发现问题。

**根本原因**\
候选地址记录先于对完整 callee 对象消费方式的核实；“找到同名注册与参数寄存器”
被过早当作类型证明。C++ 函数指针使用 opaque pointer，编译器无法替我们检查托管类型。

**错误尝试**\
第一轮使用 `0x1AECD00` 的构建已通过，但静态复查否决。另一个只读候选来自
HouseRenovation 的同名售价注册（`0x72F3B10`），其槽 `0x130/0x140` 不能用于 Item；
这个候选在写入调用代码之前排除。没有采用“上机多试几个偏移”的方案。

**最终解决方法**\
- `runtime/source/uiinfo/offsets.hpp::DrawTextWithShadow` 改为 String 重载 `0x1AED050`。
  invoker `0x75A0BA8..0x75A0D4C`，GOT `0xE193FE8`；callee 使用 `0xC0E20` 的 String 绘制链。
- Item 使用 `0x730D9A8/0x730DAC4/0x730EB6C` 证明的售价/Stack 槽
  `0x1A0/0x1B0/0x390`；Object 类型判断来自 `0xDF0DC28` 的游戏 metadata root。
- 动态提示由 `0x12340` 分配新 String，再写 UTF-16；未使用会维护全局 intern 表的
  `0x12CE0` 来生成不断变化的价格/计时字符串。后者是预检发现的风险，没有泄漏现场证据。
- 加入原 HUD 自身的非空抑制条件；该 root 具体类型无法从现有信息确认，保留其已知判断语义。
- 文本缓冲区边界、整组价 64 位乘法、INT64_MIN/MAX 有 constexpr 测试。

**为什么这个方案有效**\
类型、虚槽、this-adjustment、value/out 参数和 callee 的读取方式对应同一条调用链，
避免把形式上相同的机器参数误当作相同托管对象。动态文本无需永久加入 intern 表，
并只在当前回调中使用；背包文本由游戏页持有，每次原 hover 重新刷新。

**以后如何避免**\
P0：记录“声明类型 + 完整重载 + invoker + body + 消费对象的 callee”，五者齐备才写入地址。
同名函数、同样的 `void*`、相同长度偏移、编译成功都不能替代这条证据链。
P1：先查原版输入及坐标计算，再决定是否新增虚拟光标。B1 复用原版右摇杆与
`currentCursorTile`，实际手感仍等待真机，而不是凭静态调用链宣称所有设置下可用。

**验证边界**\
已进行源码与 AOT/ELF 复查、文本 constexpr 测试、本地重建；最终逐段 NSO 校验单独保存。
机器/背包表现、旧 Mod 综合回归、长时间性能、保存读档、联机均没有本批真机证据。

---

## 二、重复犯错模式

| 模式 | 曾导致的问题 | 共同根因 | 以后遵守的规则 |
| --- | --- | --- | --- |
| 未追到真实执行/对象层就修改 | 6 基类分派、7 库存、11 地形、17 戒指 | 把名称相同或指针可读当作结构等价 | caller→wrapper→返回对象→真实实现逐层证明，Rule 3/4 |
| 多次更换下游方案却不验证共享前提 | 8 投料、11 小径 v1–v5 | 错误输入被多种方案共用 | 每次只验证一个边界，相同症状重现先查共享上游 |
| 只实现正常路径，漏资源/状态边界 | 9 输出生命周期、14 TileSheet、25 事务 | 把成功返回前后副作用缩成一个操作 | 验证未命中、箱满、部分提交、续产，Rule 6 |
| 凭函数名推断 ABI 或入口 | 6、13、16 | managed 语义与 AOT 机器接口混用 | caller/callee 和 post-link ARM64 双核验，Rule 3 |
| 局部有效就扩大成功范围 | 1 压缩补丁、2 模块头、20 proof、21 文档 | NSO0/编译/部分状态变化被当验收 | 四层状态分开，Rule 1/11 |
| 忽略真实产物与环境来源 | 3 NPDM、4 工具链、5 对象覆盖、20/22 版本路径 | 配置、源码、构建、部署未建立对应 | 从实际构建命令追溯交付，Rule 2/10 |
| 复用已有实现却没保留语义边界 | 8 手工配方、15 电梯窗口、18 UI 锚点 | “复用 API”没有覆盖其上下文和限制 | 先读参考 DLL/构造函数，再确定最小适配，Rule 5/8 |
| 修复正确性后不重看性能与寿命 | 12 GC、19 卡顿 | 可达工作量变大，且长寿命缓存缺契约 | 当前调用内缓存、轮询、规模测试，Rule 9 |
| 审查提出风险后被后续成功覆盖 | 23–26 | 没有保留开放状态与关闭证据 | 每个遗留风险有关闭条件，不因综合成功自动关闭 |

没有证据支持把 Python/CUDA/PyTorch、前后端错判、虚拟环境混用等通用案例列为本项目已发生问题。
也不能把历史 Ghidra 缺失等同于必须安装它才能推进；本项目已使用本地 NSO/Capstone/IL 工具。

## 三、以后必须遵守的项目规则

### Rule 1：校验实际交付 NSO 的所有段，不检查一半就宣布成功（P0）

1. 记录目标文件绝对路径、完整 SHA、size、Module ID。
2. 检查 NSO0、三个段的范围和解压长度，逐段解压。
3. 比对每个启用的段哈希；校验进程退出码非零时不得标记通过。
4. 模块名从实际解压 rodata 检查；不要只看 header name 字段。
5. 源码加入版权/版本，经 ELF/elf2nso 构建。禁止继续用已证明破坏压缩流的定点替换方案。
6. 发布字节有变化就更新 SHA/BUILD_INFO，即使 Module ID 没变化。

原因：问题1在头部、text、大小全部相同时仍损坏；问题2段哈希通过但语义头错误。

### Rule 2：构建前确认真正的 sourceRoot、工具链和 NPDM 来源（P0）

1. 阅读 `tools/build_poc_clang.ps1`，确认编译的是 `runtime/source`。
2. 检查 clang/clang++/lld、头文件13.1.0、compat、A64 sysbase、elf2nso、python、
   原始 NPDM 的路径和版本；保存新的编译错误全文及退出码。
3. 保证各源文件映射的对象名唯一，清理仅限已确认的可再生成对象文件。
4. 链接后检查各功能入口和未解析符号；允许哪些外部符号须对照目标 SDK，不能删光导入列表。
5. 使用原游戏 NPDM 加最小 SVC overlay，不用旧 config.json/其他游戏模板替代。

原因：问题3–5；兼容层不是普通业务实现，修改它须重新检查布局/导入。

### Rule 3：每个 offset 必须同时有版本、函数边界、调用约定证据（P0）

UI 补充（问题27）：同时确认声明类型与完整重载。String / StringBuilder、Item /
HouseRenovation 的同名方法不可互换；动态文本不能误用 intern 工厂。

1. 确认 main Build ID；区分压缩文件偏移、main 相对虚拟地址、ASLR 后地址和 Mod 相对地址。
2. 找真实 caller、callee 与完整函数序言/结尾。
3. 标明 body、wrapper、managed invoker、虚槽或接口分派，附 this-adjustment。
4. 写清 x0–x7、w 寄存器、s0/s1 HFA、栈参数、out 指针、Nullable/value-type this。
5. 分析空值/资源未命中的控制流与异常；再生成 C++ 函数指针类型。
6. 修改后看最终 ELF 调用序列；不匹配版本先停止安装 Hook，当前自动门控缺口见问题24。

原因：问题6/13/14/16，找到函数名远不足以安全调用。

### Rule 4：画出每一次解引用，不能把地址可读当作类型证明（P0）

1. 明确 owner、field address、field value、NetRef、Value 和公开对象分别是什么。
2. 对照游戏调用链逐条保留 LDR 和 unwrap。
3. 地形必须 owner unwrap 后才 Flooring/isinst；Chest 用原接口读库存。
4. 字典/列表/NetCollection 不共享未经证明的布局；不添加“多试几个偏移”的通用 fallback。
5. 同类问题搜索其他消费者，例如全量地形、单格地形、Tapper refresh 都要修。

原因：问题7/11/17/23。两个不同的算法共用同一个 wrapper 仍然会同时失败。

### Rule 5：普通机器扩展优先复用库存级入口和机器数据（P0）

1. 对照本地实际版本 PC DLL 的 factory、wrapper、SetInput/GetOutput IL。
2. 确认新增机器属于 DataBasedObjectMachine 还是专用 wrapper。
3. 普通机器优先只改 target_machines 分类/SupportsInput，使用 AttemptAutoLoad。
4. 不再临时向 Farmer 塞 Coal，不手动补减 Chest stack，不硬编码处理时间和配方。
5. 新增特殊机器前确认 RecalculateOnCollect、经验、统计和 owner 要求。

原因：问题8/9。当前 Bee House 的 RecalculateOnCollect、普通机器经验/统计未完整移植；
鱼塘单独有经验逻辑，不能泛称“全项目没有经验”或“经验全支持”。

### Rule 6：输入、输出和装备都按完整状态转换验证（P0）

1. 分别检查调用前、原生操作返回后、生命周期后、下一次 Tick 和下一周期。
2. 出料就绪不再用 timer 猜测；先保存稳定样本，再由箱子接收，之后执行完整收取生命周期。
3. 检查空箱、满箱、部分可堆叠、多箱剩余量、连续输出与恢复后续产。
4. 遇到 Chest 已接收但 lifecycle 失败，先记录已提交状态，不能盲目重试或随意回写旧指针。
5. 装备调用原 Farmer.Equip，不只画四格或直接叠属性。

原因：问题6/8/9/25。问题25/26尚未关闭；不宣称目前有原子回滚或完整保存/多人保护。

### Rule 7：地图覆盖从对象归属出发，保持当前和后台两种验收（P1）

1. 确认当前位置、根地图、建筑室内是三个来源，未实例化地图不主动加载。
2. 记录 current、roots、interiors、枚举失败与实际扫描数，区分视图不可读与没有该类型对象。
3. 验证人在 Shed 内和离开 Shed 后的处理；地图网络之间不能合并库存。
4. 同格实体先合并，再四向连接；鱼塘覆盖完整矩形。
5. 轮询只存索引，地图变化时重取指针，检查去重与游标边界。

原因：问题10/11/19/26。

### Rule 8：UI 改动以控件 ID、资源和真实坐标语义为依据（P1）

1. 找 InventoryPage/MineElevatorMenu 构造、draw、click、hover、controller 入口。
2. 保存原矩形和邻居，按 ID 定位，拒绝凭 List 下标或字段名推断布局。
3. draw/hit-test/hover/controller 共用布局；验证 trinket 解锁前后。
4. 电梯区分相对层、实际层、已到达层与窗口容量；普通矿井继续原逻辑。
5. 地图资源先查存在性，缺失时用已确认的创建/添加接口。

原因：问题14–18。验证新 UI 不应顺便改变 Automate 业务；共享 offset/runtime 修改另查影响面。

### Rule 9：控制单帧峰值；缓存必须有明确有效期（P0）

1. 保留当前30 Tick/后台4 Tick的一张地图轮询模型，确认任务错峰。
2. 同一个集合的 unwrap 方法在本次视图只解析一次，不对每个 feature 重做 svcQueryMemory。
3. tile 队列保持哈希去重/epoch；处理饱和、上限与大图时不要悄悄改变连接语义。
4. 缓存优先索引/原生数值/字节，托管指针要证明 rooted/pinned/更新机制。
5. 长小径、多机器、多室内验证帧时间与后台公平性，不能用删除功能换取“优化成功”。

原因：问题12/19；v8有用户体验闭环，没有量化 profiler 数据。

### Rule 10：每次测试绑定一对部署文件和对应符号（P0）

1. 记录 normal/trace/audit/proof 三个实际开关；`Fatal proof: False` 单独不足以排除 Phase8Audit。
2. 构建会改 staging，诊断版结束后必须重新确认普通版，而非根据目录名判断。
3. 对比构建输出、variant、副本及实际 SD 上文件的 SHA；SD 无法读取则标“待用户确认”。
4. 完全关闭游戏后成对复制 subsdk9/main.npdm，再启动；HOME 恢复不是重载。
5. 日志先查 Title ID/Module ID/时间，再用对应 ELF 解析地址。
6. 只更新已授权的交付范围；本次复盘不自动上传、推送、覆盖主机或恢复包。

原因：问题1/3/20/22。

### Rule 11：交付结论必须能逐项追溯到证据（P1）

每次说明：源码实现与静态分析、本地编译、实际产物完整性、真机功能验证各是什么状态。
采用“候选/现场通过/用户反馈通过/未执行/无法确认”而不是笼统“完成”。
发现新证据推翻旧判断，更新当前入口说明和对应问题，不把早期 HANDOFF 当作当前事实。
根 Git 缺失与 untracked 要明确，不能给修复编造 commit。

原因：问题21–26。根因未闭合时，可以记录候选和需要的 trace，不能宣称已永久解决。

## 四、固定 Debug SOP

### A. 首先建立本次故障的身份

1. 保存用户现象、最小摆放/物品、发生时间与日志来源；不要先运行会覆盖 staging 的构建。
2. 获取本次文件完整 SHA、Mod Module ID、游戏 Title ID/Build ID、normal/诊断模式。
3. 对 NSO 跑完整三段校验。若像问题1失败，先停在产物层，业务调参没有意义。
4. 与同构建 ELF/日志关联；旧日志只能解释旧构建，其他进程报告不作为证据。
5. 分到下面最窄的入口；按第一处不符合预期的边界继续，而非按最后一个 Abort 乱改。

### B. 无法构建 / 构建后启动失败

1. 核实 cwd 和实际脚本，确认是 runtime/source，而非根 src 或旧 Makefile 流程。
2. 读取第一个编译/链接错误；核对 target、LLVM/C++头/compat/sysbase 路径，确认依赖存在。
3. 检查对象名碰撞、旧对象残留、ELF 动态导入和所有功能符号。
4. NSO 三段校验、解压 rodata 的模块名头、MOD0/重定位。
5. 启动日志若未到 exl_main，优先模块结构/导入；若 Undefined SVC 0x74，优先 NPDM pair。
6. 已到 hook-installed 后失败，再核对入口序言、SP/LR、原函数返回与回调 ABI。
7. proof 仅在正确调用点主动 Abort 才算命中；normal 必须正常进入游戏。

### C. 不崩溃但 Automate 没效果

1. 场景固定为室外普通箱子邻接单台机器，用用户报告的确切物品和数量。
2. 看真实 location 是否进入扫描：current/root/interior、readable、scans。
3. 看 machineNodes/chestNodes；为零先查字典读取、ID/类型识别，区分大箱子命名 ID。
4. 小径按 terrainEntries → raw field → unwrap → flooringNodes → ItemId → connectorNodes。
5. connectorNodes=0 时不要重写 flood；节点存在才验证同格/四向/空格/斜角/鱼塘矩形。
6. groups=0 查连通；有组后查 chestReadFail、candidates、autoloadAttempts/Success/Fail。
7. 输入成功但无产物看 startStateFail 和机器状态；不要额外手动扣料。
8. 输出看 held/ready → Chest 接收 → lifecycle → 下一周期；不要添加 timer-ready fallback。
9. 再扩大到机器链、多箱、大小箱、Shed 内/离场、跨图。

计数器来自当前源码，但 heartbeat 多项是累计计数或最近一次扫描值：
结合输出时间、场景、增量解释，不把单条“0”或旧表格当作全局状态。

### D. Data Abort / managed exception / 空地址跳转

1. 用正确 ELF 将 PC/LR 映射到函数；区分 main offset 与 subsdk9 offset。
2. PC/LR 都为0：查函数边界、序言和栈平衡（问题16）。
3. X0 类似 `0x195`：查数值误作 this/out 指针（问题13）。
4. 值类似 `0x62006e0078`：查容器误识别，范围检查之外还要证明类型（问题7）。
5. managed interface abort：从对象来源倒查 NetRef/field/LDR、虚分派（问题6/17）。
6. 资源 getter 抛异常：反汇编失败分支，查当前地图是否确实有资源（问题14）。
7. 修复一层后新栈进入下一层，记录新故障；不因为依然崩溃就撤销已证实的上层修复。

### E. 周期性卡顿

1. 对照 30/4 Tick 扫描、电梯相位及1800 Tick heartbeat，记录实际卡顿时机。
2. 统计本帧访问多少地图/feature、解析多少虚槽/系统调用，查是否重新集中全图。
3. 查 QueueTileUnique 是否退化为线性搜索，以及哈希容量/epoch。
4. 对比短/长路径、少/多机器、当前/后台地图；条件相同才比较优化。
5. 不跨 Tick 存托管裸指针来省查询；按调用内缓存或稳定索引减少成本。
6. 验证功能、公平轮询、跨图续产均保留；说明量化数据是否实际采集。

### F. UI/电梯看起来不对

1. 先确认加载的新 SHA/可见布局标记，排除旧包。
2. 按构造函数列控件 ID、矩形、上下左右邻居；验证已解锁和未解锁 trinket。
3. 分开验证 draw、click、hover、controller；外观成功不代表点击对象来源正确。
4. 电梯资源存在性与 Action 单独测，窗口滚动与目标层换算单独测。
5. 验证原矿井、原装备、Automate 未受共享 Hook/runtime 改动影响。

## 五、修改前 Checklist

- [ ] UI 同名重载已追到具体对象的消费方式；已检查原版光标输入、缩放与提示重置逻辑。

- [ ] 已读首页当前 P0，以及相关问题/Rule；旧方案若要重新考虑，已写出前提变化证据。
- [ ] 确认工作区、runtime Git 边界、已有脏文件及 untracked；保留原修改，不擅自 reset/clean。
- [ ] 找到真实构建脚本、入口、调用方、共享 runtime 消费者；知道根 src 是否参与运行。
- [ ] 确认原始 main 身份、部署 NSO 和参考 ELF 的对应关系；未用过期 BUILD_INFO 代替哈希。
- [ ] 检查已有 wrapper、类型判断、白名单、接口分派；参考 PCmod DLL 使用的是当前实际版本。
- [ ] 新 offset 有真实 caller/callee、函数边界、参数与失败分支证据。
- [ ] 区分字段地址、NetRef 指针、Value、公开对象；不靠可读性或多个猜测布局“试出来”。
- [ ] 已确认配置实际来源、工具链路径与版本；需要新依赖时先核对现有工具。
- [ ] 明确事务/GC/保存/主机所有权边界；未知项没有冒充已实现保护。
- [ ] 能把改动限制在最小模块，列出共享文件可能影响的 Automate/电梯/戒指/鱼塘消费者。
- [ ] 已设计最小证伪方法与对应验收；新增测试仅覆盖有意义的不变量和风险，不镜像实现。
- [ ] 构建前保留需要符号化的旧 NSO/ELF/日志和已验证参考，避免 staging 覆盖证据。
- [ ] UI 改动有控件 ID/矩形/导航表；扫描改动有调度与复杂度预估。

## 六、修改后 Checklist

- [ ] 新文字调用在 ELF 中仍是正确 String 重载；数值变化没有进入全局 intern 表。
- [ ] UI 分批账本明确源码/真机状态；同物品反复悬停没有重复追加，世界信息正确隐藏。

- [ ] 审阅 diff：`git -C runtime diff --check`、diff/stat/status；
  untracked 和根目录脚本/文档单独查看，不能只看 Git diff。
- [ ] 与改动相称的语法/静态检查、编译和链接；记录首个错误、非零退出码和新 warning。
- [ ] 比对最终 ELF：修改常量、调用 ABI、函数入口、所有应保留模块、允许的动态导入。
- [ ] 实际交付 NSO 的 text/rodata/data 均解压且启用哈希匹配；模块名头正确。
- [ ] 确认 `FatalProof / Phase8Trace / Phase8Audit` 实际状态；交付普通版三者均关闭。
- [ ] 源文件、build、staging、variant/package 的身份对应；更新 BUILD_INFO/文档 hash。
- [ ] 原始 main/NPDM 未被修改，部署是正确 Title ID 下的一对 overlay 文件。
- [ ] 语法/构建之外完成最小运行验证；没有硬件时明确标记“未执行”，不打勾代替。
- [ ] 核心功能、日志新 ERROR/warning、保存/读档/切图按涉及范围回归。
- [ ] 修改实际生效有本次 SHA/Module ID/日志/可见行为证据。
- [ ] 共享模块修改后检查旧功能；生命周期修改至少两周期及满箱/部分堆叠。
- [ ] 将新重要问题、失败方法、修复和验证结果追加到本手册；更新开放问题状态但不覆盖旧证据。

项目专用真机矩阵（按改动选择；没有执行逐项标“未执行”）：

| 范围 | 最小用例和成功标准 |
| --- | --- |
| 装载 | normal 连续游玩至少2分钟；proof 的目标调用点另验，不能混作稳定测试 |
| 普通机器 | 压酪机手动投入后出料，再 Milk x2 自动连续两轮；熔炉原版合法配方自动往返 |
| 生命周期 | Seed Maker 全部连续产物；Tapper/Heavy Tapper 下周期继续生产 |
| 库存 | 满箱保留；只剩部分堆叠空间；多箱分配；总量无丢失/重复；四种支持箱子 |
| Connector | 直线、转弯、小径在机器/箱子下；断开/斜角不连接；长链；鱼塘各边相邻 |
| 地图 | 当前农场、当前Shed、人在室外仍处理Shed、离开农场；按完整轮询时间等待 |
| v9新增 | 罐头机、晶球破开器分别完成合法输入、出料、两轮循环、箱满保护 |
| 电梯 | 入口图标/Action、121与5层检查点、相对125→实际245、0返回、普通矿井行为 |
| 四戒指 | 四格绘制/放入/取出/效果；鼠标或触摸及手柄导航；trinket锁定/解锁 |
| 状态边界 | 保存/重载、过夜、切图；主客机分别验证；缺对应门控时不得推断兼容 |
| 性能 | 大地图/长小径与后台轮询的响应和帧时间；优化前后场景保持一致 |

### 可直接使用的本地检查命令

以下从项目根目录运行；构建命令会覆盖 staging，先完成问题身份与旧产物保留。
原有 `analyze_nso.py` 在某些情况下会打印 hash MISMATCH，因此不能只看退出码，
还要核对所有三段；随 skill 提供的检查器将段失败统一变为退出码1。

```powershell
git -C runtime status --short
git -C runtime diff --check
git -C runtime diff --stat

python .\tools\analyze_nso.py .\exefs\main --no-xrefs --max-strings 0
python .\tools\analyze_nso.py .\atmosphere\contents\0100E65002BB8000\exefs\subsdk9 --no-xrefs --max-strings 0

# 只读、逐段检查；可选比较参考副本
python C:/Users/43017/.codex/skills/stardew-switch-engineering/scripts/verify_artifacts.py ./atmosphere/contents/0100E65002BB8000/exefs/subsdk9 --project . --compare ./runtime/build-clang/subsdk9-normal

# 需要实际代码构建时才执行；不是只读检查
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\build_poc_clang.ps1

# 用与报告相同构建的 ELF；不要拿新 ELF 套老 PC
& 'C:\Program Files\LLVM\bin\llvm-nm.exe' -C .\runtime\build-clang\automate_lite-normal.elf
& 'C:\Program Files\LLVM\bin\llvm-objdump.exe' -d --demangle .\runtime\build-clang\automate_lite-normal.elf

Get-FileHash .\atmosphere\contents\0100E65002BB8000\exefs\subsdk9 -Algorithm SHA256
Get-FileHash .\atmosphere\contents\0100E65002BB8000\exefs\main.npdm -Algorithm SHA256
```

## 七、P0 / P1 / P2 经验索引

### P0 — 必须牢记

- **同名 AOT 方法还必须核实声明类型和重载**：String/StringBuilder、Item/HouseRenovation
  的 void* 参数和部分偏移相同也不能互换；动态提示不进 intern 表（问题27）。

- **完整容器校验优先**：压缩 NSO 等长改字节会坏；当前 F2473DF7 产物就是现场反例（问题1，Rule1）。
- **启动链不是只有 Hook**：模块名头、导入、对象链接和 NPDM 权限都要正确（问题2/3/5）。
- **ABI 与对象解包优先于算法**：基类/虚分派、value-type this、函数序言、NetRef 多一级（问题6/7/11/13/16/17）。
- **库存遵循原版事务和生命周期**：不再人工配方/煤炭/扣堆栈，完整收取后续产（问题8/9/25）。
- **全图覆盖不可遗漏，单帧成本不可失控**：室内归属和轮询分别解决（问题10/19）。
- **以下风险仍开放**：精确地图集合解析、真实 Build ID 门控、提交后恢复、保存/主机门控（问题23–26）。
- **成对部署与诊断模式身份**：关闭游戏后重载；同时记录 SHA、Module ID、全部模式开关（问题3/20）。

### P1 — 应重点避免

- 工具链路径和配置来源不明时先核实实际构建，避免重装/乱改业务（问题4/22）。
- 托管对象寿命和地址稳定性分别证明，不把 GC 风险推测写成唯一已证实根因（问题12）。
- UI 的控件 ID、锚点、窗口、资源失败语义要还原（问题14/15/18）。
- 新旧日志、其他进程报告、无文件日志通道要区分（问题20）。
- 每次交付分清实现/编译/容器/真机；旧 HANDOFF 和 BUILD_INFO 不是免检凭据（问题21）。

### P2 — 工程优化

- 补齐本地源码与发布元数据的版本化关系；当前只有 runtime 浅克隆，核心 untracked（问题22）。
  这是后续工程建议，本次未擅自建立新仓库、提交或上传。
- 统一当前状态入口，保留历史章节但标明被哪个后续证据取代；不重复散落“当前 staging SHA”。
- 日志保存短小的分阶段计数与版本身份；需要日志时先确认能采集，避免每帧大量格式化。
- 历史审查提到 `runtime/source/lib/reloc/reloc.hpp::GetSymbol(const char*)`
  非 void 末尾缺返回；本轮源码仍存在。无法确认运行时触发，后续触及该导入路径前修正并验证，
  不把 warning 消失当作所有运行风险已消除。
- 检查器/分析脚本的新增测试覆盖真实不变量，如损坏 NSO 必须返回失败、原始文件不得改变；
  不为文档内容本身堆叠形式化测试。

## 八、维护约定与当前验收账本

项目根 `AGENTS.md` 已要求重要修改前读取本手册。
个人 skill 位于 `C:/Users/43017/.codex/skills/stardew-switch-engineering/SKILL.md`，
提供精简流程及只读 NSO 检查脚本；详细事实以本文件为唯一维护入口。
如项目迁移，在 skill/命令中传新 root，不复制一份长期不同步的经验正文。

重要新问题解决后追加：

```text
问题编号与等级 / 状态 / 日期
现象与最小复现
证据路径、日志编号、游戏 Build ID、Mod SHA/Module ID
直接原因 / 根本原因（事实与推断分开）
失败尝试及为什么失败
有效修复：文件、函数、参数、调用链
为什么有效
静态 / 构建 / 产物 / 真机四层验证，未执行项
回归范围与残留限制
对应 Rule / SOP / Checklist 的增补
```

同类问题新版本再现时，先比较条件差异（版本、对象类型、调用入口、部署身份、地图状态），
不要简单重复旧补丁。用户已要求以后持续更新项目经验，不需要再次询问是否维护这份项目文件；
但更新本文件不等于授权外部发布、主机操作或其他任务。

2026-09-05 初始复盘历史结果（后续状态见下表）：

| 项目 | 结论 |
| --- | --- |
| 原始 main | SHA 与历史身份一致，本轮未改动 |
| 当前 F2473DF7 staging / v9 deploy / build subsdk9 | 同哈希；staging rodata 校验失败；未覆盖 |
| 未打标 v9 subsdk9-normal | text/rodata/data 解压与哈希通过；不等于新增机器真机通过 |
| v8 | 历史用户明确反馈成功；本轮只核验本地参考哈希/产物，不重复真机测试 |
| 源码 | 已核对重要修复实现与遗留风险；没有改动运行时代码 |
| 日志 | 原开发任务中有日志摘录与用户反馈；原 Temp 崩溃文件本轮未找到 |
| Git | runtime 浅克隆/脏文件/untracked 情况确认，不能提供不存在的修复 commit |
| 本轮验证范围 | 26个问题字段齐全、11条Rule、文档链接/UTF-8检查通过；skill-creator校验通过；只读检查器对损坏文件返回1、完好参考返回0；未做游戏重建、部署、主机测试或远端操作 |

2026-09-06 UI Info Suite 2 B1 后续结果：

| 项目 | 结论 |
| --- | --- |
| staging / build / normal | 已源码重建；SHA `922844B6BA5A0AE7C969A57BF5354BE872ACFDFCCFC595888A64625DB1EC19A8` |
| 三个 NSO 段 | 解压、尺寸、启用哈希全部通过；不再使用损坏打标文件 |
| UI 子功能 | 背包单价/整组价、机器倒计时/完成提示已接入；其他功能待分批实现 |
| 原有功能 | 原逻辑文件与快照相同；共享入口追加 UI；综合真机回归未执行 |
| 本地验证 | constexpr、构建、AOT/ELF 复查、导入对比、容器校验通过；已有 warning 保留 |
| 交付 | `deploy/uiinfo-suite2-b1-merged-v10.zip`，符号位于同名目录 `symbols/` |
| SD/真机/联机 | SD 未写入；本批真机未执行；联机未适配，不能声明兼容 |
| 经验 | 新增问题27；初轮错误重载候选保留为 rejected，最终入口修正为 String 重载 |

**关闭问题的必要条件是新证据，不是又生成了一个版本号。**

## 九、2026-09-14 单人移植新增经验

以下区分实际工具/编译失败与静态发现的候选问题。没有新增真机崩溃日志，不能把潜在风险写成发生过的事故。用户已排除联机，旧 Checklist 的主客机项目在本次标为“不在范围”，不是“已通过”。

### 问题 28：IL token 解码和元数据名称不能替代真实 ABI（P0，分析工具修正）

**现象**：本轮需要从 PC DLL 的 isinst、MemberRef、TypeDef 对照 Switch 实现；分析器类型表映射错误会使类型归属不可信。PC 的托管结构、nullable 参数与 Switch 不同。

**直接原因**：`tools/dump_dotnet_il.py` 的 token 高字节类型映射需修正为 Module=0x00、TypeRef=0x01、TypeDef=0x02；仅查 UTF-16 同名字符串也不能证明入口或调用签名。

**根本原因**：分析工具输出曾未充分区分“名称线索”与“可直接调用的 ABI 证据”。是否曾因此造成真机事故，无法从现有信息确认。

**错误尝试**：按同名方法直接套用 PC 定义的路线不能解决 nullable/HFA、继承、重载问题；本轮在交付前改用多层核实。不是已执行过的真机失败方案。

**最终解决方法**：修正 token 表；新增 `extract_aot_metadata.py` 限定工厂与少数构造函数/分配器的离线解码，拒绝未知外部代码；再对照 invoker 和真实消费者。地图 Position 为 24 字节，NullablePosition 为 32 字节，经 x8 出参；Point 经 x1 值传递；Rectangle 经 x2/x3，NullableRectangle 经 x4 指针。

**为什么有效**：字段/参数的元数据与真正使用寄存器/栈的机器指令相互约束，避免凭 void* 编译通过误判类型正确。FruitTree/GameMenu 重提取一致，最终 ELF 对复杂参数复查。

**以后如何避免**：每个新 AOT 调用至少记录声明类型、body、invoker、实参布局、返回布局及一个消费者；工具输出异常先修工具，不调整运行时代码迎合错误输出。

### 问题 29：通用 tooltip 迁移遗漏旧 Hook 调用方（P1，实际编译失败已修正）

**现象**：移除 B1 专用 `AppendInventoryHoverInformation` 后，一次构建仍发现旧调用。错误发生在参与构建的 `runtime/source/hooks/wear_more_rings.cpp`。

**直接原因**：起初检查了同名 rings 实现文件，真正调用在 hooks 目录。根目录/多个同名文件使局部搜索不足。

**根本原因**：先删除接口，再完整追踪调用方，顺序倒置。

**错误尝试**：只清理 `runtime/source/rings` 无法消除 hooks 中引用；还造成该实现文件只有换行变化的无关 diff。

**最终解决方法**：移除真实 Hook 中旧调用，保持四戒指逻辑；让 `items.cpp::ItemTooltipHook` 统一追加。用 B1 备份比较确认 rings 只有换行差异后恢复原字节。构建通过。

**为什么有效**：只有一个提示扩展入口，不重复追加；接口和真实调用者一致，保持原功能实现。

**以后如何避免**：删改公共接口前 `rg` 搜完整 `runtime/source`，确认 hooks/实现/头文件全链路；比较未跟踪文件和根脚本，不能只看 git diff。新候选四戒指真机回归未执行。

### 问题 30：“查询”方法可能修改游戏，熟成分钟数不是实际剩余时间（P0，静态发现并修正）

**现象**：静态分析发现 TV.getWeeklyRecipe 会学会菜谱，GetWeatherForLocation 在缺失时创建天气记录；Cask.DayUpdate 将普通机器分钟字段设为占位值，同时按天更新另一组字段。

**直接原因**：方法名 get 不保证无副作用；同为 Object 的派生类型并不共享计时语义。

**根本原因**：若按名称和基类统一套用，会把展示功能变成写操作或显示错误结果。此为被静态拦截的风险，不是用户已报告的存档变化。

**错误尝试**：候选通用机器分钟逻辑不适用于熟成桶；直接使用 TV 周菜谱方法的方案经指令审查被排除，未作为成功方案交付。

**最终解决方法**：`hud.cpp` 读取 CookingChannel/knowsRecipe；重播使用已存在周数或只读计算和独立日期种子 RNG。姜岛只取 NetDictionary 的成功查询分支。`ui_info_suite.cpp` 熟成按 daysToMature/agingRate 向上取整。源码与证据在 bundle-cache、collection-tv-rerun、cask-aging 汇编记录中。

**为什么有效**：显示只读取既有游戏数据，不学习菜谱或补建天气；熟成计算与原版每日减 agingRate 的更新规则一致，非整天不会提前报完成。

**以后如何避免**：调用查询接口前看完整函数中的 NetField setter、随机数、内容加载与隐式创建；派生机器先查其更新方法。没有只读分支时缺省不显示，而非构造游戏状态。

### 问题 31：按存档配置需核实 SDK/权限、断写与完整名字（P0/P1，新增设计防护；真机开放）

**现象**：PC 配置依赖 SMAPI，在 Switch 不能直接沿用。设置成功留在内存不等于已写 SD；单文件覆盖遇到失败可能失去旧设置，名称哈希也不等于完整 NPC 身份。

**直接原因**：运行平台、持久化 API 和文件权限不同；配置存在跨退出/跨存档生命周期。

**根本原因**：若只完成 UI 开关而不验证真实加载/保存路径，用户重启后会看到状态不一致。实际 SD 写失败或重启丢失，无法从现有信息确认。

**错误尝试**：内部候选仅覆盖文件不能保证旧有效记录，且坏槽长度可能不符合新格式。未发布候选不算已经在真机失败的版本。

**最终解决方法**：`settings.cpp` 使用 nn::fs 原生挂载；确认 9 个导入存在于实际 SDK，核对原 NPDM 两处文件权限，未增加权限。按存档 ID 双槽；校验、递增序号、回绕比较、另一槽写入、SetFileSize 修复长度、Flush 后读回并比对校验值。格式 v2 保存完整内部名，超长拒绝；失败提示内存状态。

**为什么有效**：一槽写失败仍可选另一有效槽；校验绑定版本/存档/开关/名字，避免误读其他存档、损坏名字或部分写入。

**以后如何避免**：原生配置同时验证导入、权限、目录、打开模式、实际长度、错误返回和重启恢复。编译期损坏/回绕测试与 SDK 导入检查通过，**真实 SD 挂载、写入、损坏回退、重启恢复仍未执行**，不得闭环为真机成功。

### 问题 32：UI 替换必须遵守原页面关闭条件（P0，静态发现并修正）

**现象**：PC 打开日历前会检查手中没有拖动的物品。新原生设置面板若直接替换菜单，可能绕过原页面对 heldItem 的保护。

**直接原因**：入口绘制/点击不等于允许关闭当前页面；不同页面通过 readyToClose 虚槽实现自己的约束。

**根本原因**：只看新面板功能，漏掉旧菜单仍拥有的状态。没有发生过物品丢失的真机证据。

**错误尝试**：早期本地候选入口只做坐标命中，没有 readyToClose 门控；在本轮最后审查修正，未把它写成用户事故。

**最终解决方法**：`menu.cpp` 入口调用 GameMenu.readyToClose 16EE120；该方法获取当前页并调用 0x2A0 虚槽，遵守页面原保护。高度不足 360 不进入面板，避免负行距/除零。日历/委托从空手进入的面板打开。

**为什么有效**：复用原菜单的状态判断，避免只检查某个 InventoryPage 字段而漏掉其他页。

**以后如何避免**：新增菜单、覆盖输入、切换活动页面前确认谁拥有拖动物品和焦点，查 readyToClose/cleanup 调用链。新版本空手/拖物品、返回/退出、按键交互仍需真机验收。

### 重复模式与新规则

| 模式 | 本轮涉及问题 | 根因 | 必须遵守 |
| --- | --- | --- | --- |
| 名称/父类代替执行语义 | 28、30 | 抽象层不同 | P0：先核实完整 ABI 和方法副作用，再接 UI |
| 修改局部、遗漏调用方 | 29、32 | 入口和拥有者未追全 | P0：界面替换查生命周期；P1：接口改动全树找调用方 |
| 本地状态当持久化/验收 | 31、旧21 | 证据范围混用 | P0：SDK/权限/读回与真机四层分开，日志不能替代重启恢复 |
| 原生 UI 查询过频 | 28、30 | 忽视帧回调成本 | P1：缓存复制后的值；范围先裁剪视口；不缓存托管裸指针 |
| 只看 tracked diff | 29、旧22 | 项目大部分新源码 untracked | P2：与已知基线逐文件比较，保留用户既有改动 |

### 单人 UI Debug SOP / Checklist 增补

修改前：

- [ ] 核对真实构建 root、main Build ID、B1/候选身份；先备份将被覆盖的产物。
- [ ] 检查是否命中 27–32 的重载、解包、只读查询、配置和菜单状态问题。
- [ ] 字段来自元数据、ABI 来自 invoker/消费者，未把 PC 类型直接当 Switch 布局。
- [ ] 地图/日期/NPC/玩家归属明确；本次单人，不顺便扩大到联机。
- [ ] 绘制和坐标使用同一 UI/世界视口；原版菜单、截图、事件门控已考虑。

出现问题时：先确认安装包 SHA 与该包 ELF → 完整日志首个异常 → 禁用对应 UI 开关缩小范围 → 核对 Hook 入口和参数 → 字典 owner 解包 → 实際游戏状态及只读方法 → 原版 UI 缩放/输入/关闭条件 → 最新最小改动。设置问题另外先看面板保存结果，再看实际 SD 双槽长度/校验/存档 ID，不先乱改 NPDM。

修改后：

- [ ] `tools/test_uiinfo.ps1` 与实际构建通过，保留 warning 和失败输出。
- [ ] 新 SDK 导入在游戏 SDK 存在，NPDM 只含已知必要差异。
- [ ] 完整 NSO 三段解压/摘要，staging/包内文件一致；不要二次打标。
- [ ] 检查原 Mod 源码对比和所有新文件；只恢复明确属于自己的无关换行。
- [ ] 按 `docs/UIINFO_SUITE2_SWITCH.md` 真机清单逐项填写。未执行就保持待验收。
- [ ] 更新本页当前候选、包 BUILD_INFO、ZIP 哈希；旧 B1 回退包保持原样。

2026-09-14 本地结果：ARM64 编译期测试、完整构建、9 个新增导入对照、元数据重提取、NSO 三段校验通过；原始 main/NPDM 未修改；安装包与符号准备在 deploy 下。**未写实际 SD，未执行新版本真机验证，不做联机适配。**

### 问题 33：合并版 Hook 数量超出共享池，v11 启动即主动中止（P0）

**现象**：用户随后提供 `01789389449_0100e65002bb8000.log`，MainThread 在 exl_main 安装 UI 菜单 Hook 时 Data Abort。PC 为 mod+0x6fcc（AbortImpl），X27 为主动中止标记 0x6969696900000000。完整原日志和定位位于 `analysis/crashes/uiinfo-v11-20260914/`；本次有原始日志，不受本文开头历史日志缺失的限制。

**直接原因**：v11 `JitSize=0x1000`，每个 trampoline 保留 200 字节，只够 20 个。正常安装需要 28 个，含可选 trace 总计 29 个。`AllocForTrampoline` 使用 `i > HookMax`，错误放行第 21 个（索引 20），第 22 个 MenuScrollHook（索引 21）分配失败并触发 R_ABORT_UNLESS。

**证据**：日志 Module ID 8ABDD215…8237 与不可变 v11 ELF 匹配。ELF +0x7608 调用 AbortImpl，源行 602 是分配失败检查；+0x16714 调用 Hook 的返回 +0x16718 对应 MenuScroll 安装。旧 ELF 检查 `cmp w8,#20; b.hi`。第 21 个的完整保留区域可能越过池末尾，但日志不能确认实际越界写入或其他内存损坏。

**根本原因**：扩展完整 UI 功能时只验证各 Hook 的指纹、ABI、编译和产物，没有检查原 Mod 与新增 Hook 共用的有限启动资源；框架分配器还存在上界错误。局部正确不能保证合并启动成功。

**错误尝试**：没有证据表明本次曾靠重装、修改游戏版本、删存档或改 UI offset 尝试修复。此前把编译/NSO 完整性作为主要交付检查没有发现容量超限；这些检查必要，但不覆盖运行时分配预算。不能编造“已尝试且失败”的过程。

**最终解决方法（源码修复，本地验证通过，真机闭环待确认）**：

1. `runtime/source/program/setting.hpp` 的 JitSize 改为 0x2000，40 个槽。
2. 新增 `lib/hook/nx64/pool_layout.hpp` 统一大小/容量/严格 `< capacity`；`hook_impl.cpp` 分配器复用该判断。
3. `tests/hook_pool_test.cpp` 检查空池、首末合法索引、首个越界索引及真实第 22/29 次需求，加入 `tools/test_uiinfo.ps1`。
4. `tools/check_hook_budget.py` 从源码读取布局常量、枚举合并项目 Hook 与唯一静态安装点，保守计入 trace 并要求至少 8 槽余量；实际构建脚本在覆盖产物前执行。
5. v11.1 单独打包，保留失败 v11 的 NSO/ELF/源码和用户已测试 B1；无二进制定点打标。

**为什么有效**：修复安装失败的共享容量，而不是跳过菜单功能。8 KB 预留 40×200=8000 字节，29 个全部装入后还有 11 槽；严格边界不再允许访问第一个非法槽。最终 ELF 确认 JIT 区域为 0x2000，分支为 `cmp w8,#39; b.hi`。该结论不等于未运行的 UI 全部正确。

**验证范围**：旧 4 KB 预算负向测试拒绝、新 8 KB 通过；ARM64 编译期断言、正常构建、三段 NSO 解压/哈希、staging 一致性通过；与失败 v11 相比 SDK 导入相同，247 个原源码文件未变，仅修改分配器/配置/版本及新增布局头。详见 `analysis/uiinfo-v11-1-fix-audit.json`。v11.1 真机启动、读档、原 Mod 与新 UI 功能回归 **未执行**。构建仍有既有 reloc 缺返回和框架类型警告，未宣称零警告。

**以后如何避免 / 新规则**：

| 等级 | 模式 → 曾导致的问题 → 根因 → 必须遵守 |
| --- | --- |
| P0 | 只审新增模块 → 问题 33 启动失败 → 忽视共享容量 → 新 Hook 按整个 subsdk9 统计，计算槽位大小与严格上界，保守包含条件构建 |
| P0 | 编译/解压成功扩大为运行可用 → 问题 33 → 验证层次不同 → 安装预算、最终 ELF 和真机启动分别记录 |
| P1 | 从 Data Abort 猜对象指针 → 会延长排查 → 忽略主动中止栈 → 先匹配 Module ID/ELF，再看 AbortImpl 上游失败的 Result/断言 |
| P2 | 版本源码与产物混用 → 无法复现 → 只保留最新版 → 失败包同样保留完整 ELF、日志、哈希和源码 |

**启动 Debug SOP 增补**：完整日志 → main/mod 身份 → 匹配旧 ELF → PC/返回地址符号化 → 若为 AbortImpl 则找上层分配/权限/断言 → 统计实际安装顺序和整个共享池 → 最小容量及边界修复 → 编译期边界与预算正反检查 → 最终 ELF 反汇编 → 三段 NSO/NPDM/包校验 → 新包真机启动与读档。尚未进入游戏时，不能用 UI 开关缩小范围，也不要先修改游戏业务逻辑。

修改前 Checklist：

- [ ] 新增/重复安装是否消耗共享 trampoline/inline/JIT 资源？各池预算分别是多少？
- [ ] 安装保持一次调用且不在循环；静态清单不是控制流证明，动态安装需显式扩展预算。
- [ ] 老包的日志、NSO 和 ELF 身份是否严格对应并已保留？

修改后 Checklist：

- [ ] 旧失败配置必须被预算检查拒绝；当前最坏配置和边界测试通过。
- [ ] 新 ELF 实际区域大小、分配步长、拒绝边界正确，不只读头文件。
- [ ] 共享框架修复没有删减原功能；对照不可变前包，记录变化范围。
- [ ] 真机启动后再做功能回归；未收到新反馈前保留“未执行”，不复用 B1 成功声明。

### 问题34：早期力度写入未覆盖实际抛竿，Perfect不保证高品质（用户反馈，v12.1候选）

现象：用户反馈v12抛竿不是最大距离、钓不到高品质鱼，其余功能完好。未获得逐帧日志，不把静态解释冒充真机因果复现。

证据：v12仅在pressUseToolButton返回true且isTimingCast已置位时写castingPower=1。实际doStartCasting位于0x1A95E80，0x1A95FD4清除timing并置casting；tickUpdate在0x1A9CEF0按timing分支决定是否继续改力度，0x1A9D440等读取力度计算射程。写工具键返回时的早期状态不能覆盖所有延后开始路径。
品质在0x1A94CBC..CF0：普通0+Perfect仍为0，银1升金2，金2及铱4升/保持铱4。v12只改perfect，没有设置基础品质。

修复：新增实际doStartCasting的单参数body Hook，只在自动化开启且玩家/鱼竿匹配时于Orig前固定力度1；现有BobberBar.update Hook把fishQuality+0xD0设为4，再走原版结算。没有重写落点/距离公式，没有修改垃圾或宝箱物品。与v12相比只改fishing.cpp、signatures.hpp及版本字符串。

失败尝试：没有额外真机试错记录。旧v12为已收到缺陷反馈的方案，不重复将其早期写入认定为全路径满力度。
验证：原版品质分支8组输入在隔离ARM64模拟器全部符合预期；原始text哈希锁定；本地构建、三段NSO校验、Hook31/40槽通过。新的满力度/铱星修复真机复测未执行。旧v12包保留为完整源码/ELF/NSO基线。

规则：对异步动画/事件触发的动作，在最终消费状态的入口确认参数；“完美”标志与物品品质是不同状态，必须检查结算映射。详见docs/AUTO_FISHING_SWITCH.md与analysis/fishing-v12-1-quality-test.json。

### 问题35：NPC地图坐标单位、虚方法与绘制阶段（静态发现，v13真机开放）

现象：整合 NPC Map Locations 3.5.2 大地图时，静态发现旧首字标记用 mapBounds 的未放大宽高裁剪显示坐标，会漏掉右侧/下方标记；旧 VisitVillagers 直接调用 NPC.get_IsVillager 基类 body，实际恒返回 true，不能区分怪物。这是静态证据，不是用户提交的真机缺陷报告。

证据：MapPage.drawMap `0x173D2DC..2FC` 将 mapBounds 宽高左移2位，原版头像 `0x173CA00..A20` 对 GetMapPixelPosition 仅加 mapBounds 原点。NPC `0x18F0510` 为 `mov w0,#1; ret`，invoker `0x74A96E8` 从虚槽 `0x110` 调用。MapPage.draw 的 `0x173C64C..708` 明确区分带地图矩阵的 drawMiniPortraits 与恢复批次后的 drawTooltip。

旧方案/失败尝试：不继续使用旧整页 draw 后追加首字的方案。离线提取元数据时，按“前一 ret 后即 factory”猜边界命中了 invoker，工具拒绝外部代码 `0x19C1890`；随后读取真实序言和分配调用找到 factory 后才重新提取。没有放宽工具白名单，没有真机试错记录。

修复：仅在 NPC 地图模块建立独立遍历器，用虚调用筛选村民，另纳入马/孩子；不顺带修改共享 VisitVillagers 或生日HUD等旧消费者。地图范围乘四，坐标不二次缩放。原地图Hook移至 drawMiniPortraits，新增 drawTooltip Hook 保证人物说明层次；无小地图、无输入Hook、无额外Tick。头像与类型元数据、任务具体集合/目标字段、商人日历均有离线证据。全局缓存仅保存原生复制值，托管对象/贴图在每次回调重取。

验证：43处16字节函数/消费者指纹、ARM64编译期边界测试及既有UI/钓鱼测试通过；本地正常构建、NSO三段解压哈希、NPDM/staging一致性、SDK导入与v12.1相同。已有源码仅改 npc_map.cpp 和版本字符串，新增两个地图头文件，其他253个已有源码相同。最终ELF池0x2000、步长200、严格索引上界39，预算32/40余8。**真机头像、缩放、剧情NPC覆盖、性能与旧功能回归未执行。**共享 VisitVillagers 的基类调用问题在其他消费者仍开放，本次不宣称整个UI模块都已修复。

规则：地图单位必须同时追踪位置输出、边界尺寸和绘制变换；虚方法按具体派发，不直接调用基类恒值实现；原版地图上叠加头像和提示要分别核对批次/变换阶段。元数据factory边界以真实序言/分配/构造链为准，不依赖前一ret启发式。新增Hook继续执行全包预算，当前8槽余量已到项目最低阈值，下次新增必须重新评估。

## 问题36：查询界面接入共享菜单、组合键和托管枚举器（2026-09-17）

现象/任务：用户要求将 LookupAnything 纳入既有整合包，选择单人完整分类、允许未发现信息、指向查询与分类浏览、ZL+R3、B返回和暂停。此条记录本地设计与实现证据，没有新增真机成功反馈。

关键证据：当前Hook池32/40且最低余量8，故复用现有GameMenu的7个菜单Hook及合并Tick。GameMenu(bool) body `16EBF20` / invoker `73BF3F8` 是 x0=this、w1=bool，null-this 在 `16EC998` 分配；菜单交给Game1.activeClickableMenu持有。单人 `shouldTimePass` 的活动菜单分支 `13B08B8..13B0948` 对普通菜单返回false。既有菜单关闭流程负责世界恢复，不保存旧托管菜单裸指针。

输入证据：Buttons元数据常量实际值为LeftTrigger=0x800000、R3=0x80、L3=0x40。旧UI设置页将触发器用于翻页，查询必须先消费自身输入。两键都松开才能重新触发；ZL+L3+R3不触发两种功能；断线后保持锁存直到释放。查询初始化失败时，其门控不影响自动钓鱼。

数据/ABI：农场动物 ValuesCollection 是含owner的8字节值，GetEnumerator `413E6AC` 通过x8返回56字节；MoveNext `39F50DC` 使用owner虚槽210解包。字符串字典与地点字典均有各自32字节枚举器、x0/x1键值对返回证据，不能把接口名称当作具体数组布局。物品目录沿BaseItemDataDefinition标识槽40/GetAllIds槽60及已验证List<string>构造器读取数据，不创建预览物品，避免随机品质或特殊构造副作用。所有跨帧状态均为原生字符、数值或布尔值。

静态发现及修正：首轮指向搜索先截取128条列表，会漏掉密集农田后部对象；改为按光标过滤后再分页。建筑不能只匹配左上角，改为读取tilesWide/tilesHigh覆盖完整占地。新查询期间跳过自动钓鱼/自动化/电梯的附加Tick；关闭的同一Tick也跳过，下一Tick恢复。工具/钓鱼/进食期间的请求等待当前事务结束，自动钓鱼等待查询时不再开始下一竿或新进食。初步本地构建的C479...、4C04...为中间产物，最终候选为首页E885...；不能混用身份。

验证：69个入口/消费者指纹、组合键/分页/配方分段/UTF-16边界编译期测试通过，原UI、自动钓鱼及NPC地图测试通过。最终ARM64正常构建通过；ELF Hook池0x2000、步长200、严格索引<=39；SDK导入与v13一致；NSO三段解压和哈希及staging一致性通过。源码仅改10个既有文件，新增lookup目录4个文件，没有删除已有文件；抛竿与完美/铱品质Hook源码块未改。`tools/package_lookup.py` 还检查旧包源码身份、Hook安装表达式、指纹与已安装入口的重叠、ZIP CRC和源码/产物清单。

开放范围：真机启动、操作映射、菜单渲染、耗时、动态谜题、查询中保存/读档边界和组合功能回归均未执行。详情字段、原文条件、PC扩展差异见LOOKUP_ANYTHING_SWITCH功能账本；显示原始条件不代表已经模拟捕获概率或判断所有条件成立。沿用v13既有开放风险。当前结论是本地候选通过，不是绝对无冲突或真机稳定声明。

规则增补：新增功能优先复用有单一入口的分派Hook；菜单状态必须分清等待事务、显示、关闭当帧和恢复四个阶段。列表的显示上限不能影响指向命中范围。值类型集合/枚举器的this指针、x8结构返回与键值对寄存器必须分别核验。功能账本明确实现字段与未覆盖的PC扩展，交付保持旧包可回退。

## 问题37：FastAnimations 的对象计时、ABI 与 v13 基线恢复（2026-09-17）

任务与基线：用户作废 Lookup v14，指定以 v13 整合 Fast Animations 1.16.0，包含武器/弹弓，默认2倍，可关闭或选3倍，跳过吃喝确认。先核验两个历史包全部源码清单并保存 v14 ELF，再恢复257个完全匹配的 v13 文件；后续仅修改 Tick、版本和两个设置文件，新增动画模块4文件，253个原文件保持一致。没有新增 Hook，没有改动钓鱼、NPC地图等旧模块实现。

证据：PC字段名称不等于 Switch ABI。AnimationFrame 是40字节结构，通过 x8 返回；树倒下字段是 NetBool，箱子帧计时是 NetInt，ShippingMenu 的私有字段需结合消费者确认。59个函数入口指纹及相关 invoker/body 反汇编保留在 analysis/fastanimations-*。TemporaryAnimatedSprite 可以直接保存 Texture 对象而不填 textureName，因此最终实现通过 ParsedItemData.GetTexture 0x1521930 与 TemporaryAnimatedSprite.get_Texture 0x1A45A60 比较对象和源矩形，不能只比较贴图名称。

静态发现与修正：早期本地候选的贴图名称匹配不能覆盖直接贴图构造，已在交付前修正并重新构建；9F65DCDA... 是已被替代的中间产物。未进行真机试错。拒绝整体重复 GameTick 或加速所有 DelayedAction 的方案：只增加目标对象更新，马笛仅缩短已验证缓存 delegate E283768 对应的延迟任务。BobberBar 仅推进 SparklingText，不能重复运行结算更新。动作或地点切换后停止额外更新，完成的目标粒子从仍持有它的原列表移除，避免重复回调。

设置迁移：保留原29位功能设置，以最高位31记录新配置格式，旧掩码首次默认启用位29、关闭3倍位30；新配置显式关闭后不在读档时重新打开。配置原有版本、校验和及双槽结构不变。L3+R3仍只用于钓鱼。

验证：配置迁移/计时边界的 ARM64 编译期测试、原 UI/钓鱼/NPC 静态检查通过；最终正常 ARM64 构建通过。NSO SHA-256 EB390AAE9F260E2A9CDD4A633F1D81DA6FE6746460E435BDF17D3F26C91C15BE，Module ID 61929B058A6B1289D5A8214A7FA3064F9FF35759；三段解压及哈希、build/staging 一致性通过。Hook预算32/40余8；打包脚本再次核验地址重叠、SDK导入、ELF池大小/步长/严格边界、源码清单和ZIP CRC。

开放范围：真机启动、画面、性能与所有组合回归未执行。原版对象更新会同时推进该对象其他帧计时，尤其战斗与交通需要真机回归；部分独立粒子和特殊染色层保持原速，不能宣称PC全部视觉效果等价。遍历上限与大农场性能、v13既有保存/集合风险继续开放。完整范围和验收矩阵见 docs/FAST_ANIMATIONS_SWITCH.md。

规则：加速必须限定到具体对象和明确事务阶段；不要通过全局时间或延迟队列连带推进无关功能。贴图识别沿用真实对象构造/消费者证据，不能仅凭可空名称。静态互斥、构建、产物完整性和真机运行是四种独立证据。

## 问题38：黑幕 alpha 被当作渲染透明度钳制，导致出门黑屏（2026-09-17）

现象：用户安装上一轮候选后报告“出了门之后黑屏”。未提供此次崩溃日志或运行状态细节，故不把静态复现扩张为排除一切其他故障。失败候选 v13.1 的 NSO 为 EB390AAE...，Module ID 61929B05...；其完整包、源码、ELF仍保留。重建前核对备份 ELF 与当时 build 的 SHA 完全一致：E4F067EA6DAC8733D5D2D1ED53599A87DC5A79977BE6617E5C6165CD8F84EC49。

证据与根因：Game1.updatePause 在 0x13B3C98 调用 ScreenFade.Update 0x11367B0。原版 0x1136810..14 比较 rodata 0xAC76FC0 的 float 1.1，只有严格超过才进入 0x1136828 的黑幕回调；0x1136874..78 比较 0xAC77D7C 的 -0.1，只有严格低于才进入 0x113687C 的清除/完成回调。比较发生在原版 alpha 推进之前。v13.1 新模块在 Orig Tick 后把 alpha 限制到 [0,1]，因此下帧永远达不到两个阈值。函数指纹、ABI、编译、压缩段校验均无法检测这种时序逻辑错误。PC FadeHandler 没有该钳制；这属于本次移植自行引入的错误。

失败方案：v13.1 的“先夹到显示范围，再由原版处理完成”不可行；不得改为强制清除黑幕或直接调用传送来掩盖错误，这会绕过原版生命周期。新增世界对象扫描最初也被列为排查假设；目前没有其导致本次黑屏的证据，修复不改动这部分。

修复：v13.1.1 用 AdvanceFade 仅加减指定步长，允许状态 alpha 跨过阈值，保留原版更新方向、门控和完成回调。不新增 Hook，不重复执行整个游戏更新或主动调用传送。相对失败包仅修改 fastanimations.cpp、logic.hpp 和 project_metadata.hpp，其余258个源码相同。

验证：tools/test_fastanimations_fade_aot.py 加载哈希锁定的原版 text/rodata，在隔离 ARM64 模拟器执行真实比较分支，遇到回调入口即停，不执行回调。6个边界结果与严格比较一致；1/8/16/17/33/50/100ms、1/2/3倍、两个方向共42组逐帧模型，旧加速模式28组均无法完成，修复后42组均到达完成入口。C++编译期回归直接调用修复函数覆盖跨阈值及保持已越界值。原配置/UI/钓鱼/NPC检查通过，正常ARM64构建、最终NSO三段解压/哈希、build/staging一致性通过，Hook仍32/40余8。最终身份见当前P0。打包进一步核验旧包身份、源码差异和ZIP内容。

开放验证：真机修复后出门/进门、连续切图、2倍/3倍/关闭对比及组合回归未执行；音频/按键是否响应、实际故障日志待用户补充。现有结论是代码缺陷已离线复现并修正，不是新包真机成功。

规则：字段看似透明度/进度也可能承载状态机哨兵值。任何新增 clamp、归零或截断，都必须核对原版完成阈值与回调前后顺序；不能只核验生产值的函数入口。保持旧失败产物，以真实消费者分支做回归，不用仅与实现同义的测试代替。
