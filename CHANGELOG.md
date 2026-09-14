FLA++ (FLACompatBridge) Changelog
==================================

v1.10c2 - 2026-07-11
--------------------
- Second release candidate for the v1.10 compatibility feature line.

[Main-Thread Safety]
- Deferred ExtendedData pool continuations are no longer invoked by a worker
  thread. A bounded, validated replay pump now runs from the GTA
  CStreaming::IsVeryBusy game-thread hook.
- PedStreamingZoneRepair is diagnostic-only and can no longer call
  CStreaming::StreamZoneModels off-thread.
- DllMain now performs only minimal state setup and starts one initialization
  thread. Configuration I/O, OLA sanitization, VEH registration, and early
  ProperShaders setup run outside the loader lock.
- VehFuncs pool-guard polling now starts immediately after configuration and
  VEH setup, before the delayed compatibility pass, preventing its startup
  `VehicleExtendedData::AllocateBlocks` call from racing the guard installer.
- Bridge configuration and logging now use absolute paths rooted at
  `gta_sa.exe`, with bounded startup retries for transient config-open races.
- The deferred replay pump now chains the verified Hoodlum
  `CStreaming::IsVeryBusy` target used by FLA instead of abandoning the pump
  when FLA has already redirected the vanilla entry point.
- A signature-checked `CPools::Initialise` tail hook runs two bounded replay
  batches after the original function returns. Explicit main-thread CPools
  recovery and successful lazy pool creation use the same path, covering FLA
  setups that create the core pools individually and startup phases that do not
  call `CStreaming::IsVeryBusy`.
- VehFuncs discovery now requires the exact `VehFuncs.asi` module name; a
  `gsx.asi` loaded from the `modloader\\VehFuncs` directory can no longer be
  mistaken for VehFuncs and patched at its fallback RVA.

[Animation and Exception Recovery]
- Invalid animation frame node arrays now reject the current frame without
  truncating live game-owned arrays.
- Fixed the velocity-frame guard flag preservation and removed the incorrect
  x87 fstp from the ShouldModelBeStreamed exception recovery path.
- Exact-EIP render/animation recovery now accepts stale high addresses while
  still checking the access type.

[OLA Coexistence]
- OLA identification now requires the exact III.VC.SA.LimitAdjuster.asi base
  name and no longer matches $fastman92limitAdjuster.asi.
- Removed runtime restoration of hard-coded vanilla pool-hook bytes. OLA
  overlap handling is performed by pre-load INI sanitization so active FLA
  hooks are not destroyed.

[ProperShaders Coexistence]
- Replaced the destructive AddTxdSlot overlap workaround with a combined tail
  adapter that executes both the FLA TXD hash-registration thunk and the
  ProperShaders callback with their expected register/stack contracts.
- Removed the scan that rewrote FLA-relocated GTA CModelInfo pointers back to
  vanilla addresses.
- Fixed-RVA ProperShaders patches and executable CStreaming rewrites are now
  gated by the verified ProperShaders text hash and byte signatures.

[Release Defaults]
- RuntimeRewrite is disabled and code-enforced to executable pages only.
- Ped streaming repair and population-budget override are disabled; the
  population comparison remains at the vanilla 4 ms default.
- Deferred targeted pool replay is enabled in both the generated configuration
  and compiled fallback so a guarded allocation cannot be silently discarded.
- No API or export changes; API version remains 6.

v1.10c1 - 2026-06-12
--------------------
- Post-v1.00 compatibility release candidate for FLA++.
- Product version follows the existing FLA_PLUS_PLUS release line. This build
  targets fastman92 Limit Adjuster 7.6 but is not versioned as 7.6.
- Adds guarded ProperShaders compatibility for the FLA AddTxdSlot hook conflict,
  shader-name null return path, and CModelInfo render-range relocation mismatch.
- Adds TXD/RwTexDictionary pointer guards, a WidescreenFix sprite-name guard,
  and additional animation association/frame update guards for crash and stuck
  animation recovery.
