$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
$cxx = 'C:\devkitPro\devkitARM\arm-none-eabi\include\c++\13.1.0'
& 'C:\Program Files\LLVM\bin\clang++.exe' --target=aarch64-none-elf -std=gnu++23 -fsyntax-only -nostdinc++ `
    -isystem $cxx -isystem "$project/toolchains/compat" -isystem "$cxx/arm-none-eabi" `
    -isystem "$project/toolchains/root/opt/devkitpro/devkitA64/aarch64-none-elf/include" `
    -I "$project/runtime/source" "$project/tests/fishing_logic_test.cpp"
if ($LASTEXITCODE -ne 0) { throw 'Fishing compile-time tests failed' }
& python "$project/tools/audit_fishing.py"
if ($LASTEXITCODE -ne 0) { throw 'Fishing fingerprints failed' }
& "$project/tools/test_uiinfo.ps1"
Write-Output 'Fishing chord/release, stamina/time thresholds and food ranking compile-time tests passed. Game execution not performed.'
