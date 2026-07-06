param([string]$Root)

if (-not $Root) { $Root = "$PSScriptRoot\.." }
$Root     = (Resolve-Path $Root).Path
$Exe      = Join-Path $Root "bin\rechan.exe"
$ResDir   = Join-Path $Root "res"
$OutDir   = Join-Path $Root "shipping"
$OutZip   = Join-Path $OutDir "rechan-win64.zip"
$TmpDir   = Join-Path $env:TEMP "rechan-win64-pack"
$Stage    = Join-Path $TmpDir "rechan-win64"

if (Test-Path $TmpDir) { Remove-Item $TmpDir -Recurse -Force }
New-Item -ItemType Directory -Force $Stage | Out-Null
New-Item -ItemType Directory -Force $OutDir | Out-Null

$hash = (Get-FileHash $Exe -Algorithm SHA256).Hash.ToLower()
"$hash  rechan.exe" | Set-Content -Encoding UTF8 (Join-Path $Stage "SHA256SUM.txt")

Copy-Item $Exe (Join-Path $Stage "rechan.exe")
Copy-Item (Join-Path $ResDir "pc") (Join-Path $Stage "pc") -Recurse

if (Test-Path $OutZip) { Remove-Item $OutZip }
Compress-Archive -Path "$Stage\*" -DestinationPath $OutZip

Remove-Item $TmpDir -Recurse -Force
Write-Host "Packed: $OutZip"
