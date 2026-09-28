$ErrorActionPreference='Stop'
$project=Split-Path -Parent $PSScriptRoot
$cxx='C:\devkitPro\devkitARM\arm-none-eabi\include\c++\13.1.0'
& 'C:\Program Files\LLVM\bin\clang++.exe' --target=aarch64-none-elf -std=gnu++23 -fsyntax-only -nostdinc++ `
 -isystem $cxx -isystem "$project/toolchains/compat" -isystem "$cxx/arm-none-eabi" `
 -isystem "$project/toolchains/root/opt/devkitpro/devkitA64/aarch64-none-elf/include" `
 -I "$project/runtime/source" "$project/tests/npcmap_logic_test.cpp"
if($LASTEXITCODE -ne 0) { throw 'NPC map boundary tests failed' }
& python "$project/tools/audit_npcmap.py"
if($LASTEXITCODE -ne 0) { throw 'NPC map fingerprint audit failed' }
& "$project/tools/test_fishing.ps1"
Write-Output 'NPC map boundaries, tooltip fit, fingerprints and existing UI/fishing compile-time checks passed. Hardware not executed.'
