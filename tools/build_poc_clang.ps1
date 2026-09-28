param(
    [switch]$FatalProof,
    [switch]$Phase8Trace,
    [switch]$Phase8Audit
)

$ErrorActionPreference = 'Stop'
$workspace = Split-Path -Parent $PSScriptRoot
$runtime = Join-Path $workspace 'runtime'
$build = Join-Path $runtime 'build-clang'
$llvm = 'C:\Program Files\LLVM\bin'
$clang = Join-Path $llvm 'clang.exe'
$clangxx = Join-Path $llvm 'clang++.exe'
$dkpRoot = Join-Path $workspace 'toolchains\root\opt\devkitpro'
$a64 = Join-Path $dkpRoot 'devkitA64'
$a64Include = Join-Path $a64 'aarch64-none-elf\include'
$gccCxx = 'C:\devkitPro\devkitARM\arm-none-eabi\include\c++\13.1.0'
$compat = Join-Path $workspace 'toolchains\compat'
$elf2nso = Join-Path $dkpRoot 'tools\bin\elf2nso.exe'
$npdmPatcher = Join-Path $workspace 'tools\patch_npdm_syscalls.py'
$originalNpdm = Join-Path $workspace 'exefs\main.npdm'
$sysbase = Join-Path $a64 'aarch64-none-elf\lib\pic\libsysbase.a'
$elf = Join-Path $build 'automate_lite.elf'
$nso = Join-Path $build 'subsdk9'
$npdm = Join-Path $build 'main.npdm'
$buildLabel = if ($Phase8Audit) { 'phase8-audit' } elseif ($FatalProof -and $Phase8Trace) { 'fatal-proof-phase8-trace' } elseif ($FatalProof) { 'fatal-proof' } elseif ($Phase8Trace) { 'phase8-trace' } else { 'normal' }
$variantName = "subsdk9-$buildLabel"
$variantNso = Join-Path $build $variantName
$variantElf = Join-Path $build ("automate_lite-{0}.elf" -f $buildLabel)
$deploy = Join-Path $workspace 'atmosphere\contents\0100E65002BB8000\exefs'
$variantDeployName = "phase8-$buildLabel"
$variantDeploy = Join-Path $workspace "deploy\$variantDeployName\atmosphere\contents\0100E65002BB8000\exefs"

foreach ($required in @($clang, $clangxx, $gccCxx, $compat, $elf2nso, $sysbase, $npdmPatcher, $originalNpdm)) {
    if (-not (Test-Path $required)) {
        throw "Required build dependency is missing: $required"
    }
}

# Check the merged hook inventory before overwriting any previous build output.
& python (Join-Path $workspace 'tools/check_hook_budget.py') --output (Join-Path $workspace 'analysis/uiinfo-hook-budget.json')
if ($LASTEXITCODE -ne 0) { throw 'Trampoline pool budget check failed.' }

New-Item -ItemType Directory -Force $build, $deploy, $variantDeploy | Out-Null

# Never link stale flat-name objects from builds made before the relative-path
# object naming scheme below. These are disposable compiler outputs confined
# to runtime/build-clang.
Get-ChildItem $build -File -Filter '*.o' | ForEach-Object {
    Remove-Item -LiteralPath $_.FullName -Force
}

$common = @(
    '--target=aarch64-none-elf',
    '-march=armv8-a+crc+crypto',
    '-fPIC',
    '-fvisibility=default',
    '-ffunction-sections',
    '-fdata-sections',
    '-D__SWITCH__',
    '-D__RTLD_6XX__',
    '-DEXL_LOAD_KIND=Module',
    '-DEXL_LOAD_KIND_ENUM=2',
    '-DEXL_PROGRAM_ID=0x0100E65002BB8000',
    '-DEXL_USE_FAKEHEAP',
    "-I$runtime/source",
    "-I$runtime/source/lib",
    "-I$runtime/source/nn",
    "-I$runtime/source/program",
    "-I$runtime/source/rtld",
    "-I$runtime/source/hooks",
    "-I$runtime/source/automate",
    "-I$runtime/source/game"
)
if ($FatalProof) {
    $common += '-DAUTOMATE_LITE_PHASE4_FATAL_PROOF'
}
if ($Phase8Trace) {
    $common += '-DAUTOMATE_LITE_PHASE8_TRACE'
}
if ($Phase8Audit) {
    $common += '-DAUTOMATE_LITE_PHASE8_AUDIT'
}

