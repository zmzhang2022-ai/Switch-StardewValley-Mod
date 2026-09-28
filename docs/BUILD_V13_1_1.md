# v13.1.1 源码与构建

`runtime/source` 与已交付修复包 BUILD_INFO.json 的261个源码 SHA-256 一一对应。完整安装包另含匹配 ELF、审计记录和源码快照。

实际构建入口：Windows PowerShell `tools/build_poc_clang.ps1`，输出 `runtime/build-clang/subsdk9-normal` 和 staging 的 subsdk9/main.npdm。脚本当前使用项目既有 LLVM/devkitPro 安装路径；首次搭建须安装工具链并按脚本配置路径。仓库不附带工具链二进制。

构建所需原版 NPDM 与离线审计所需游戏 main/text/rodata 需从自己的目标版本合法取得，仓库不包含这些游戏文件。`tools/test_fastanimations.ps1` 包含 ARM64 编译期检查；原版指纹/模拟器验证还需要本地分析缓存和 Python capstone/unicorn 等依赖。发布记录中的成功检查来自原开发工作区，不表示新检出可无依赖直接运行全部离线审计。

`tools/package_fastanimations.py` 是原工作区交付脚本，需要本地 v13/v13.1 基线包及分析证据；不能在空白检出中直接重打包。当前发布安装包保持原始字节与 SHA-256，未二次修改 NSO。

默认2倍/可选3倍/关闭，修复后真机复测未完成。更多限制见 FAST_ANIMATIONS_SWITCH.md 与 PROJECT_LESSONS.md。