- Runtime filename and exported API namespace remain FLACompatBridge for
  compatibility.
- API version remains 6; this release number does not imply an ABI break.

2026-06-09
----------
[Pattern Scanning + Version Hash Fallback for PoolAllocateGuard]
- Replaced hard-coded module offsets with .text-section pattern scanning
  for CLEO+, MixSets, and Urbanize AllocateBlocks hooks.
- Pattern: "mov eax, [CPools::ms_p*Pool] ; mov reg, [eax+4]" with ModRM
  validation to reject false positives.
- If pattern scan is ambiguous (multiple matches) or finds no match,
  falls back to CRC32 .text-hash version database.
- Ultimate fallback retains legacy hard-coded offsets so existing setups
  continue to work while the new system is being validated.
- Added psapi dependency for GetModuleInformation / section enumeration.
- Rationale: previous hard-coded offsets (CLEO+ 0x30590/0x30700/0x30870,
  MixSets 0x00CCB0, Urbanize 0x018750) break silently when the target
  module is recompiled or updated.

[Pattern Scanning Bugfix]
- Fixed malformed mask string in FindAllocateBlocksByPattern that caused
  the scanner loop to never execute ("xxxxx xx?x" -> "xxxxx?xx").
- Corrected the assumed instruction sequence. Actual CLEO+/MixSets/Urbanize
  machine code is:
      A1 [poolPtr]          ; mov eax, [CPools::ms_p*Pool]
      ?? 04 00 00 00        ; mov <reg>, 4
      56                    ; push esi
      8B F1                 ; mov esi, ecx
  not the previously assumed "mov reg, [eax+4]".
- Added measured CLEO+ text-hash 0x11DFE4B9 to the known-version table.

[Rename CLEO+ Object Guard]
- Renamed log label from "ObjectExtendedData::AllocateBlocks" to
  "CLEO+ ObjectPool accessor (AllocateBlocks-like prologue)".
- CLEO+ source confirms it has no ObjectExtendedData<T> template instance;
  Object extensions use ObjExtendedDataStore (std::unordered_map).
- The 0x30590 pattern is still patched because it has the same prologue
  shape and the guard is benign, but the name now accurately reflects
  that it is not a true ObjectExtendedData::AllocateBlocks.

[LazyCPool Immediate Deferred Allocate Trigger]
- When EnsureLazyCPoolReady successfully creates a missing pool,
  immediately trigger any deferred PoolAllocateGuard replays for that
  pool instead of waiting for DeferredPoolAllocateReplayThread polling.
- This reduces the window where plugin-sdk ExtendedData blocks are not
  yet allocated after a pool is lazily created, mitigating potential
  timing/capacity mismatches between the pool and ExtendedData arrays.

[Export FLACompatBridge_IsModelLoaded API]
- New C export: FLACompatBridge_IsModelLoaded(uint32_t modelId)
  Returns 1 if the model's streaming load state == 1, 0 otherwise.
- Internally uses Bridge's existing GetStreamingLoadState via FLA's
  relocated CStreaming::ms_aInfoForModel table.
- This API is provided for CLEO+ and other modules to call instead of
  their own hard-coded streaming info checks, which fail in high-ID mode.
- CLEO+ FLACompat::IsModelLoaded currently returns true unconditionally
  in high-ID mode because FLA does not export load state. Bridge now
  provides this capability via the export.

2026-06-10
----------
[RuntimeRewrite Data Section False Positive Fix]
- Changed default rule ExecutableOnly from 0 to 1 for all three built-in
  rules (CModelInfo, CStreaming, RadarTrace).
- Added heuristic filter for non-executable regions: when ExecutableOnly=0,
  matches in data sections are now rejected unless the preceding byte is a
  plausible instruction opcode that takes a 32-bit immediate operand
  (A1/A3, 68, B8-BF, 05/0D/15/1D/25/2D/35/3D).
- This prevents patching data values that happen to coincidentally equal
  the old address constant (e.g. a struct field, vtable entry, or padding).
