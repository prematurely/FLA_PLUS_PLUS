# FLA++ / FLACompatBridge

FLA++ is a compatibility bridge for **GTA San Andreas 1.0 US** running with
fastman92 Limit Adjuster. The runtime file is still named
`FLACompatBridge.asi` so existing loaders and external ASIs can keep using the
same module/API name.

Current release candidate: `v1.10c2`
Public baseline: `v1.00`
API version: `6`

## What It Does

- Protects legacy CLEO/ASI code from FLA-relocated game tables.
- Guards pool, model, streaming, radar, animation, and TXD access paths that are
  commonly hit by high-limit GTA SA installs.
- Adds compatibility guards for ProperShaders, WidescreenFix, CLEO+, Urbanize,
  Taxi77, SanPablo, MixSets, and similar legacy-risk modules.
- Keeps modern loaders/fixes out of RuntimeRewrite through module policy
  denylist rules.

FLA++ is not a replacement for fastman92 Limit Adjuster. It runs beside FLA and
patches compatibility problems caused by expanded limits, high IDs, relocated
tables, and older plugins that still assume vanilla addresses.

## Install

Download the release assets:

- `FLACompatBridge.asi`
- `FLACompatBridge.ini`

Copy both files into:

```text
GTA San Andreas\scripts\
```

Do not replace the ASI while `gta_sa.exe` is running.

## Supported Setup

| Component | Notes |
|---|---|
| Game | GTA SA 1.0 US HOODLUM |
| Limit adjuster | fastman92 Limit Adjuster 7.6 |
| CLEO | CLEO 5.4 + CLEO+ |
| Render/fix mods | ProperShaders, SkyGfx, WidescreenFix, SilentPatch |
| Other tested mods | Urbanize, Proper Fixes, RoSA, Taxi77, SanPablo, MixSets |

Open Limit Adjuster can be present for non-overlapping limits, but SA limits
already owned by FLA should stay disabled there.

## v1.10c2 Highlights

- Deferred ExtendedData pool continuations now run through a bounded
  `CStreaming::IsVeryBusy` main-thread pump instead of a worker thread. The
  pump verifies the process's earliest-created thread before replaying work and
  chains FLA's verified Hoodlum implementation when FLA owns the entry hook.
  A signature-checked `CPools::Initialise` tail hook runs bounded batches after
  core pool creation, and the lazy-pool success path does the same when FLA
  creates the pools individually. This covers startup paths that have not begun
  calling `CStreaming::IsVeryBusy` yet.
- Open Limit Adjuster coexistence uses pre-load INI sanitization for overlapping
  SA limits. Runtime restoration of vanilla pool-hook bytes is disabled so FLA
  hooks are not destroyed after installation.
- Targeted PoolAllocateGuard coverage includes VehFuncs, while the broad
  automatic guard remains disabled by default. VehFuncs polling starts before
  the delayed compatibility pass to cover its early startup allocation.
- New animation/RenderWare crash guards: AnimBlendGroup,
  RpAnimBlendClumpInit, RwClumpForAllAtomics, and
  `CRenderer::ShouldModelBeStreamed` collision-model validation.
- Animation recovery no longer truncates live game-owned node arrays, and the
  velocity and x87 recovery paths preserve the expected machine state.
- ProperShaders `AddTxdSlot` coexistence now uses a combined adapter that runs
  both the FLA hash-registration thunk and the ProperShaders callback. The
  adapter is rechecked periodically if either module overwrites the GTA hook.
- ProperShaders fixed-RVA and executable `CStreaming` patches require the
  supported on-disk `.text` hash plus byte signatures. The old destructive
  CModelInfo relocation reversal has been removed.
- Ped streaming repair is diagnostic-only. Population-budget override and
  RuntimeRewrite are disabled in the release configuration; RuntimeRewrite is
  additionally restricted to executable pages when explicitly enabled.
- `DllMain` performs minimal setup and starts a single initialization thread so
  configuration I/O, OLA sanitization, and compatibility setup run outside the
  loader lock.
- Config and log paths are resolved from `gta_sa.exe`, avoiding current-directory
  races while modloader is starting.
- Optional loose path node (`.dat`) diagnostics scan.
- No API or export changes; ABI stays at version 6.

