param()
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
$clang = 'C:\Program Files\LLVM\bin\clang++.exe'
$cxx = 'C:\devkitPro\devkitARM\arm-none-eabi\include\c++\13.1.0'
$arguments = @(
    '--target=aarch64-none-elf', '-std=gnu++23', '-fsyntax-only', '-nostdinc++',
    '-isystem', $cxx,
    '-isystem', (Join-Path $project 'toolchains/compat'),
    '-isystem', (Join-Path $cxx 'arm-none-eabi'),
    '-isystem', (Join-Path $project 'toolchains/root/opt/devkitpro/devkitA64/aarch64-none-elf/include'),
    '-I', (Join-Path $project 'runtime/source'),
    (Join-Path $project 'tests/uiinfo_text_buffer_test.cpp'),
    (Join-Path $project 'tests/uiinfo_logic_test.cpp'),
    (Join-Path $project 'tests/hook_pool_test.cpp')
)
& $clang @arguments
if ($LASTEXITCODE -ne 0) { throw 'UIInfo compile-time regression tests failed' }
Write-Output 'Passed ARM64 compile-time tests: text, XP, crop days, aging, seasons, range edges, config integrity, trampoline pool boundaries.'
Write-Output 'This does not execute the game, SDK file operations, or Switch rendering.'
