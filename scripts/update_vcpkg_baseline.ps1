#requires -Version 5.1
param(
  [string]$VcpkgRoot = $env:VCPKG_ROOT,
  [string]$ManifestPath = "$PSScriptRoot\..\vcpkg.json"
)
$ErrorActionPreference = 'Stop'

if (-not (Test-Path "$VcpkgRoot\.git")) { throw "VCPKG_ROOT is not a git repo: $VcpkgRoot" }
$git = Get-Command git -ErrorAction SilentlyContinue
if (-not $git) { throw "git not found" }

Push-Location $VcpkgRoot
$sha = (git rev-parse HEAD).Trim()
Pop-Location

if ($sha.Length -ne 40) { throw "Invalid baseline commit: $sha" }
if (-not (Test-Path $ManifestPath)) { throw "vcpkg.json not found: $ManifestPath" }

$json = Get-Content $ManifestPath -Raw | ConvertFrom-Json
# Add or update builtin-baseline
$json | Add-Member -NotePropertyName 'builtin-baseline' -NotePropertyValue $sha -Force
($json | ConvertTo-Json -Depth 10) | Set-Content -Path $ManifestPath -Encoding utf8

Write-Host "[BASELINE] builtin-baseline -> $sha"
