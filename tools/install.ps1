<#
.SYNOPSIS
    FLA++ (FLACompatBridge) one-step installer for GTA San Andreas 1.0 US.

.DESCRIPTION
    Copies FLACompatBridge.asi into <game>\scripts\, backs up any existing build,
    and tells you how to verify the install. Run this with the game closed.

.PARAMETER GameDirectory
    GTA San Andreas folder containing gta_sa.exe. If omitted, the script looks
    next to itself first (extract the zip into the game folder), then a few
    common install locations.

.PARAMETER AsiPath
    Path to FLACompatBridge.asi. Defaults to the copy next to this script.

.PARAMETER Force
    Install even if the game process is currently running.
#>
[CmdletBinding()]
param(
    [string]$GameDirectory,
    [string]$AsiPath,
    [switch]$Force
)

$ErrorActionPreference = 'Stop'

function Write-Step { param([string]$Text) Write-Host "==> $Text" -ForegroundColor Cyan }
function Write-Ok   { param([string]$Text) Write-Host "    $Text" -ForegroundColor Green }
function Write-Warn2{ param([string]$Text) Write-Host "    $Text" -ForegroundColor Yellow }

function Resolve-GameDirectory {
    param([string]$Explicit)

    $candidates = New-Object System.Collections.Generic.List[string]
    if ($Explicit) { $candidates.Add($Explicit) }
    if ($PSScriptRoot) { $candidates.Add($PSScriptRoot) }
    $candidates.Add((Join-Path (Get-Location) '.'))

    foreach ($drive in @('C:', 'D:', 'E:', 'F:')) {
        $candidates.Add("$drive\Program Files (x86)\Rockstar Games\GTA San Andreas")
        $candidates.Add("$drive\Program Files\Rockstar Games\GTA San Andreas")
        $candidates.Add("$drive\Games\GTA San Andreas")
        $candidates.Add("$drive\GTA San Andreas")
        $candidates.Add("$drive\SteamLibrary\steamapps\common\Grand Theft Auto San Andreas")
    }

    foreach ($candidate in $candidates) {
        if (-not $candidate) { continue }
        try { $full = [IO.Path]::GetFullPath($candidate) } catch { continue }
        if (Test-Path -LiteralPath (Join-Path $full 'gta_sa.exe') -PathType Leaf) { return $full }
    }
    return $null
}

Write-Step 'FLA++ installer'

$running = Get-Process -Name 'gta_sa' -ErrorAction SilentlyContinue
if ($running -and -not $Force) {
    Write-Host 'GTA San Andreas is running. Close the game first, then run this again.' -ForegroundColor Red
    Write-Host 'Never replace an ASI while the game is running.' -ForegroundColor Red
    exit 1
}

$game = Resolve-GameDirectory -Explicit $GameDirectory
if (-not $game) {
    Write-Host 'Could not find gta_sa.exe.' -ForegroundColor Red
    Write-Host 'Extract this zip into your GTA San Andreas folder and run install.bat,' -ForegroundColor Yellow
    Write-Host 'or run:  .\install.ps1 -GameDirectory "D:\Games\GTA San Andreas"' -ForegroundColor Yellow
    exit 1
}
Write-Ok "Game: $game"

if (-not $AsiPath) { $AsiPath = Join-Path $PSScriptRoot 'FLACompatBridge.asi' }
if (-not (Test-Path -LiteralPath $AsiPath -PathType Leaf)) {
    Write-Host "FLACompatBridge.asi not found at: $AsiPath" -ForegroundColor Red
    exit 1
}
$asiFull = (Resolve-Path -LiteralPath $AsiPath).Path
Write-Ok "ASI : $asiFull"

$scripts = Join-Path $game 'scripts'
if (-not (Test-Path -LiteralPath $scripts -PathType Container)) {
    New-Item -ItemType Directory -Path $scripts -Force | Out-Null
    Write-Ok 'Created scripts\ folder'
}

$target = Join-Path $scripts 'FLACompatBridge.asi'
if (Test-Path -LiteralPath $target -PathType Leaf) {
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $backup = "$target.$stamp.bak"
    Copy-Item -LiteralPath $target -Destination $backup -Force
    Write-Ok "Backed up existing ASI -> $(Split-Path -Leaf $backup)"
}

Copy-Item -LiteralPath $asiFull -Destination $target -Force
Write-Ok 'Installed scripts\FLACompatBridge.asi'

$ini = Join-Path $scripts 'FLACompatBridge.ini'
if (Test-Path -LiteralPath $ini -PathType Leaf) {
    Write-Warn2 'An existing FLACompatBridge.ini was found and left in place.'
    Write-Warn2 'If you upgraded from an older version and something misbehaves, delete it'
    Write-Warn2 'and let the bridge write a fresh one on the next launch.'
}

Write-Step 'Requirements'
Write-Ok 'GTA SA 1.0 US (HOODLUM) + fastman92 Limit Adjuster 7.6 + CLEO 5.4 / CLEO+'
Write-Ok 'v1.11 and newer also require SA-RenderStack v3.2 for the ProperShaders guard'

Write-Step 'Next'
Write-Ok '1. Launch the game once.'
Write-Ok "2. Check that $(Join-Path $scripts 'FLACompatBridge.log') exists and has content."
Write-Ok '3. FLACompatBridge.ini is generated automatically on first run - do not copy one from an older version.'
Write-Ok 'To uninstall: delete FLACompatBridge.asi, .ini and .log from scripts\.'
