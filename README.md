# FLA++ / FLACompatBridge

FLA++ is a compatibility bridge for **GTA San Andreas 1.0 US** running with
fastman92 Limit Adjuster. The runtime file is still named
`FLACompatBridge.asi` so existing loaders and external ASIs can keep using the
same module/API name.

Current release candidate: `v1.10c1`
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
```

Only enable broad RuntimeRewrite rules for modules that need them. Modern fixes
and render plugins should stay denied unless you are testing a specific crash.

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
