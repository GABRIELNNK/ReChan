param([string]$Root)

if (-not $Root) { $Root = "$PSScriptRoot\.." }
$Root     = (Resolve-Path $Root).Path
$Nro      = Join-Path $Root "switch\rechan\rechan.nro"
$ResDir   = Join-Path $Root "res"
$OutDir   = Join-Path $Root "shipping"
$OutZip   = Join-Path $OutDir "rechan-switch.zip"
$TmpDir   = Join-Path $env:TEMP "rechan-switch-pack"
# Staged to mirror the SD card layout (switch/rechan/...), so the zip can be
# extracted straight onto an SD card's root -- matches the sdmc:/switch/rechan
# working directory the game chdir()s into on boot (see src/gen/main.cpp).
$Stage    = Join-Path $TmpDir "switch\rechan"

if (-not (Test-Path $Nro)) {
    Write-Error "Not found: $Nro -- build the release .nro first (switch\build.sh release or switch\build.cmd release)."
    exit 1
}

if (Test-Path $TmpDir) { Remove-Item $TmpDir -Recurse -Force }
New-Item -ItemType Directory -Force $Stage | Out-Null
New-Item -ItemType Directory -Force $OutDir | Out-Null

$hash = (Get-FileHash $Nro -Algorithm SHA256).Hash.ToLower()
"$hash  rechan.nro" | Set-Content -Encoding UTF8 (Join-Path $Stage "SHA256SUM.txt")

Copy-Item $Nro (Join-Path $Stage "rechan.nro")
Copy-Item (Join-Path $ResDir "pc") (Join-Path $Stage "pc") -Recurse

if (Test-Path $OutZip) { Remove-Item $OutZip }
Compress-Archive -Path "$TmpDir\switch" -DestinationPath $OutZip

Remove-Item $TmpDir -Recurse -Force
Write-Host "Packed: $OutZip"
Write-Host "Extract onto an SD card root so it lands at switch\rechan\rechan.nro."
Write-Host "Note: does not include game assets -- point the app at your own legally-owned disc image (see discimage/ under sdmc:/switch/rechan/) on first run."
