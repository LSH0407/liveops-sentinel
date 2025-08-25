Param(
  [switch]$WhatIfOnly
)

Write-Host "=== Clean liveops-sentinel workspace ==="

$paths = @(
  "build", "build_*", "out", "out_*",
  ".vs", "x64", "ipch",
  "Release", "Debug",
  "CMakeCache.txt", "CMakeFiles",
  "Testing", "_deps",
  "vcpkg_installed", "CMakeUserPresets.json"
)

foreach ($p in $paths) {
  Get-ChildItem -LiteralPath . -Filter $p -Force -ErrorAction SilentlyContinue |
    ForEach-Object {
      if ($WhatIfOnly) {
        Write-Host "[WHATIF] would remove $($_.FullName)"
      } else {
        try {
          Remove-Item $_.FullName -Recurse -Force -ErrorAction Stop
          Write-Host "[OK] removed $($_.FullName)"
        } catch {
          Write-Warning "[SKIP] $($_.FullName) - $($_.Exception.Message)"
        }
      }
    }
}

# 빌드 내부 캐시 흔적 추가 제거
Get-ChildItem -Path . -Recurse -Include "CMakeCache.txt","CMakeFiles","*.vcxproj.user" -ErrorAction SilentlyContinue |
  ForEach-Object {
    if ($WhatIfOnly) { Write-Host "[WHATIF] would remove $($_.FullName)" }
    else {
      try { Remove-Item $_.FullName -Recurse -Force -ErrorAction Stop; Write-Host "[OK] removed $($_.FullName)" }
      catch { Write-Warning "[SKIP] $($_.FullName) - $($_.Exception.Message)" }
    }
  }

Write-Host "=== Done ==="
