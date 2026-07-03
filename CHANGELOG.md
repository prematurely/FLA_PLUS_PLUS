FLA++ (FLACompatBridge) Changelog
==================================

v1.10c2 - 2026-07-03
--------------------
- Second release candidate for the v1.10 compatibility feature line.

[Open Limit Adjuster Coexistence Guard]
- New subsystem: GuardOpenLimitAdjusterModuleLoad, GuardOpenLimitAdjusterSaLimits,
  AuditOpenLimitAdjusterSaOverlaps, RepairOpenLimitAdjusterSaPoolHooks.
- Detects whether an SA-limit pool hook belongs to FLA or OLA, restores FLA's
  patch if OLA has overwritten it, and audits overlapping ownership instead of
  requiring OLA to be disabled outright.
- EnableOpenLimitAdjusterSaLimitGuard defaults on; EnableOpenLimitAdjusterModuleGuard
  and EnableOpenLimitAdjusterOverlapAudit default off.

[VehFuncs Pool Allocate Guard]
- Extended the existing targeted PoolAllocateGuard pattern-scanning
  (previously CLEO+/MixSets/Urbanize only) to VehFuncs.
- This is narrower than RuntimeRewrite: VehFuncs stays denied for
  RuntimeRewrite and AutoPoolGuard, only the specific AllocateBlocks guard
  is opted in.

[Animation & RenderWare Crash Guards]
- New guards: AnimBlendGroup, RpAnimBlendClumpInit, RwClumpForAllAtomics,
  and CRenderer::ShouldModelBeStreamed collision-model validation
  (Bridge_LogInvalidShouldModelBeStreamedColModel).
- All four enabled by default; target null/corrupt clump and collision-model
  access from partially streamed or loaded entities.

[Streaming Busy Threshold & Population Update Budget Patches]
- InstallStreamingBusyThresholdPatch and InstallPopulationUpdateBudgetPatch,
  tunable via StreamingBusyThreshold and PopulationBudgetMs.
- Detects an existing hook at the population budget patch address first and
  skips patching to avoid breaking an existing hook chain.

[Ped Streaming Zone Repair + Gang-Only Population Guard]
- New watchdog threads (PedStreamingZoneRepairThread, GangOnlyPopulationGuardThread)
  for zone-based streaming and ped population edge cases.

[Batch Lazy CPool Initialise]
- AreCorePoolsReadyForDeferredReplay and EnsureBatchLazyCPoolsInitialised add a
  batch-ready check ahead of deferred PoolAllocateGuard replay.
- EnableBatchLazyCPoolInitialise defaults off; this stays opt-in pending
  further testing, consistent with the project's stability doctrine of not
  defaulting to broad automatic recovery paths.

[ProperShaders CStreaming Image Rewrite]
- ApplyProperShadersCompat now also scans the ProperShaders module image
  itself for embedded CStreaming::ms_aInfoForModel references and rewrites
  them to the FLA-relocated address, in addition to the existing CModelInfo
  render-range table repair.
- Added runtime/probe source fallback logging so the value source
  (FLA runtime state vs. instruction-operand probe) is auditable.

[FLA Path Node Diagnostics]
- Optional scan of loose path node (.dat) files (EnableFlaPathNodeDiagnostics,
  default off) for diagnosing node ID conflicts.

[Modloader INI Parsing Helpers]
- Internal .ini section/key parsing helpers (ExtractIniSectionName,
  ExtractIniKeyName, ExtractModloaderIgnoreEntry) shared by the OLA guard
  and other modloader-aware audits.

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
