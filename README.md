# FLA++ / FLACompatBridge

A compatibility and crash-mitigation bridge for **GTA San Andreas** when running under [fastman92's Limit Adjuster (FLA)](https://github.com/fastman92/fastman92_limit_adjuster) with heavily expanded pools, relocated tables, and high model IDs.

> **Runtime filename:** `FLACompatBridge.asi`  
> **Config file:** `FLACompatBridge.ini`

---

## What problem does this solve?

When FLA expands the game beyond vanilla limits (e.g. 160,000+ file IDs, 100,000+ buildings, relocated `CModelInfo`/`CStreaming` tables), legacy mods and CLEO scripts that hard-code original GTA SA 1.0 US memory addresses will crash or read garbage. FLA++ does **not** patch the game itself — it acts as a **regulatory compatibility layer** that:

- Rewrites hard-coded vanilla addresses inside legacy mods at runtime
- Guards early pool access with on-demand pool construction
- Bridges opcode behavior for scripts that rely on fixed special-actor slots / radar traces
- Classifies crashes to distinguish FLA issues from mod bugs
- Protects modern plugins (SilentPatch, SkyGfx, etc.) from being touched

---

## Target Environment

| Component | Version / Notes |
|-----------|-----------------|
| Game | GTA SA 1.0 US HOODLUM (14,383,616 bytes) |
| Limit Adjuster | FLA 7.6 & OLA  |
| CLEO | CLEO 5.4 + CLEO+ (final) |
| Tested mods | Urbanize, Proper Fixes, RoSA, Taxi77, SanPablo, MixSets |

---

## Architecture

```
FLA (expands limits, relocates tables)
   │
   ▼
FLACompatBridge.asi  ──►  ModulePolicy (allowlist / denylist)
   │                        │
   ▼                        ▼
┌─────────────────┐   ┌──────────────────┐
│ Diagnostics     │   │ RuntimeRewrite   │
│ - VEH + crash   │   │ - Scan legacy    │
│   classification│   │   modules for    │
│ - Module snap   │   │   old addresses  │
│ - Risk const    │   │ - Patch to FLA   │
│   scan          │   │   relocated addr │
└─────────────────┘   └──────────────────┘
   │
   ▼
┌─────────────────┐   ┌──────────────────┐
│ LazyCPoolRegistry│   │ Mod Compat Layer │
│ - On-demand pool │   │ - CLEO+ guards   │
│   construction   │   │ - Urbanize preload│
│ - Avoid full     │   │ - Taxi77 radar   │
│   CPools::Init   │   │   / 03D3 fallback│
└─────────────────┘   │ - SanPablo 0296  │
                      │   special actor  │
                      └──────────────────┘
```

---

## Key Modules

| Module | Config Key | What it does |
|--------|-----------|--------------|
| **RuntimeRewrite** | `EnableRuntimeRewrite` | Scans allowed legacy modules (`.cleo`, `.asi`) for hard-coded vanilla addresses like `0xA9B0C8` (`CModelInfo`) and rewrites them to FLA's current relocated table. Supports rescan for late-loaded plugins. |
| **LazyCPoolRegistry** | `EnableLazyCPoolRegistry` | When a legacy mod tries to allocate from a pool that isn't ready yet, constructs just that pool using FLA INI capacities instead of calling the dangerous full `CPools::Initialise`. |
| **SpecialActorBridge** | `EnableSanPabloSpecialActorBridge` | Intercepts CLEO opcode `0296` (`unload_special_actor`) and provides encoded modes for high-ID special actor requests without fragile string parsing. |
| **RadarTrace Bridge** | `EnableRadarBlipHandleGuard`, `EnableCleoTargetBlipCoordsBridge` | Redirects old CLEO scripts reading `CRadar::ms_RadarTrace` directly to FLA's expanded trace table; prevents taxi/GPS scripts from falling back to world `(0, 0)`. |
| **CrashClassification** | `EnableCrashClassification` | VEH handler doesn't just log raw EIP — it categorizes: `MODEL_BOUND_CENTRE`, `RADAR_BLIP_HANDLE`, `PLACEABLE_MATRIX_LIFETIME`, `CLEO_PLUGIN`, `FLA_INTERNAL`, etc. |
| **Taxi77 Guard** | `EnableTaxi77SetCarCoordinatesGuard`, `EnableTaxi77StateWatchdog` | Blocks `00AB` writes near `(0, 0)` and recovers with last known target-blip coords; watchdog detects stuck main thread and jumps back to `@START`. |
| **CLEO+ Guards** | `EnableCleoDispatchGuard`, `EnableCleoThunk26720Guard` | Hardens CLEO+ dispatch paths against null objects and invalid jump targets when FLA expands object/ped/vehicle pools. |

---

## Build

Requires **Visual Studio Build Tools 2022** (v145) or full VS2022.

```powershell
cmd /c 'call "C:

Program Files (x86)

Microsoft Visual Studio

2022

BuildTools

VC

Auxiliary

Build

vcvars32.bat" >nul && MSBuild "FLACompatBridge.vcxproj" /p:Configuration=Release /p:Platform=Win32 /m'
```

Output:
```
FLACompatBridge.asi
FLACompatBridge.pdb
```

Deploy:
```powershell
Copy-Item FLACompatBridge.asi ..\..\scripts\FLACompatBridge.asi -Force
```

> Do **not** overwrite while `gta_sa.exe` is running.

---

## Config Highlights

See [`FLACompatBridge.ini`](FLACompatBridge.ini) for full options.

```ini
[General]
EnableBridge = 1
EnableDiagnosticsGroup = 1
EnableFLACompat = 1
EnableModCompat = 1

[ModulePolicy]
; Denylist wins over allowlist
ModernModuleDenylist = SilentPatch;WidescreenFix;WindowedMode;CrashInfo;modloader.asi;MixSets;FLACompatBridge;fastman92;ProperFixes;SkyGfx;VehFuncs

[RuntimeRewrite]
EnableRuntimeRewrite = 1
EnableRuntimeRewriteRescan = 1
RuntimeRewriteAllowlist = CLEO+.cleo;SA.Text.cleo;SA.GameEntities.cleo;SA.MemoryOperations.cleo;SA.Audio.cleo;std.;urbanize

[FLA]
EnableFlaTrainInitHookRepair = 1
EnableLazyCPoolRegistry = 1
EnablePickupModelLoadGuard = 1
```

---

## Exported API

Other ASIs can query FLA++ at runtime via `GetProcAddress`:

```c
uint32_t FLACompatBridge_GetApiVersion(void);
uint32_t FLACompatBridge_GetFileIdCapacity(void);
uintptr_t  FLACompatBridge_GetRelocatedAddress(uint32_t id);
int        FLACompatBridge_GetPoolInfoByName(const char* name, uintptr_t* poolPtr, uint32_t* capacity);
uint32_t   FLACompatBridge_RebuildSpecialActorCatalog(void);
int        FLACompatBridge_RequestSpecialActorByCode(uint32_t modelId, uint32_t actorCode);
```

Header: [`FLACompatBridgeAPI.h`](FLACompatBridgeAPI.h)

---

## File Overview

| File | Purpose |
|------|---------|
| `FLACompatBridge.cpp` | Single-file implementation (~400KB, ~10,600 lines). Contains VEH, RuntimeRewrite, LazyCPool, SpecialActorBridge, crash classification, mod guards, and all export APIs. |
| `FLACompatBridgeAPI.h` | C export declarations for external ASIs. |
| `FLACompatBridge.vcxproj` | VS2022 project: Win32, `/MT`, C++17, outputs `.asi`. |
