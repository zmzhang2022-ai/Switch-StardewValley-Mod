# Patches only fixed-length provenance strings in the verified v9 NSO.
# It refuses to touch a binary whose expected source strings are absent.
$ErrorActionPreference = 'Stop'

$targets = @(
    'D:\Switch_StardewVally\atmosphere\contents\0100E65002BB8000\exefs\subsdk9',
    'D:\Switch_StardewVally\deploy\automate-skull-rings4-woodpath-fishpond-layout2-v9-preserves-geodecrusher-performance\subsdk9',
    'D:\Switch_StardewVally\runtime\build-clang\subsdk9'
)

$patches = @(
    @{ Offset = 40649; Needle = 'svcUnmapProcessMemory(rw, procHandle, ro, size)'; Replacement = 'AutomateLite v9|Copyright 2026 zmzhang2022-ai' },
    @{ Offset = 40947; Needle = 'D:\Switch_StardewVally\runtime/source\lib/util/sys/mem_layout.hppv'; Replacement = 'https://github.com/zmzhang2022-ai/Switch-StardewValley-Mod' }
)

foreach ($path in $targets) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing target: $path" }
    $bytes = [IO.File]::ReadAllBytes($path)
    foreach ($patch in $patches) {
        $needle = [Text.Encoding]::ASCII.GetBytes($patch.Needle)
        $replacement = [Text.Encoding]::ASCII.GetBytes($patch.Replacement)
        if ($replacement.Length -gt $needle.Length) { throw "Replacement is too long at offset $($patch.Offset)" }
        for ($i = 0; $i -lt $needle.Length; $i++) {
            if ($bytes[$patch.Offset + $i] -ne $needle[$i]) {
                throw "Verified needle mismatch at offset $($patch.Offset) in $path"
            }
        }
        for ($i = 0; $i -lt $needle.Length; $i++) {
            $bytes[$patch.Offset + $i] = if ($i -lt $replacement.Length) { $replacement[$i] } else { 0 }
        }
    }
    [IO.File]::WriteAllBytes($path, $bytes)
    Write-Output "Patched $path"
}