## v1.10c1 Highlights

- ProperShaders compatibility for the FLA `AddTxdSlot` hook conflict.
- ProperShaders shader-name null return guard.
- CModelInfo render-range relocation repair for ProperShaders.
- TXD and `RwTexDictionaryFindNamedTexture` pointer guards.
- WidescreenFix sprite-name guard.
- Extra animation association/frame update guards for stuck animation and crash
  recovery.
- RuntimeRewrite false-positive hardening for non-executable data sections.
- PoolAllocateGuard pattern scanning and version-hash fallback.
- `FLACompatBridge_IsModelLoaded` export for external modules.

## Config Notes

The release INI is intentionally conservative. Important switches:

```ini
[General]
EnableBridge = 1
EnableFLACompat = 1
EnableModCompat = 1

[ModulePolicy]
EnableModulePolicy = 1
ModernModuleDenylist = SilentPatch;WidescreenFix;WindowedMode;CrashInfo;modloader.asi;MixSets;FLACompatBridge;fastman92;DINPUT8;vorbis;ProperFixes;SkyGfx;VehFuncs
ForceNoRuntimeRewrite = ProperFixes;SkyGfx;ProperShaders;VehFuncs
ForceNoAutoPoolGuard = ImprovedStreaming;ProperFixes;ProperShaders
EnableProperShadersCompat = 1

[Mods]
EnableRuntimeRewrite = 0
EnableRuntimeRewriteRescan = 0

[PoolGuards]
EnableAutoPoolAllocateGuard = 0
EnableDeferredPoolAllocateReplay = 1

[Diagnostics]
EnablePedStreamingZoneRepair = 0
EnablePopulationUpdateBudgetPatch = 0
PopulationUpdateBudgetMs = 4
EnableOpenLimitAdjusterSaLimitGuard = 1
EnableOpenLimitAdjusterModuleGuard = 0
```

Only enable broad RuntimeRewrite rules for modules that need them. Modern fixes
and render plugins should stay denied unless you are testing a specific crash.
Keep OLA's non-overlapping limits available, but let the SA-limit guard comment
out entries already owned by FLA.

## Versioning

- `v1.00`: first public baseline.
- `v1.01`: bugfix-only patch line.
- `v1.10`: compatibility feature line.
- `v1.10cN`: release candidates for `v1.10`.
- `v2.00`: reserved for breaking config/API/runtime behavior changes.

FLA++ versions are separate from fastman92 Limit Adjuster versions.

## Build

Requires Visual Studio Build Tools with the Win32 MSVC toolchain.

```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\MSBuild.exe" `
  .\FLACompatBridge.vcxproj `
  /p:Configuration=Release `
  /p:Platform=Win32 `
  /m
```

Output:

```text
..\..\.codex-build\FLACompatBridge\FLACompatBridge.asi
..\..\.codex-build\FLACompatBridge\FLACompatBridge.pdb
```

## Exported API

External ASIs can load `FLACompatBridge.asi` and query exports with
`GetProcAddress`.

Common exports:

```c
uint32_t FLACompatBridge_GetApiVersion(void);
uint32_t FLACompatBridge_GetFileIdCapacity(void);
uint32_t FLACompatBridge_GetCompatFlags(void);
uint32_t FLACompatBridge_GetRuntimeSource(void);
uintptr_t FLACompatBridge_GetRelocatedAddress(uint32_t id);
uintptr_t FLACompatBridge_GetModelInfo(uint32_t modelId);
uintptr_t FLACompatBridge_GetStreamingInfo(uint32_t modelId);
int FLACompatBridge_GetPoolInfo(uint32_t poolId, uintptr_t* poolPtr, uint32_t* capacity);
int FLACompatBridge_IsModelLoaded(uint32_t modelId);
```

See `FLACompatBridgeAPI.h` for the full API.

## Files

| File | Purpose |
|---|---|
| `FLACompatBridge.cpp` | Runtime implementation. |
| `FLACompatBridgeAPI.h` | Public C export declarations. |
| `FLACompatBridge.vcxproj` | Win32 ASI build project. |
| `CHANGELOG.md` | Release notes and compatibility changes. |
