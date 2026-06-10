FLA++ (FLACompatBridge) Changelog
==================================

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
