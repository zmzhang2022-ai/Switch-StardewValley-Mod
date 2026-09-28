$ErrorActionPreference='Stop'
$project=Split-Path -Parent $PSScriptRoot
$cxx='C:\devkitPro\devkitARM\arm-none-eabi\include\c++\13.1.0'
& 'C:\Program Files\LLVM\bin\clang++.exe' --target=aarch64-none-elf -std=gnu++23 -fsyntax-only -nostdinc++ `
 -isystem $cxx -isystem "$project/toolchains/compat" -isystem "$cxx/arm-none-eabi" `
 -isystem "$project/toolchains/root/opt/devkitpro/devkitA64/aarch64-none-elf/include" `
 -I "$project/runtime/source" "$project/tests/fastanimations_logic_test.cpp"
if($LASTEXITCODE -ne 0) { throw 'FastAnimations migration/timing tests failed' }
& python "$project/tools/audit_fastanimations.py"
if($LASTEXITCODE -ne 0) { throw 'FastAnimations source/fingerprint audit failed' }
& python "$project/tools/test_fastanimations_fade_aot.py"
if($LASTEXITCODE -ne 0) { throw 'Original ARM64 fade threshold regression failed' }
& "$project/tools/test_npcmap.ps1"
Write-Output 'FastAnimations config migration/timing, v13 source, NPC map, fishing and UI static checks passed. Hardware not executed.'
