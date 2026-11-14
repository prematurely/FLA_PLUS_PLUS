# FLA++ / FLACompatBridge

Compatibility bridge for **GTA San Andreas 1.0 US** running with **fastman92 Limit Adjuster (FLA)**.

The runtime file keeps the name `FLACompatBridge.asi` on purpose, so existing loaders and external
ASIs that link against the module/API name keep working.

| | |
|---|---|
| Latest release | [**v1.11**](https://github.com/prematurely/FLA_PLUS_PLUS/releases/tag/v1.11) — requires SA-RenderStack v3.2 |
| Public baseline | v1.00 |
| API version | 6 (37 exports) |

---

## For players: install in one drop

### 1. Check you have the right game

| Component | Required |
|---|---|
| Game | GTA San Andreas **1.0 US** (HOODLUM) |
| Limit adjuster | fastman92 Limit Adjuster **7.6** |
| CLEO | CLEO **5.4** + CLEO+ |
| FLA++ v1.11 only | SA-RenderStack **v3.2** or newer |

If your `gta_sa.exe` is v1.01, v2.00, the Steam build, or the Definitive Edition, **this will not work** —
the bridge patches 1.0 US addresses directly.

### 2. Drop one file into the game

1. Download `FLACompatBridge.asi` from the [latest release](https://github.com/prematurely/FLA_PLUS_PLUS/releases/latest).
2. Put it in:

```text
GTA San Andreas\scripts\FLACompatBridge.asi
```

3. Make sure the game is **not running**, then launch it once.

The first launch writes `GTA San Andreas\scripts\FLACompatBridge.ini` with the release defaults.
**Do not copy an INI from an older version** — the config gains keys between versions, and a stale file
silently keeps old behaviour. If you need to reset, delete the INI and start the game again.

> **modloader users:** an `.asi` inside a modloader folder also works, because config and log paths are
> resolved from `gta_sa.exe` rather than the current directory. Confirm the log file appears (next step)
> before assuming it loaded.

### 3. Verify it actually loaded

Start the game, then check:

```text
GTA San Andreas\scripts\FLACompatBridge.log
```

- The file exists and has lines → the bridge is running.
- No file at all → the ASI was not loaded. Check: it is in `scripts\`, your loader loads `.asi` from there,
  and no other tool overwrote it.

### 4. Uninstall / roll back

Delete `FLACompatBridge.asi`, `FLACompatBridge.ini` and `FLACompatBridge.log` from `scripts\`.
Nothing else is modified on disk. To go back to a previous version, put the old ASI back and delete the INI
so it regenerates for that version.

### 5. Troubleshooting first

| Symptom | Likely cause |
|---|---|
| Crash at startup after swapping versions | Old INI left behind — delete the INI |
| No log file, nothing happens | ASI not loaded (wrong folder, loader missing, or blocked) |
| Crash or missing textures only with ProperShaders | Version mismatch — see the v1.11 note below |
| Pool or pool-hook problems after installing Open Limit Adjuster | OLA is re-enabling limits that FLA already owns |

> **v1.11 note:** the ProperShaders render-state guard moved out of this repository into SA-RenderStack's
> D3D9 layer. With SA-RenderStack v3.1 or older, **no component owns that guard**. Install SA-RenderStack
> **v3.2+** if you run ProperShaders.

---

## 简体中文快速开始

**前置条件**：GTA SA **1.0 US (HOODLUM)** + fastman92 Limit Adjuster **7.6** + CLEO 5.4 / CLEO+。
使用 v1.11 还需要 **SA-RenderStack v3.2** 或更新版本。

**安装（一步落地）**：

1. 从 [最新 Release](https://github.com/prematurely/FLA_PLUS_PLUS/releases/latest) 下载 `FLACompatBridge.asi`。
2. 关闭游戏，把文件放进 `GTA San Andreas\scripts\`。
3. 启动一次游戏，会自动生成 `scripts\FLACompatBridge.ini`。**不要**从旧版本复制 INI。

**验证**：看 `scripts\FLACompatBridge.log` 是否生成并有内容；没有文件说明 ASI 没被加载。

**卸载**：删掉 `scripts\` 下的 `FLACompatBridge.asi`、`.ini`、`.log` 三个文件即可，不会改动其他文件。

---

## What it does

- Protects legacy CLEO/ASI code from FLA-relocated game tables.
- Guards pool, model, streaming, radar, animation and TXD access paths that high-limit installs hit.
- Adds compatibility guards for ProperShaders, WidescreenFix, CLEO+, Urbanize, Taxi77, SanPablo, MixSets
  and similar legacy-risk modules.
- Keeps modern loaders/fixes out of RuntimeRewrite through module policy denylist rules.

FLA++ is **not** a replacement for fastman92 Limit Adjuster. It runs beside FLA and patches the
compatibility problems caused by expanded limits, high IDs, relocated tables, and older plugins that
still assume vanilla addresses.

## Supported setup

| Component | Notes |
|---|---|
| Game | GTA SA 1.0 US HOODLUM |
| Limit adjuster | fastman92 Limit Adjuster 7.6 |
| CLEO | CLEO 5.4 + CLEO+ |
| Render/fix mods | ProperShaders, SkyGfx, WidescreenFix, SilentPatch |
| Other tested mods | Urbanize, Proper Fixes, RoSA, Taxi77, SanPablo, MixSets |

Open Limit Adjuster can be present for non-overlapping limits, but SA limits already owned by FLA must stay
disabled there.

## Release history

| Version | Type | Notes |
|---|---|---|
| [v1.11](https://github.com/prematurely/FLA_PLUS_PLUS/releases/tag/v1.11) | stable | Architecture split into 15 translation units; render-state contract moved to SA-RenderStack; ProperShaders gate re-pointed at the 2026-07-03 build. In-game verification pending. |
| [v1.10c3](https://github.com/prematurely/FLA_PLUS_PLUS/releases/tag/v1.10c3) | prerelease | Frozen before the split. Last build where FLA++ owns the render-state guard. Known limitation: only recognises the 2026-06-02 ProperShaders build. |
| [v1.10c2](https://github.com/prematurely/FLA_PLUS_PLUS/releases/tag/v1.10c2) | prerelease | Superseded. |
| [v1.00](https://github.com/prematurely/FLA_PLUS_PLUS/releases/tag/v1.00) | baseline | First public release. |

See [CHANGELOG.md](CHANGELOG.md) for the full list.

## Config notes

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

Only enable broad RuntimeRewrite rules for modules that need them. Modern fixes and render plugins should
stay denied unless you are testing a specific crash. Keep OLA's non-overlapping limits available, but let
the SA-limit guard comment out entries already owned by FLA.

## Build from source

Requires Visual Studio Build Tools with the Win32 MSVC toolchain (CI uses `v143`).

```powershell
msbuild .\FLACompatBridge.vcxproj `
  /m `
  /p:Configuration=Release `
  /p:Platform=Win32
```

Output: `build\FLACompatBridge.asi` (plus `.pdb` locally).

The release workflow builds the ASI and publishes it as the release asset; the INI is generated by the
bridge on first run, so no INI is shipped.

## Exported API

External ASIs can load `FLACompatBridge.asi` and query exports with `GetProcAddress`.

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

See [FLACompatBridgeAPI.h](FLACompatBridgeAPI.h) for the full surface (API version 6, 37 exports).

## Source layout

| File | Purpose |
|---|---|
| `FLACompatBridge.cpp` | Entry point, DllMain, initialization thread. |
| `FLACompatBridgeInternal.h` | Internal declarations shared by the translation units. |
| `BridgeConfig.cpp` | INI parsing, defaults, default-config generation. |
| `BridgeGlobals.cpp` | Global state and resolved paths. |
| `BridgeLogging.cpp` | Log file handling. |
| `BridgeFlaRuntime.cpp` | FLA detection and runtime interop. |
| `BridgeMemory.cpp` | Pattern scanning, patching, VEH helpers. |
| `BridgePoolGuards.cpp` | Pool allocation guards and deferred replay. |
| `BridgeGameGuards.cpp` | Pool/model/streaming/radar guards. |
| `BridgeAnimGuards.cpp` | Animation and RenderWare crash guards. |
| `BridgeCompatCleo.cpp` | CLEO/CLEO+ compatibility. |
| `BridgeCompatOla.cpp` | Open Limit Adjuster coexistence. |
| `BridgeCompatProperShaders.cpp` | ProperShaders `AddTxdSlot` arbitration and texture guards. |
| `BridgeSpecialActors.cpp` | SanPablo / Taxi77 special-actor bridge. |
| `BridgeRuntimeRewrite.cpp` | Legacy module address relocation. |
| `BridgeDiagnostics.cpp` | Diagnostic passes and reporting. |

## Versioning

- `v1.00`: first public baseline.
- `v1.01`: bugfix-only patch line.
- `v1.10`: compatibility feature line; `v1.10cN` are its release candidates.
- `v1.11`: architecture line — decoupled from the ProperShaders render-state contract.
- `v2.00`: reserved for breaking config/API/runtime behaviour changes.

FLA++ versions are independent of fastman92 Limit Adjuster versions.

## License

MIT — see [LICENSE](LICENSE).