$systemIncludes = @('-isystem', $a64Include)
$cxx = @(
    '-std=gnu++23',
    '-fno-rtti',
    '-fno-exceptions',
    '-fno-asynchronous-unwind-tables',
    '-fno-unwind-tables',
    '-nostdinc++',
    '-isystem', $gccCxx,
    '-isystem', $compat,
    '-isystem', (Join-Path $gccCxx 'arm-none-eabi')
)

$sources = Get-ChildItem (Join-Path $runtime 'source') -Recurse -File |
    Where-Object Extension -In '.cpp', '.c', '.s'
$sourceRoot = (Join-Path $runtime 'source').TrimEnd('\', '/')

foreach ($source in $sources) {
    # Source folders can legitimately contain files with the same base name
    # (for example elevator/skull_cavern_elevator.cpp and its hooks peer).
    # A flat BaseName.o path silently overwrote the first object and could
    # leave unresolved symbols in the shared ELF. Include the relative source
    # path in every object name so the link always receives both units.
    $relativeSource = $source.FullName.Substring($sourceRoot.Length).TrimStart('\', '/')
    $objectName = ($relativeSource -replace '[\\/:]', '_') + '.o'
    $object = Join-Path $build $objectName
    if ($source.Extension -eq '.cpp') {
        & $clangxx @common @cxx @systemIncludes -O2 -c $source.FullName -o $object
    } else {
        & $clang @common @systemIncludes -O2 -c $source.FullName -o $object
    }
    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed: $($source.FullName)"
    }
}

$objects = Get-ChildItem (Join-Path $build '*.o') | ForEach-Object FullName
$linkArgs = @(
    '--target=aarch64-none-elf',
    '-fuse-ld=lld',
    '-nostdlib',
    '-Wl,-shared',
    "-Wl,-T,$runtime/misc/link.ld",
    '-Wl,--gc-sections',
    '-Wl,--build-id=sha1',
    '-Wl,--export-dynamic',
    '-Wl,-init=exl_module_init',
    '-Wl,-soname,subsdk9',
    "-Wl,-Map,$build/automate_lite.map",
    '-o', $elf
)

& $clangxx @linkArgs $objects $sysbase
if ($LASTEXITCODE -ne 0) {
    throw 'Link failed'
}

& $elf2nso $elf $nso
if ($LASTEXITCODE -ne 0) {
    throw 'elf2nso failed'
}

& python $npdmPatcher $originalNpdm $npdm
if ($LASTEXITCODE -ne 0) {
    throw 'NPDM patch failed'
}

Copy-Item $elf $variantElf -Force
Copy-Item $nso $variantNso -Force
Copy-Item $nso (Join-Path $deploy 'subsdk9') -Force
Copy-Item $npdm (Join-Path $deploy 'main.npdm') -Force
Copy-Item $nso (Join-Path $variantDeploy 'subsdk9') -Force
Copy-Item $npdm (Join-Path $variantDeploy 'main.npdm') -Force
$hash = Get-FileHash $nso -Algorithm SHA256
Write-Output "Built: $nso"
Write-Output "Saved symbols: $variantElf"
Write-Output "Saved variant: $variantNso"
Write-Output "Deployed staging copy: $(Join-Path $deploy 'subsdk9')"
Write-Output "Deployed NPDM overlay: $(Join-Path $deploy 'main.npdm')"
Write-Output "SHA-256: $($hash.Hash)"
Write-Output "Fatal proof: $FatalProof"
Write-Output "Phase 8 trace: $Phase8Trace"
Write-Output "Phase 8 audit: $Phase8Audit"

