#include "FLACompatBridgeInternal.h"

void ResolveGameRelativePath(const char* relativePath, char* out, size_t outSize)
{
    if (!relativePath || !out || outSize == 0) {
        return;
    }

    char executablePath[MAX_PATH]{};
    const DWORD length = GetModuleFileNameA(nullptr, executablePath, static_cast<DWORD>(sizeof(executablePath)));
    if (!length || length >= sizeof(executablePath)) {
        return;
    }

    char* slash = std::strrchr(executablePath, '\\');
    char* slash2 = std::strrchr(executablePath, '/');
    if (slash2 && (!slash || slash2 > slash)) {
        slash = slash2;
    }
    if (!slash) {
        return;
    }

    const size_t directoryLength = static_cast<size_t>(slash - executablePath) + 1;
    const size_t relativeLength = std::strlen(relativePath);
    if (directoryLength + relativeLength >= outSize) {
        return;
    }

    std::memcpy(out, executablePath, directoryLength);
    std::memcpy(out + directoryLength, relativePath, relativeLength + 1);
}

void ResolveGamePath(const char* path, char* out, size_t outSize)
{
    if (!path || !out || outSize == 0) {
        return;
    }

    out[0] = '\0';
    const bool absolute =
        (path[0] && path[1] == ':') ||
        (path[0] == '\\' && path[1] == '\\') ||
        (path[0] == '/' && path[1] == '/');
    if (absolute) {
        strncpy_s(out, outSize, path, _TRUNCATE);
        return;
    }

    ResolveGameRelativePath(path, out, outSize);
    if (!out[0]) {
        strncpy_s(out, outSize, path, _TRUNCATE);
    }
}

void InitializeBridgeFilePaths()
{
    ResolveGameRelativePath(kLogRelativePath, g_logPath, sizeof(g_logPath));
    ResolveGameRelativePath(kConfigRelativePath, g_configPath, sizeof(g_configPath));
    ResolveGameRelativePath(kFlaLogRelativePath, g_flaLogPath, sizeof(g_flaLogPath));
    ResolveGameRelativePath(kFlaIniRelativePath, g_flaIniPath, sizeof(g_flaIniPath));
}

bool ReadSmallTextValue(const char* filePath, const char* key, char* out, size_t outSize)
{
    char resolvedPath[MAX_PATH]{};
    ResolveGamePath(filePath, resolvedPath, sizeof(resolvedPath));
    if (!resolvedPath[0]) {
        return false;
    }

    FILE* file = nullptr;
    const bool isBridgeConfig = _stricmp(resolvedPath, g_configPath) == 0;
    const int maxOpenAttempts = isBridgeConfig ? 101 : 1;
    for (int attempt = 0; attempt < maxOpenAttempts; ++attempt) {
        if (fopen_s(&file, resolvedPath, "r") == 0 && file) {
            break;
        }
        if (attempt + 1 < maxOpenAttempts) {
            InterlockedIncrement(&g_bridgeConfigOpenRetries);
            Sleep(10);
        }
    }
    if (!file) {
        return false;
    }

    char line[512];
    const size_t keyLen = std::strlen(key);
    bool found = false;
    while (std::fgets(line, sizeof(line), file)) {
        char* p = line;
        while (*p == ' ' || *p == '\t') {
            ++p;
        }
        if (*p == '#' || *p == ';') {
            continue;
        }
        if (_strnicmp(p, key, keyLen) != 0) {
            continue;
        }
        p += keyLen;
        while (*p == ' ' || *p == '\t') {
            ++p;
        }
        if (*p != '=') {
            continue;
        }
        ++p;
        while (*p == ' ' || *p == '\t') {
            ++p;
        }

        char* end = p + std::strlen(p);
        while (end > p && (end[-1] == '\r' || end[-1] == '\n' || end[-1] == ' ' || end[-1] == '\t')) {
            --end;
        }
        *end = '\0';

        strncpy_s(out, outSize, p, _TRUNCATE);
        found = true;
        break;
    }

    std::fclose(file);
    return found;
}

bool ReadSectionTextValue(const char* filePath, const char* section, const char* key, char* out, size_t outSize)
{
    char resolvedPath[MAX_PATH]{};
    ResolveGamePath(filePath, resolvedPath, sizeof(resolvedPath));
    if (!resolvedPath[0]) {
        return false;
    }

    FILE* file = nullptr;
    if (fopen_s(&file, resolvedPath, "r") != 0 || !file) {
        return false;
    }

    char line[512];
    const size_t keyLen = std::strlen(key);
    bool inSection = false;
    bool found = false;

    while (std::fgets(line, sizeof(line), file)) {
        char* p = line;
        while (*p == ' ' || *p == '\t') {
            ++p;
        }

        if (*p == '[') {
            char* close = std::strchr(p + 1, ']');
            if (close) {
                *close = '\0';
                inSection = _stricmp(p + 1, section) == 0;
            }
            continue;
        }

        if (!inSection || *p == '#' || *p == ';' || *p == '\0' || *p == '\r' || *p == '\n') {
            continue;
        }
        if (_strnicmp(p, key, keyLen) != 0) {
            continue;
        }
        p += keyLen;
        while (*p == ' ' || *p == '\t') {
            ++p;
        }
        if (*p != '=') {
            continue;
        }
        ++p;
        while (*p == ' ' || *p == '\t') {
            ++p;
        }

        char* end = p + std::strlen(p);
        while (end > p && (end[-1] == '\r' || end[-1] == '\n' || end[-1] == ' ' || end[-1] == '\t')) {
            --end;
        }
        *end = '\0';

        strncpy_s(out, outSize, p, _TRUNCATE);
        found = true;
        break;
    }

    std::fclose(file);
    return found;
}

void WriteDefaultBridgeConfigIfMissing()
{
    if (GetFileAttributesA(g_configPath) != INVALID_FILE_ATTRIBUTES) {
        return;
    }

    static const char text[] =
        "; FLA++ configuration\n"
        "; Runtime file/API name remains FLACompatBridge for compatibility.\n"
        "; 1 = enabled, 0 = disabled. Defaults preserve the current tested stable profile.\n"
        "\n"
        "[General]\n"
        "; Master switch for this bridge. Set 0 to disable all bridge patches except the loader itself.\n"
        "EnableBridge = 1\n"
        "\n"
        "; Group switches. These work like MixSets-style category toggles.\n"
        "; If a group is 0, all detailed options under that group are ignored.\n"
        "EnableDiagnosticsGroup = 1\n"
        "EnableFLACompat = 1\n"
        "EnableModCompat = 1\n"
        "\n"
        "[Diagnostics]\n"
        "EnableVectoredExceptionHandler = 1\n"
        "EnableExceptionDiagnostics = 1\n"
        "EnableStackScanDiagnostics = 0\n"
        "EnableMemoryRegionDiagnostics = 0\n"
        "EnableBoundCentreVehRecovery = 1\n"
        "; Skips one failed CPtrList insertion when PtrNodeSingle/PtrNodeDouble allocation returns null instead of crashing.\n"
        "EnablePtrNodeExhaustionGuard = 1\n"
        "EnableExceptionLoopBreaker = 1\n"
        "ExceptionLoopBreakerTerminate = 1\n"
        "ExceptionLoopBreakerThreshold = 32\n"
        "MaxExceptionLogs = 16\n"
        "EnableModuleSnapshot = 0\n"
        "EnableRiskConstantScan = 0\n"
        "EnableRelocatedAddressDiagnostics = 0\n"
        "EnablePoolPointerDiagnostics = 0\n"
        "; Runtime population pool sampling. Logs Ped/Vehicle/Object pool usage after a save is loaded.\n"
        "EnablePopulationPoolDiagnostics = 0\n"
        "PopulationPoolDiagStartDelayMs = 15000\n"
        "PopulationPoolDiagIntervalMs = 5000\n"
        "PopulationPoolDiagIterations = 24\n"
        "; Clears the vanilla GANGS_CONTROLS_THE_STREETS population latch if a save/mod leaves it stuck on.\n"
        "; This does not clear GANGMEMBERS_EVERYWHERE, so gang/Urbanize content can still spawn normally.\n"
        "EnableGangOnlyPopulationGuard = 1\n"
        "GangOnlyPopulationGuardStartDelayMs = 15000\n"
        "GangOnlyPopulationGuardIntervalMs = 5000\n"
        "GangOnlyPopulationGuardIterations = 72\n"
        "GangOnlyPopulationGuardClearCheatFlag = 1\n"
        "; Diagnostic-only sampler for vanilla CStreaming pedestrian-zone state.\n"
        "; It never calls GTA streaming functions from its worker thread.\n"
        "EnablePedStreamingZoneRepair = 0\n"
        "; Legacy compatibility key. Runtime calls remain disabled even if this is set to 1.\n"
        "PedStreamingZoneRepairCallOriginal = 0\n"
        "PedStreamingZoneRepairStartDelayMs = 20000\n"
        "PedStreamingZoneRepairIntervalMs = 2000\n"
        "PedStreamingZoneRepairIterations = 180\n"
        "PedStreamingZoneRepairMaxCalls = 12\n"
        "PedStreamingZoneRepairMaxLogs = 64\n"
        "; Raises CStreaming::IsVeryBusy threshold so heavy map/LOD preloading does not starve normal pedestrian streaming forever.\n"
        "EnableStreamingBusyThresholdPatch = 1\n"
        "StreamingBusyThreshold = 128\n"
        "; Optional diagnostic override for CGame::Process population generation budget.\n"
        "; The release default preserves the vanilla 4 ms comparison.\n"
        "EnablePopulationUpdateBudgetPatch = 0\n"
        "PopulationUpdateBudgetMs = 4\n"
        "EnableCrashClassification = 1\n"
        "EnableRuntimeRewriteAudit = 0\n"
        "EnableOpenLimitAdjusterOverlapAudit = 0\n"
        "; Logs whether FLA Paths map size / DAT node IDs match the installed nodes*.dat file set.\n"
        "EnableFlaPathNodeDiagnostics = 0\n"
        "; Source comparison with III.VC.SA.LimitAdjuster v1.6.1: OLA SA limits patch the same pools/lists that FLA owns.\n"
        "; Keep this on when fastman92 Limit Adjuster/FLA++ is active. Use allowlist only for one-key experiments.\n"
        "EnableOpenLimitAdjusterSaLimitGuard = 1\n"
        "; Emergency isolation only. Set 1 to add OLA to modloader's IgnoreMods if OLA itself still crashes.\n"
        "; Keep 0 for real coexistence testing; FLA++ will sanitize OLA SA overlaps instead.\n"
        "EnableOpenLimitAdjusterModuleGuard = 0\n"
        "OpenLimitAdjusterSaLimitAllowlist = \n"
        "\n"
        "[ModulePolicy]\n"
        "; Global compatibility policy shared by RuntimeRewrite and AutoPoolAllocateGuard.\n"
        "; Deny wins over every allowlist. Add modern fixes/loaders here so FLA++ only manages legacy-risk modules.\n"
        "EnableModulePolicy = 1\n"
        "LegacyModuleAllowlist = .cleo;.asi;modloader\n"
        "ModernModuleDenylist = SilentPatch;WidescreenFix;WindowedMode;CrashInfo;modloader.asi;MixSets;FLACompatBridge;fastman92;DINPUT8;vorbis;_noDEP;rundll32exefix;iii.vc.sa.limitadjuster;ImgLimitAdjuster;ProperFixes;ProperShaders\n"
        "ForceNoRuntimeRewrite = \n"
        "ForceNoAutoPoolGuard = ImprovedStreaming\n"
        "; ProperShaders writes a TXD hook inside FLA's CTxdStore::AddTxdSlot JMP. Keep this on if ProperShaders is loaded.\n"
        "EnableProperShadersCompat = 1\n"
        "\n"
        "[RuntimeRewrite]\n"
        "; Runtime rewrite patches loaded modules in memory. Keep 0 unless testing a named module.\n"
        "EnableRuntimeRewrite = 0\n"
        "; Re-scan after startup so late-loaded CLEO/ASI/modloader DLL modules are covered.\n"
        "EnableRuntimeRewriteRescan = 0\n"
        "EnableRuntimeRewriteRuleTable = 1\n"
        "RuntimeRewriteStartDelayMs = 1000\n"
        "RuntimeRewriteIterations = 60\n"
        "RuntimeRewriteIntervalMs = 1000\n"
        "RuntimeRewriteRuleCount = 3\n"
        "RuntimeRewriteDefaultMaxPatchesPerModule = 256\n"
        "; Semicolon/comma separated substring allowlist, for example: .cleo;urbanize\n"
        "; Empty means no module is patched, even with EnableRuntimeRewrite = 1.\n"
        "; This is a safety default, not a bug: list every module by name instead of\n"
        "; leaving this blank and expecting it to mean \"patch everything\".\n"
        "RuntimeRewriteAllowlist = \n"
        "; Semicolon/comma separated substring denylist. Denylist wins over allowlist.\n"
        "RuntimeRewriteDenylist = SilentPatch;WidescreenFix;WindowedMode;CrashInfo;modloader.asi;MixSets;FLACompatBridge;fastman92;DINPUT8;vorbis\n"
        "\n"
        "[RuntimeRewriteRules]\n"
        "; RuleNNNTarget: CModelInfo, CStreaming, RadarTrace, AnimBlocks, StreamedScripts, HandlingManager, VehicleRecording, or Static.\n"
        "; RuleNNNNew is only used when Target=Static. Per-rule allow/deny lists override the global lists when non-empty.\n"
        "Rule001Enabled = 1\n"
        "Rule001Name = CModelInfo::ms_modelInfoPtrs\n"
        "Rule001Old = 0x00A9B0C8\n"
        "Rule001Target = CModelInfo\n"
        "Rule001Align4 = 1\n"
        "Rule001ExecutableOnly = 1\n"
        "Rule001AuditOnly = 0\n"
        "Rule001MaxPatchesPerModule = 256\n"
        "Rule002Enabled = 1\n"
        "Rule002Name = CStreaming::ms_aInfoForModel\n"
        "Rule002Old = 0x008E4CC0\n"
        "Rule002Target = CStreaming\n"
        "Rule002Align4 = 1\n"
        "Rule002ExecutableOnly = 1\n"
        "Rule002AuditOnly = 0\n"
        "Rule002MaxPatchesPerModule = 256\n"
        "Rule003Enabled = 1\n"
        "Rule003Name = CRadar::ms_RadarTrace\n"
        "Rule003Old = 0x00BA86F0\n"
        "Rule003Target = RadarTrace\n"
        "Rule003Align4 = 1\n"
        "Rule003ExecutableOnly = 1\n"
        "Rule003AuditOnly = 0\n"
        "Rule003MaxPatchesPerModule = 512\n"
        "\n"
        "[FLA]\n"
        "EnableLegacyModelInfoShadow = 1\n"
        "EnableLegacyStreamingInfoShadow = 1\n"
        "EnableFlaExtendedIdApi = 1\n"
        "LegacyShadowStartDelayMs = 500\n"
        "LegacyShadowIterations = 300\n"
        "LegacyShadowIntervalMs = 200\n"
        "; Restores FLA error-reporting hook at CEntity::GetBoundRect 0x534134.\n"
        "; Keeps LOD/no-collision models from becoming fatal errors.\n"
        "EnableFlaNoCollisionErrorRestore = 1\n"
        "; Safe replacement for FLA's Enable cheat string loader.\n"
        "; Keep FLA's own Enable cheat string loader disabled.\n"
        "EnableBridgeCheatStringLoader = 1\n"
        "; Repairs FLA train type carriage loader hook at CTrain::InitTrains.\n"
        "; Preserves EAX across FLA's loader call.\n"
        "EnableFlaTrainInitHookRepair = 1\n"
        "; High-risk legacy restore. Keep 0 unless testing a CObject::Init collision-hook regression.\n"
        "EnableFlaObjectInitCollisionRestore = 0\n"
        "; Guards pickup object creation when FLA/model-streaming reports a model defined but its RwObject is not loaded yet.\n"
        "; Default only covers the save pickup model. Add IDs/names separated by semicolons, for example: 1277;pickupsave\n"
        "EnablePickupModelLoadGuard = 1\n"
        "PickupModelLoadGuardProtectHighIds = 1\n"
        "PickupModelLoadGuardAllowlist = 1277;pickupsave\n"
        "PickupModelLoadGuardFlags = 0x1E\n"
        "PickupModelLoadGuardLoadNow = 1\n"
        "PickupModelLoadGuardMaxLogs = 64\n"
        "; Guards CColAccel::startCache when an old/half-patched entry reads the original CColStore pool before it is valid.\n"
        "; Uses FLA Collision size as the fallback pool size, so expanded maps keep their capacity.\n"
        "EnableColAccelStartCachePoolGuard = 1\n"
        "; Redirects null CColModelPool::New this pointer to CPools::ms_pColModelPool when FLA/plugin loaders call collision allocation through a stale entry.\n"
        "EnableColModelPoolNewGuard = 1\n"
        "; Table-driven lazy CPool construction derived from FLA/original CPools::Initialise.\n"
        "; Builds only the missing pool requested by a guard, using capacities from fastman92limitAdjuster_GTASA.ini.\n"
        "EnableLazyCPoolRegistry = 1\n"
        "; Keep 0. Batch-creating core CPools before the real game/FLA init path can leave the ped pool\n"
        "; populated with model 0/player-type entries and causes normal pedestrians to stop spawning.\n"
        "EnableBatchLazyCPoolInitialise = 0\n"
        "; Keep 0. When CReplay::MarkEverythingAsNew first touches missing Ped/Vehicle pools,\n"
        "; create only those pools via the lazy registry and continue; skipping this path crashes later.\n"
        "EnableReplayPoolReadSkipGuard = 0\n"
        "; Keep 0. Creating full core CPools before the real game/FLA init path can poison Ped population.\n"
        "; Targeted lazy pool guards below still create only the single pool required to avoid a crash.\n"
        "EnableEarlyCPoolsInitialiseRecovery = 0\n"
        "; Unsafe legacy recovery. Keep 0: calling CPools::Initialise from an exception path can leave pools half-initialized.\n"
        "; The bridge now waits for the real game initialization path and defers old plugin pool allocation until core pools are ready.\n"
        "EnableCPoolsInitialiseRecovery = 0\n"
        "\n"
        "[Mods]\n"
        "; Per-mod compatibility group switches.\n"
        "EnableCLEOPlusCompat = 1\n"
        "EnableUrbanizeCompat = 1\n"
        "EnableTaxi77Compat = 1\n"
        "EnableSanPabloCompat = 1\n"
        "EnableMixSetsCompat = 1\n"
        "\n"
        "; Shared old-mod compatibility patches.\n"
        "EnableCObjectCreateBridge = 1\n"
        "EnableAnimUncompressGuard = 1\n"
        "EnableAnimStaticAssocGuard = 1\n"
        "; Expensive legacy per-frame/per-node VirtualQuery diagnostics. Keep disabled for normal play.\n"
        "EnableAnimFrameUpdateGuard = 0\n"
        "; One integer check per animated clump. Prevents GTA's frame callbacks from entering with an empty node array.\n"
        "EnableAnimEmptyUpdateGuard = 1\n"
        "; Logs animation block/group destruction so stale static associations can be traced to their releaser.\n"
        "EnableAnimLifecycleDiagnostics = 1\n"
        "; Guards RpAnimBlendClumpInit against null/corrupt clumps before animation data allocation.\n"
        "EnableRpAnimBlendClumpInitGuard = 1\n"
        "; Guards RenderWare RpClumpForAllAtomics against null/corrupt clumps from partially loaded model instances.\n"
        "EnableRwClumpForAllAtomicsGuard = 1\n"
        "; Guards CRenderer::ShouldModelBeStreamed against null collision models from partially streamed LOD/building entries.\n"
        "EnableShouldModelBeStreamedGuard = 1\n"
        "EnableGetBoundCentreInlineGuard = 1\n"
        "EnableGetBoundRectColModelGuard = 1\n"
        "; Matrix guard master switch.\n"
        "EnablePlaceableRemoveMatrixGuard = 1\n"
        "; Disabled by default: skipping RemoveMatrix by guessing the caller return can corrupt the stack on heavy IPL loads.\n"
        "EnablePlaceableRemoveMatrixSkipGuard = 0\n"
        "; Keeps expanded map/model loads alive by allocating/recycling a static matrix when the original list is exhausted.\n"
        "EnablePlaceableStaticMatrixAllocGuard = 1\n"
        "; Matrix guard is address-backed because GTA SA 1.0 US exposes this only as fixed machine code.\n"
        "; Signature check prevents the bridge from touching this path when another plugin has changed the code bytes.\n"
        "MatrixGuardSignatureCheck = 1\n"
        "MatrixGuardRecoverStaticList = 1\n"
        "MatrixGuardListBase = 0x00B74288\n"
        "MatrixGuardRemoveMatrixEntry = 0x0054F3B3\n"
        "MatrixGuardRemoveMatrixNullLoad = 0x0054F3B6\n"
        "MatrixGuardAllocateStaticOwnerWrite = 0x0054F557\n"
        "MatrixGuardAllocateStaticResume = 0x0054F55D\n"
        "MatrixGuardAllocateStaticMin = 0x0054F4C0\n"
        "MatrixGuardAllocateStaticMax = 0x0054F640\n"
        "; Guards CRadar::GetActualBlipArrayIndex against invalid high blip handles.\n"
        "; Fixes old CLEO scripts that can pass stale blip handles when FLA expands Radar traces.\n"
        "EnableRadarBlipHandleGuard = 1\n"
        "; Bridges CLEO5 SA.GameEntities 0AB6 target-blip coords to FLA's relocated radar trace table.\n"
        "; Fixes old taxi/GPS scripts that otherwise fall back to world 0,0 under expanded Radar traces.\n"
        "EnableCleoTargetBlipCoordsBridge = 1\n"
        "; Rewrites hardcoded CRadar::ms_RadarTrace 0xBA86F0 constants in allowed old modules to FLA's relocated table.\n"
        "; Covers GPS/HUD/CLEO plugins that read LOWORD(targetBlip) directly instead of calling the game helper.\n"
        "EnableRadarTraceRuntimeRewrite = 1\n"
        "; Guards opcode 03D3 get_closest_car_node_with_heading from writing world 0,0 when remote path nodes are not streamed yet.\n"
        "; This prevents old taxi/GPS scripts from teleporting to the map origin on cross-city routes.\n"
        "EnableClosestCarNode03D3Fallback = 1\n"
        "\n"
        "[CLEOPlus]\n"
        "EnableCleoObjectCreateInlineRestore = 1\n"
        "EnableCleoDispatchGuard = 1\n"
        "; Legacy emergency fallback only. Keep 0: creating core CPools from a bad CLEO+ dispatch target\n"
        "; bypasses the real game/FLA initialization chain and can leave streets empty.\n"
        "EnableCleoDispatchLazyPoolRecovery = 0\n"
        "EnableCleoThunk26720Guard = 1\n"
        "EnableCleoPlusExtendedObjectVarGuard = 0\n"
        "EnableCleoPlusPoolAllocateGuard = 1\n"
        "\n"
        "[MixSets]\n"
        "EnableMixSetsPoolAllocateGuard = 1\n"
        "\n"
        "[VehFuncs]\n"
        "; Guards VehFuncs VehicleExtendedData::AllocateBlocks during early startup.\n"
        "; VehFuncs reads CPools::ms_pVehiclePool directly; if FLA has not created it yet,\n"
        "; the original function crashes at vehfuncs.asi+0x2965D. Keep this on when VehFuncs is installed.\n"
        "EnableVehFuncsPoolAllocateGuard = 1\n"
        "\n"
        "[Urbanize]\n"
        "EnableUrbanizePoolAllocateGuard = 1\n"
        "EnableUrbanizeProblemPedPreload = 1\n"
        "\n"
        "[PoolGuards]\n"
        "; Scans allowed legacy modules for plugin-sdk ExtendedData AllocateBlocks patterns.\n"
        "; Deferred continuations are replayed only from the GTA CStreaming game-thread hook.\n"
        "EnableAutoPoolAllocateGuard = 0\n"
        "EnableDeferredPoolAllocateReplay = 1\n"
        "AutoPoolAllocateGuardMaxPatches = 256\n"
        "DeferredPoolAllocateReplayIterations = 600\n"
        "DeferredPoolAllocateReplayIntervalMs = 100\n"
        "AutoPoolAllocateGuardAllowlist = .asi;.cleo;modloader\n"
        "AutoPoolAllocateGuardDenylist = FLACompatBridge;fastman92;DINPUT8;vorbis;CrashInfo;SilentPatch;WidescreenFix;WindowedMode;modloader.asi;_noDEP;rundll32exefix\n"
        "\n"
        "[Taxi77]\n"
        "; Blocks Taxi77 set_car_coordinates calls when they try to write near world 0,0 and logs the script IP offset.\n"
        "EnableTaxi77SetCarCoordinatesGuard = 1\n"
        "; Watches Taxi77's main CLEO thread for long-running call/boarding states that never return to @START.\n"
        "; Recovery jumps only the Taxi77 main thread back to its own @START label; no Taxi77.cs bytecode is edited.\n"
        "EnableTaxi77StateWatchdog = 1\n"
        "EnableTaxi77StateWatchdogRecovery = 1\n"
        "Taxi77WatchdogPollMs = 1000\n"
        "Taxi77MainStuckSeconds = 75\n"
        "Taxi77StartLabelOffset = 2459\n"
        "Taxi77ActiveMinOffset = 2672\n"
        "Taxi77ActiveMaxOffset = 8626\n"
        "\n"
        "[SanPablo]\n"
        "; SANPABLO-only bridge for original special actors.\n"
        "; The SANPABLO script uses opcode 0296 with integer modes: 1=request group, 2=check group, 3=release group.\n"
        "; Generic bridge modes are also available to every script through opcode 0296:\n"
        ";   100000 + modelId = request configured actor, 200000 + modelId = check, 300000 + modelId = release.\n"
        ";   10000000 + modelId * 1000 + actorCode = request a catalog actor into a target special slot.\n"
        "; Packed high-ID mode: -2147483648 + op * 536870912 + actorCode * 262144 + modelId.\n"
        ";   op: 1=request, 2=check loaded, 3=release. modelId up to 262143, actorCode up to 2047.\n"
        "; Example: 10295012 requests SpecialActorName012 into model slot 295.\n"
        "; Example: -1584398016 requests actorCode 100 into model slot 320.\n"
        "; Example: set 30@ = 100295, then 0296: unload_special_actor 30@ requests SpecialActor295.\n"
        "; This avoids CLEO 023C string parsing crashes while keeping special actor names in the ASI/INI layer.\n"
        "EnableSanPabloSpecialActorBridge = 1\n"
        "\n"
        "[SpecialActorSlots]\n"
        "; Target model IDs used as special actor slots. Vanilla is 290-299.\n"
        "; Raise MaxModelId or AutoMaxModelId when FLA/another plugin creates usable extended special slots.\n"
        "MinModelId = 290\n"
        "MaxModelId = 299\n"
        "AutoDetectExtendedSlots = 1\n"
        "AutoMaxModelId = 20000\n"
        "RequireModelInfoSlot = 1\n"
        "\n"
        "[SpecialActors]\n"
        "; Default 290-299 special actor slots. Change or add entries as needed.\n"
        "; Common valid names in gta3.img include:\n"
        "; ANDRE, BB, BBTHIN, CAT, CESAR, CLAUDE, CROGRL1, CROGRL2, CROGRL3,\n"
        "; DNB1, DNB2, DNB3, EMMET, FORELLI, JANITOR, JETHRO, JIZZY, KENDL,\n"
        "; MACCER, MADDOGG, OGLOC, PULASKI, ROSE, RYDER, SMOKE, SMOKEV,\n"
        "; SWEET, TENPEN, TRUTH, WUZIMU, ZERO.\n"
        "SpecialActor290 = SWEET\n"
        "SpecialActor291 = RYDER\n"
        "SpecialActor292 = SMOKEV\n"
        "SpecialActor293 = ZERO\n"
        "SpecialActor294 = CESAR\n"
        "SpecialActor295 = TENPEN\n"
        "SpecialActor296 = PULASKI\n"
        "SpecialActor297 = TRUTH\n"
        "SpecialActor298 = OGLOC\n"
        "SpecialActor299 = KENDL\n"
        "\n"
        "[SpecialActorCatalog]\n"
        "; Catalog used by encoded requests: 10000000 + modelId * 1000 + actorCode.\n"
        "; INI names below are stable. Auto-scanned names start at AutoCatalogFirstCode.\n"
        "AutoScanImgArchives = 1\n"
        "AutoScanModloader = 1\n"
        "AllowMissingTxd = 0\n"
        "LogCatalog = 1\n"
        "AutoCatalogFirstCode = 100\n"
        "MaxAutoCatalogNames = 512\n"
        "CatalogBuildDelayMs = 8000\n"
        "; Semicolon-separated allow filter. Tokens match exact names or numbered variants; use NAME* for a wider prefix.\n"
        "; Use * to catalog every DFF/TXD pair, but that can include non-peds.\n"
        "AutoScanFilter = SWEET;RYDER;SMOKE;SMOKEV;CESAR;ZERO;TENPEN;PULASKI;TRUTH;OGLOC;KENDL;JIZZY;MADDOGG;MACCER;WUZIMU;EMMET;JETHRO;JANITOR;CLAUDE;FORELLI;ANDRE;BB;BBTHIN;CAT;CROGRL;DNB;ROSE;SUZIE;TBONE;TORINO;HMOGAR\n"
        "SpecialActorName001 = SWEET\n"
        "SpecialActorName002 = RYDER\n"
        "SpecialActorName003 = SMOKE\n"
        "SpecialActorName004 = SMOKEV\n"
        "SpecialActorName005 = CESAR\n"
        "SpecialActorName006 = ZERO\n"
        "SpecialActorName007 = TENPEN\n"
        "SpecialActorName008 = PULASKI\n"
        "SpecialActorName009 = TRUTH\n"
        "SpecialActorName010 = OGLOC\n"
        "SpecialActorName011 = KENDL\n"
        "SpecialActorName012 = JIZZY\n"
        "SpecialActorName013 = MADDOGG\n"
        "SpecialActorName014 = MACCER\n"
        "SpecialActorName015 = WUZIMU\n"
        "SpecialActorName016 = EMMET\n"
        "SpecialActorName017 = JETHRO\n"
        "SpecialActorName018 = JANITOR\n"
        "SpecialActorName019 = CLAUDE\n"
        "SpecialActorName020 = FORELLI\n"
        "SpecialActorName021 = ANDRE\n"
        "SpecialActorName022 = BB\n"
        "SpecialActorName023 = BBTHIN\n"
        "SpecialActorName024 = CAT\n"
        "SpecialActorName025 = CROGRL1\n"
        "SpecialActorName026 = CROGRL2\n"
        "SpecialActorName027 = CROGRL3\n"
        "SpecialActorName028 = DNB1\n"
        "SpecialActorName029 = DNB2\n"
        "SpecialActorName030 = DNB3\n"
        "SpecialActorName031 = ROSE\n"
        "SpecialActorName032 = SUZIE\n"
        "SpecialActorName033 = TBONE\n"
        "SpecialActorName034 = TORINO\n"
        "SpecialActorName035 = HMOGAR\n"
        "SpecialActorName036 = RYDER2\n"
        "SpecialActorName037 = RYDER3\n";

    HANDLE file = CreateFileA(g_configPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD written = 0;
    WriteFile(file, text, static_cast<DWORD>(std::strlen(text)), &written, nullptr);
    CloseHandle(file);
}

bool ReadBridgeBool(const char* key, bool defaultValue)
{
    char value[64]{};
    if (!ReadSmallTextValue(g_configPath, key, value, sizeof(value))) {
        return defaultValue;
    }

    if (_stricmp(value, "1") == 0 || _stricmp(value, "true") == 0 ||
        _stricmp(value, "yes") == 0 || _stricmp(value, "on") == 0) {
        return true;
    }

    if (_stricmp(value, "0") == 0 || _stricmp(value, "false") == 0 ||
        _stricmp(value, "no") == 0 || _stricmp(value, "off") == 0) {
        return false;
    }

    return defaultValue;
}

int ReadBridgeInt(const char* key, int defaultValue, int minValue, int maxValue)
{
    char value[64]{};
    if (!ReadSmallTextValue(g_configPath, key, value, sizeof(value))) {
        return defaultValue;
    }

    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 0);
    if (end == value) {
        return defaultValue;
    }

    if (parsed < minValue) {
        return minValue;
    }
    if (parsed > maxValue) {
        return maxValue;
    }
    return static_cast<int>(parsed);
}

uint32_t ReadBridgeU32(const char* key, uint32_t defaultValue)
{
    char value[64]{};
    if (!ReadSmallTextValue(g_configPath, key, value, sizeof(value))) {
        return defaultValue;
    }

    char* p = value;
    while (*p == ' ' || *p == '\t') {
        ++p;
    }

    char* end = nullptr;
    const unsigned long parsed = std::strtoul(p, &end, 0);
    if (end == p) {
        return defaultValue;
    }
    return static_cast<uint32_t>(parsed);
}

void ReadBridgeText(const char* key, char* out, size_t outSize, const char* defaultValue) {
    if (!out || outSize == 0) {
        return;
    }
    out[0] = '\0';
    if (!ReadSmallTextValue(g_configPath, key, out, outSize) && defaultValue) {
        strncpy_s(out, outSize, defaultValue, _TRUNCATE);
    }
}

int ReadFlaInt(const char* key, int defaultValue, int minValue, int maxValue)
{
    char value[64]{};
    if (!ReadSmallTextValue(g_flaIniPath, key, value, sizeof(value))) {
        return defaultValue;
    }

    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (end == value) {
        return defaultValue;
    }

    if (parsed < minValue) {
        return minValue;
    }
    if (parsed > maxValue) {
        return maxValue;
    }
    return static_cast<int>(parsed);
}

void LoadBridgeConfig()
{
    WriteDefaultBridgeConfigIfMissing();

    g_config.enableBridge = ReadBridgeBool("EnableBridge", g_config.enableBridge);
    g_config.enableDiagnosticsGroup = ReadBridgeBool("EnableDiagnosticsGroup", g_config.enableDiagnosticsGroup);
    g_config.enableFLACompat = ReadBridgeBool("EnableFLACompat", g_config.enableFLACompat);
    g_config.enableModCompat = ReadBridgeBool("EnableModCompat", g_config.enableModCompat);
    g_config.enableCLEOPlusCompat = ReadBridgeBool("EnableCLEOPlusCompat", g_config.enableCLEOPlusCompat);
    g_config.enableUrbanizeCompat = ReadBridgeBool("EnableUrbanizeCompat", g_config.enableUrbanizeCompat);
    g_config.enableTaxi77Compat = ReadBridgeBool("EnableTaxi77Compat", g_config.enableTaxi77Compat);
    g_config.enableSanPabloCompat = ReadBridgeBool("EnableSanPabloCompat", g_config.enableSanPabloCompat);
    g_config.enableMixSetsCompat = ReadBridgeBool("EnableMixSetsCompat", g_config.enableMixSetsCompat);

    g_config.enableVectoredExceptionHandler = ReadBridgeBool("EnableVectoredExceptionHandler", g_config.enableVectoredExceptionHandler);
    g_config.enableExceptionDiagnostics = ReadBridgeBool("EnableExceptionDiagnostics", g_config.enableExceptionDiagnostics);
    g_config.enableStackScanDiagnostics = ReadBridgeBool("EnableStackScanDiagnostics", g_config.enableStackScanDiagnostics);
    g_config.enableMemoryRegionDiagnostics = ReadBridgeBool("EnableMemoryRegionDiagnostics", g_config.enableMemoryRegionDiagnostics);
    g_config.enableBoundCentreVehRecovery = ReadBridgeBool("EnableBoundCentreVehRecovery", g_config.enableBoundCentreVehRecovery);
    g_config.enablePtrNodeExhaustionGuard = ReadBridgeBool("EnablePtrNodeExhaustionGuard", g_config.enablePtrNodeExhaustionGuard);
    g_config.enableExceptionLoopBreaker = ReadBridgeBool("EnableExceptionLoopBreaker", g_config.enableExceptionLoopBreaker);
    g_config.exceptionLoopBreakerTerminate = ReadBridgeBool("ExceptionLoopBreakerTerminate", g_config.exceptionLoopBreakerTerminate);
    g_config.exceptionLoopBreakerThreshold = ReadBridgeInt("ExceptionLoopBreakerThreshold", g_config.exceptionLoopBreakerThreshold, 2, 10000);
    g_config.maxExceptionLogs = ReadBridgeInt("MaxExceptionLogs", g_config.maxExceptionLogs, 0, 256);

    g_config.enableModuleSnapshot = ReadBridgeBool("EnableModuleSnapshot", g_config.enableModuleSnapshot);
    g_config.enableRiskConstantScan = ReadBridgeBool("EnableRiskConstantScan", g_config.enableRiskConstantScan);
    g_config.enableRelocatedAddressDiagnostics = ReadBridgeBool("EnableRelocatedAddressDiagnostics", g_config.enableRelocatedAddressDiagnostics);
    g_config.enablePoolPointerDiagnostics = ReadBridgeBool("EnablePoolPointerDiagnostics", g_config.enablePoolPointerDiagnostics);
    g_config.enablePopulationPoolDiagnostics = ReadBridgeBool("EnablePopulationPoolDiagnostics", g_config.enablePopulationPoolDiagnostics);
    g_config.populationPoolDiagStartDelayMs = ReadBridgeInt("PopulationPoolDiagStartDelayMs", g_config.populationPoolDiagStartDelayMs, 0, 300000);
    g_config.populationPoolDiagIntervalMs = ReadBridgeInt("PopulationPoolDiagIntervalMs", g_config.populationPoolDiagIntervalMs, 250, 60000);
    g_config.populationPoolDiagIterations = ReadBridgeInt("PopulationPoolDiagIterations", g_config.populationPoolDiagIterations, 0, 10000);
    g_config.enableGangOnlyPopulationGuard = ReadBridgeBool("EnableGangOnlyPopulationGuard", g_config.enableGangOnlyPopulationGuard);
    g_config.gangOnlyPopulationGuardStartDelayMs = ReadBridgeInt("GangOnlyPopulationGuardStartDelayMs", g_config.gangOnlyPopulationGuardStartDelayMs, 0, 300000);
    g_config.gangOnlyPopulationGuardIntervalMs = ReadBridgeInt("GangOnlyPopulationGuardIntervalMs", g_config.gangOnlyPopulationGuardIntervalMs, 250, 60000);
    g_config.gangOnlyPopulationGuardIterations = ReadBridgeInt("GangOnlyPopulationGuardIterations", g_config.gangOnlyPopulationGuardIterations, 0, 10000);
    g_config.gangOnlyPopulationGuardClearCheatFlag = ReadBridgeBool("GangOnlyPopulationGuardClearCheatFlag", g_config.gangOnlyPopulationGuardClearCheatFlag);
    g_config.enablePedStreamingZoneRepair = ReadBridgeBool("EnablePedStreamingZoneRepair", g_config.enablePedStreamingZoneRepair);
    g_config.pedStreamingZoneRepairCallOriginal = ReadBridgeBool("PedStreamingZoneRepairCallOriginal", g_config.pedStreamingZoneRepairCallOriginal);
    g_config.pedStreamingZoneRepairStartDelayMs = ReadBridgeInt("PedStreamingZoneRepairStartDelayMs", g_config.pedStreamingZoneRepairStartDelayMs, 0, 300000);
    g_config.pedStreamingZoneRepairIntervalMs = ReadBridgeInt("PedStreamingZoneRepairIntervalMs", g_config.pedStreamingZoneRepairIntervalMs, 250, 60000);
    g_config.pedStreamingZoneRepairIterations = ReadBridgeInt("PedStreamingZoneRepairIterations", g_config.pedStreamingZoneRepairIterations, 0, 10000);
    g_config.pedStreamingZoneRepairMaxCalls = ReadBridgeInt("PedStreamingZoneRepairMaxCalls", g_config.pedStreamingZoneRepairMaxCalls, 0, 10000);
    g_config.pedStreamingZoneRepairMaxLogs = ReadBridgeInt("PedStreamingZoneRepairMaxLogs", g_config.pedStreamingZoneRepairMaxLogs, 0, 10000);
    if (g_config.pedStreamingZoneRepairCallOriginal) {
        Log("ped zone repair config: PedStreamingZoneRepairCallOriginal is ignored; off-thread GTA streaming calls are disabled");
        g_config.pedStreamingZoneRepairCallOriginal = false;
    }
    g_config.enableStreamingBusyThresholdPatch = ReadBridgeBool("EnableStreamingBusyThresholdPatch", g_config.enableStreamingBusyThresholdPatch);
    g_config.streamingBusyThreshold = ReadBridgeInt("StreamingBusyThreshold", g_config.streamingBusyThreshold, 5, 10000);
    g_config.enablePopulationUpdateBudgetPatch = ReadBridgeBool("EnablePopulationUpdateBudgetPatch", g_config.enablePopulationUpdateBudgetPatch);
    g_config.populationUpdateBudgetMs = ReadBridgeInt("PopulationUpdateBudgetMs", g_config.populationUpdateBudgetMs, 1, 127);
    g_config.enableCrashClassification = ReadBridgeBool("EnableCrashClassification", g_config.enableCrashClassification);
    g_config.enableRuntimeRewriteAudit = ReadBridgeBool("EnableRuntimeRewriteAudit", g_config.enableRuntimeRewriteAudit);
    g_config.enableOpenLimitAdjusterOverlapAudit = ReadBridgeBool("EnableOpenLimitAdjusterOverlapAudit", g_config.enableOpenLimitAdjusterOverlapAudit);
    g_config.enableFlaPathNodeDiagnostics = ReadBridgeBool("EnableFlaPathNodeDiagnostics", g_config.enableFlaPathNodeDiagnostics);
    g_config.enableOpenLimitAdjusterSaLimitGuard = ReadBridgeBool("EnableOpenLimitAdjusterSaLimitGuard", g_config.enableOpenLimitAdjusterSaLimitGuard);
    g_config.enableOpenLimitAdjusterModuleGuard = ReadBridgeBool("EnableOpenLimitAdjusterModuleGuard", g_config.enableOpenLimitAdjusterModuleGuard);
    ReadBridgeText("OpenLimitAdjusterSaLimitAllowlist", g_config.openLimitAdjusterSaLimitAllowlist, sizeof(g_config.openLimitAdjusterSaLimitAllowlist));
    g_config.enableModulePolicy = ReadBridgeBool("EnableModulePolicy", g_config.enableModulePolicy);
    ReadBridgeText("LegacyModuleAllowlist", g_config.legacyModuleAllowlist, sizeof(g_config.legacyModuleAllowlist), ".cleo;.asi;modloader");
    ReadBridgeText("ModernModuleDenylist", g_config.modernModuleDenylist, sizeof(g_config.modernModuleDenylist),
        "SilentPatch;WidescreenFix;WindowedMode;CrashInfo;modloader.asi;MixSets;FLACompatBridge;fastman92;DINPUT8;vorbis;_noDEP;rundll32exefix;iii.vc.sa.limitadjuster;ImgLimitAdjuster");
    ReadBridgeText("ForceNoRuntimeRewrite", g_config.forceNoRuntimeRewrite, sizeof(g_config.forceNoRuntimeRewrite));
    ReadBridgeText("ForceNoAutoPoolGuard", g_config.forceNoAutoPoolGuard, sizeof(g_config.forceNoAutoPoolGuard));
    g_config.enableProperShadersCompat = ReadBridgeBool("EnableProperShadersCompat", g_config.enableProperShadersCompat);
    g_config.enableRuntimeRewrite = ReadBridgeBool("EnableRuntimeRewrite", g_config.enableRuntimeRewrite);
    g_config.enableRuntimeRewriteRescan = ReadBridgeBool("EnableRuntimeRewriteRescan", g_config.enableRuntimeRewriteRescan);
    g_config.enableRuntimeRewriteRuleTable = ReadBridgeBool("EnableRuntimeRewriteRuleTable", g_config.enableRuntimeRewriteRuleTable);
    g_config.runtimeRewriteStartDelayMs = ReadBridgeInt("RuntimeRewriteStartDelayMs", g_config.runtimeRewriteStartDelayMs, 0, 30000);
    g_config.runtimeRewriteIterations = ReadBridgeInt("RuntimeRewriteIterations", g_config.runtimeRewriteIterations, 0, 10000);
    g_config.runtimeRewriteIntervalMs = ReadBridgeInt("RuntimeRewriteIntervalMs", g_config.runtimeRewriteIntervalMs, 100, 60000);
    g_config.runtimeRewriteRuleCount = ReadBridgeInt("RuntimeRewriteRuleCount", g_config.runtimeRewriteRuleCount, 0, static_cast<int>(kMaxRuntimeRewriteRules));
    g_config.runtimeRewriteDefaultMaxPatchesPerModule = ReadBridgeInt("RuntimeRewriteDefaultMaxPatchesPerModule", g_config.runtimeRewriteDefaultMaxPatchesPerModule, 1, 100000);
    ReadSmallTextValue(g_configPath, "RuntimeRewriteAllowlist", g_config.runtimeRewriteAllowlist, sizeof(g_config.runtimeRewriteAllowlist));
    ReadSmallTextValue(g_configPath, "RuntimeRewriteDenylist", g_config.runtimeRewriteDenylist, sizeof(g_config.runtimeRewriteDenylist));
    LoadRuntimeRewriteRules();

    g_config.enableLegacyModelInfoShadow = ReadBridgeBool("EnableLegacyModelInfoShadow", g_config.enableLegacyModelInfoShadow);
    g_config.enableLegacyStreamingInfoShadow = ReadBridgeBool("EnableLegacyStreamingInfoShadow", g_config.enableLegacyStreamingInfoShadow);
    g_config.enableFlaExtendedIdApi = ReadBridgeBool("EnableFlaExtendedIdApi", g_config.enableFlaExtendedIdApi);
    g_config.legacyShadowStartDelayMs = ReadBridgeInt("LegacyShadowStartDelayMs", g_config.legacyShadowStartDelayMs, 0, 30000);
    g_config.legacyShadowIterations = ReadBridgeInt("LegacyShadowIterations", g_config.legacyShadowIterations, 0, 10000);
    g_config.legacyShadowIntervalMs = ReadBridgeInt("LegacyShadowIntervalMs", g_config.legacyShadowIntervalMs, 10, 10000);

    g_config.enableCObjectCreateBridge = ReadBridgeBool("EnableCObjectCreateBridge", g_config.enableCObjectCreateBridge);
    g_config.enableCleoObjectCreateInlineRestore = ReadBridgeBool("EnableCleoObjectCreateInlineRestore", g_config.enableCleoObjectCreateInlineRestore);
    g_config.enableCleoDispatchGuard = ReadBridgeBool("EnableCleoDispatchGuard", g_config.enableCleoDispatchGuard);
    g_config.enableCleoDispatchLazyPoolRecovery = ReadBridgeBool("EnableCleoDispatchLazyPoolRecovery", g_config.enableCleoDispatchLazyPoolRecovery);
    g_config.enableCleoThunk26720Guard = ReadBridgeBool("EnableCleoThunk26720Guard", g_config.enableCleoThunk26720Guard);
    g_config.enableCleoPlusExtendedObjectVarGuard = ReadBridgeBool("EnableCleoPlusExtendedObjectVarGuard", g_config.enableCleoPlusExtendedObjectVarGuard);
    g_config.enableCleoPlusPoolAllocateGuard = ReadBridgeBool("EnableCleoPlusPoolAllocateGuard", g_config.enableCleoPlusPoolAllocateGuard);
    g_config.enableMixSetsPoolAllocateGuard = ReadBridgeBool("EnableMixSetsPoolAllocateGuard", g_config.enableMixSetsPoolAllocateGuard);
    g_config.enableUrbanizePoolAllocateGuard = ReadBridgeBool("EnableUrbanizePoolAllocateGuard", g_config.enableUrbanizePoolAllocateGuard);
    g_config.enableVehFuncsPoolAllocateGuard = ReadBridgeBool("EnableVehFuncsPoolAllocateGuard", g_config.enableVehFuncsPoolAllocateGuard);
    g_config.enableAutoPoolAllocateGuard = ReadBridgeBool("EnableAutoPoolAllocateGuard", g_config.enableAutoPoolAllocateGuard);
    g_config.enableDeferredPoolAllocateReplay = ReadBridgeBool("EnableDeferredPoolAllocateReplay", g_config.enableDeferredPoolAllocateReplay);
    g_config.autoPoolAllocateGuardMaxPatches = ReadBridgeInt("AutoPoolAllocateGuardMaxPatches", g_config.autoPoolAllocateGuardMaxPatches, 0, 10000);
    g_config.deferredPoolAllocateReplayIterations = ReadBridgeInt("DeferredPoolAllocateReplayIterations", g_config.deferredPoolAllocateReplayIterations, 0, 100000);
    g_config.deferredPoolAllocateReplayIntervalMs = ReadBridgeInt("DeferredPoolAllocateReplayIntervalMs", g_config.deferredPoolAllocateReplayIntervalMs, 10, 10000);
    ReadSmallTextValue(g_configPath, "AutoPoolAllocateGuardAllowlist", g_config.autoPoolAllocateGuardAllowlist, sizeof(g_config.autoPoolAllocateGuardAllowlist));
    ReadSmallTextValue(g_configPath, "AutoPoolAllocateGuardDenylist", g_config.autoPoolAllocateGuardDenylist, sizeof(g_config.autoPoolAllocateGuardDenylist));
    g_config.enableAnimUncompressGuard = ReadBridgeBool("EnableAnimUncompressGuard", g_config.enableAnimUncompressGuard);
    g_config.enableAnimStaticAssocGuard = ReadBridgeBool("EnableAnimStaticAssocGuard", g_config.enableAnimStaticAssocGuard);
    g_config.enableAnimFrameUpdateGuard = ReadBridgeBool("EnableAnimFrameUpdateGuard", g_config.enableAnimFrameUpdateGuard);
    g_config.enableAnimEmptyUpdateGuard = ReadBridgeBool("EnableAnimEmptyUpdateGuard", g_config.enableAnimEmptyUpdateGuard);
    g_config.enableAnimLifecycleDiagnostics = ReadBridgeBool("EnableAnimLifecycleDiagnostics", g_config.enableAnimLifecycleDiagnostics);
    g_config.enableRpAnimBlendClumpInitGuard = ReadBridgeBool("EnableRpAnimBlendClumpInitGuard", g_config.enableRpAnimBlendClumpInitGuard);
    g_config.enableRwClumpForAllAtomicsGuard = ReadBridgeBool("EnableRwClumpForAllAtomicsGuard", g_config.enableRwClumpForAllAtomicsGuard);
    g_config.enableShouldModelBeStreamedGuard = ReadBridgeBool("EnableShouldModelBeStreamedGuard", g_config.enableShouldModelBeStreamedGuard);
    g_config.enableGetBoundCentreInlineGuard = ReadBridgeBool("EnableGetBoundCentreInlineGuard", g_config.enableGetBoundCentreInlineGuard);
    g_config.enableGetBoundRectColModelGuard = ReadBridgeBool("EnableGetBoundRectColModelGuard", g_config.enableGetBoundRectColModelGuard);
    g_config.enablePlaceableRemoveMatrixGuard = ReadBridgeBool("EnablePlaceableRemoveMatrixGuard", g_config.enablePlaceableRemoveMatrixGuard);
    g_config.enablePlaceableRemoveMatrixSkipGuard = ReadBridgeBool("EnablePlaceableRemoveMatrixSkipGuard", g_config.enablePlaceableRemoveMatrixSkipGuard);
    g_config.enablePlaceableStaticMatrixAllocGuard = ReadBridgeBool("EnablePlaceableStaticMatrixAllocGuard", g_config.enablePlaceableStaticMatrixAllocGuard);
    g_config.matrixGuardSignatureCheck = ReadBridgeBool("MatrixGuardSignatureCheck", g_config.matrixGuardSignatureCheck);
    g_config.matrixGuardRecoverStaticList = ReadBridgeBool("MatrixGuardRecoverStaticList", g_config.matrixGuardRecoverStaticList);
    g_config.matrixGuardListBase = ReadBridgeU32("MatrixGuardListBase", g_config.matrixGuardListBase);
    g_config.matrixGuardRemoveMatrixEntry = ReadBridgeU32("MatrixGuardRemoveMatrixEntry", g_config.matrixGuardRemoveMatrixEntry);
    g_config.matrixGuardRemoveMatrixNullLoad = ReadBridgeU32("MatrixGuardRemoveMatrixNullLoad", g_config.matrixGuardRemoveMatrixNullLoad);
    g_config.matrixGuardAllocateStaticOwnerWrite = ReadBridgeU32("MatrixGuardAllocateStaticOwnerWrite", g_config.matrixGuardAllocateStaticOwnerWrite);
    g_config.matrixGuardAllocateStaticResume = ReadBridgeU32("MatrixGuardAllocateStaticResume", g_config.matrixGuardAllocateStaticResume);
    g_config.matrixGuardAllocateStaticMin = ReadBridgeU32("MatrixGuardAllocateStaticMin", g_config.matrixGuardAllocateStaticMin);
    g_config.matrixGuardAllocateStaticMax = ReadBridgeU32("MatrixGuardAllocateStaticMax", g_config.matrixGuardAllocateStaticMax);
    g_config.enableUrbanizeProblemPedPreload = ReadBridgeBool("EnableUrbanizeProblemPedPreload", g_config.enableUrbanizeProblemPedPreload);
    g_config.enableFlaNoCollisionErrorRestore = ReadBridgeBool("EnableFlaNoCollisionErrorRestore", g_config.enableFlaNoCollisionErrorRestore);
    g_config.enableBridgeCheatStringLoader = ReadBridgeBool("EnableBridgeCheatStringLoader", g_config.enableBridgeCheatStringLoader);
    g_config.enableFlaTrainInitHookRepair = ReadBridgeBool("EnableFlaTrainInitHookRepair", g_config.enableFlaTrainInitHookRepair);
    g_config.enableFlaObjectInitCollisionRestore = ReadBridgeBool("EnableFlaObjectInitCollisionRestore", g_config.enableFlaObjectInitCollisionRestore);
    g_config.enablePickupModelLoadGuard = ReadBridgeBool("EnablePickupModelLoadGuard", g_config.enablePickupModelLoadGuard);
    g_config.pickupModelLoadGuardProtectHighIds = ReadBridgeBool("PickupModelLoadGuardProtectHighIds", g_config.pickupModelLoadGuardProtectHighIds);
    ReadBridgeText("PickupModelLoadGuardAllowlist", g_config.pickupModelLoadGuardAllowlist, sizeof(g_config.pickupModelLoadGuardAllowlist), "1277;pickupsave");
    g_config.pickupModelLoadGuardFlags = ReadBridgeInt("PickupModelLoadGuardFlags", g_config.pickupModelLoadGuardFlags, 0, 0xFF);
    g_config.pickupModelLoadGuardLoadNow = ReadBridgeBool("PickupModelLoadGuardLoadNow", g_config.pickupModelLoadGuardLoadNow);
    g_config.pickupModelLoadGuardMaxLogs = ReadBridgeInt("PickupModelLoadGuardMaxLogs", g_config.pickupModelLoadGuardMaxLogs, 0, 100000);
    g_config.enableColAccelStartCachePoolGuard = ReadBridgeBool("EnableColAccelStartCachePoolGuard", g_config.enableColAccelStartCachePoolGuard);
    g_config.enableColModelPoolNewGuard = ReadBridgeBool("EnableColModelPoolNewGuard", g_config.enableColModelPoolNewGuard);
    g_config.enableLazyCPoolRegistry = ReadBridgeBool("EnableLazyCPoolRegistry", g_config.enableLazyCPoolRegistry);
    g_config.enableBatchLazyCPoolInitialise = ReadBridgeBool("EnableBatchLazyCPoolInitialise", g_config.enableBatchLazyCPoolInitialise);
    g_config.enableReplayPoolReadSkipGuard = ReadBridgeBool("EnableReplayPoolReadSkipGuard", g_config.enableReplayPoolReadSkipGuard);
    g_config.enableEarlyCPoolsInitialiseRecovery = ReadBridgeBool("EnableEarlyCPoolsInitialiseRecovery", g_config.enableEarlyCPoolsInitialiseRecovery);
    g_config.enableCPoolsInitialiseRecovery = ReadBridgeBool("EnableCPoolsInitialiseRecovery", g_config.enableCPoolsInitialiseRecovery);
    g_config.enableRadarBlipHandleGuard = ReadBridgeBool("EnableRadarBlipHandleGuard", g_config.enableRadarBlipHandleGuard);
    g_config.enableCleoTargetBlipCoordsBridge = ReadBridgeBool("EnableCleoTargetBlipCoordsBridge", g_config.enableCleoTargetBlipCoordsBridge);
    g_config.enableRadarTraceRuntimeRewrite = ReadBridgeBool("EnableRadarTraceRuntimeRewrite", g_config.enableRadarTraceRuntimeRewrite);
    g_config.enableClosestCarNode03D3Fallback = ReadBridgeBool("EnableClosestCarNode03D3Fallback", g_config.enableClosestCarNode03D3Fallback);
    g_config.enableTaxi77SetCarCoordinatesGuard = ReadBridgeBool("EnableTaxi77SetCarCoordinatesGuard", g_config.enableTaxi77SetCarCoordinatesGuard);
    g_config.enableTaxi77StateWatchdog = ReadBridgeBool("EnableTaxi77StateWatchdog", g_config.enableTaxi77StateWatchdog);
    g_config.enableTaxi77StateWatchdogRecovery = ReadBridgeBool("EnableTaxi77StateWatchdogRecovery", g_config.enableTaxi77StateWatchdogRecovery);
    g_config.enableSanPabloSpecialActorBridge = ReadBridgeBool("EnableSanPabloSpecialActorBridge", g_config.enableSanPabloSpecialActorBridge);
    g_config.specialActorMinModelId = ReadBridgeInt("MinModelId", g_config.specialActorMinModelId, 0, 1000000);
    g_config.specialActorMaxModelId = ReadBridgeInt("MaxModelId", g_config.specialActorMaxModelId, 0, 1000000);
    g_config.specialActorAutoDetectExtendedSlots = ReadBridgeBool("AutoDetectExtendedSlots", g_config.specialActorAutoDetectExtendedSlots);
    g_config.specialActorAutoMaxModelId = ReadBridgeInt("AutoMaxModelId", g_config.specialActorAutoMaxModelId, 0, 1000000);
    g_config.specialActorAutoScanImgArchives = ReadBridgeBool("AutoScanImgArchives", g_config.specialActorAutoScanImgArchives);
    g_config.specialActorAutoScanModloader = ReadBridgeBool("AutoScanModloader", g_config.specialActorAutoScanModloader);
    g_config.specialActorAllowMissingTxd = ReadBridgeBool("AllowMissingTxd", g_config.specialActorAllowMissingTxd);
    g_config.specialActorRequireModelInfoSlot = ReadBridgeBool("RequireModelInfoSlot", g_config.specialActorRequireModelInfoSlot);
    g_config.specialActorLogCatalog = ReadBridgeBool("LogCatalog", g_config.specialActorLogCatalog);
    g_config.specialActorAutoCatalogFirstCode = ReadBridgeInt("AutoCatalogFirstCode", g_config.specialActorAutoCatalogFirstCode, 1, 999);
    g_config.specialActorMaxAutoCatalogNames = ReadBridgeInt("MaxAutoCatalogNames", g_config.specialActorMaxAutoCatalogNames, 0, static_cast<int>(kMaxRuntimeSpecialActorNames));
    g_config.specialActorCatalogBuildDelayMs = ReadBridgeInt("CatalogBuildDelayMs", g_config.specialActorCatalogBuildDelayMs, 0, 60000);
    ReadSmallTextValue(g_configPath, "AutoScanFilter", g_config.specialActorAutoScanFilter, sizeof(g_config.specialActorAutoScanFilter));
    if (!g_config.specialActorAutoScanFilter[0]) {
        strncpy_s(g_config.specialActorAutoScanFilter, sizeof(g_config.specialActorAutoScanFilter),
            "SWEET;RYDER;SMOKE;SMOKEV;CESAR;ZERO;TENPEN;PULASKI;TRUTH;OGLOC;KENDL;JIZZY;MADDOGG;MACCER;WUZIMU;EMMET;JETHRO;JANITOR;CLAUDE;FORELLI;ANDRE;BB;BBTHIN;CAT;CROGRL;DNB;ROSE;SUZIE;TBONE;TORINO;HMOGAR",
            _TRUNCATE);
    }
    g_config.taxi77WatchdogPollMs = ReadBridgeInt("Taxi77WatchdogPollMs", g_config.taxi77WatchdogPollMs, 250, 60000);
    g_config.taxi77MainStuckSeconds = ReadBridgeInt("Taxi77MainStuckSeconds", g_config.taxi77MainStuckSeconds, 10, 3600);
    g_config.taxi77StartLabelOffset = ReadBridgeInt("Taxi77StartLabelOffset", g_config.taxi77StartLabelOffset, 0, 0xFFFF);
    g_config.taxi77ActiveMinOffset = ReadBridgeInt("Taxi77ActiveMinOffset", g_config.taxi77ActiveMinOffset, 0, 0xFFFF);
    g_config.taxi77ActiveMaxOffset = ReadBridgeInt("Taxi77ActiveMaxOffset", g_config.taxi77ActiveMaxOffset, 0, 0xFFFF);

    if (!g_config.enableBridge) {
        g_config.enableDiagnosticsGroup = false;
        g_config.enableFLACompat = false;
        g_config.enableModCompat = false;
        g_config.enableCLEOPlusCompat = false;
        g_config.enableUrbanizeCompat = false;
        g_config.enableTaxi77Compat = false;
        g_config.enableSanPabloCompat = false;
        g_config.enableMixSetsCompat = false;
    }

    if (!g_config.enableDiagnosticsGroup) {
        g_config.enableVectoredExceptionHandler = false;
        g_config.enableExceptionDiagnostics = false;
        g_config.enableBoundCentreVehRecovery = false;
        g_config.enablePtrNodeExhaustionGuard = false;
        g_config.enableExceptionLoopBreaker = false;
        g_config.enableModuleSnapshot = false;
        g_config.enableRiskConstantScan = false;
        g_config.enableRelocatedAddressDiagnostics = false;
        g_config.enablePoolPointerDiagnostics = false;
        g_config.enablePopulationPoolDiagnostics = false;
        g_config.enableGangOnlyPopulationGuard = false;
        g_config.enablePedStreamingZoneRepair = false;
        g_config.enableStreamingBusyThresholdPatch = false;
        g_config.enableCrashClassification = false;
        g_config.enableRuntimeRewriteAudit = false;
        g_config.enableOpenLimitAdjusterOverlapAudit = false;
        g_config.enableFlaPathNodeDiagnostics = false;
    }

    if (!g_config.enableFLACompat) {
        g_config.enableLegacyModelInfoShadow = false;
        g_config.enableLegacyStreamingInfoShadow = false;
        g_config.enableFlaExtendedIdApi = false;
        g_config.enableFlaNoCollisionErrorRestore = false;
        g_config.enableBridgeCheatStringLoader = false;
        g_config.enableFlaTrainInitHookRepair = false;
        g_config.enableFlaObjectInitCollisionRestore = false;
        g_config.enablePickupModelLoadGuard = false;
        g_config.enableColAccelStartCachePoolGuard = false;
        g_config.enableColModelPoolNewGuard = false;
        g_config.enableLazyCPoolRegistry = false;
        g_config.enableBatchLazyCPoolInitialise = false;
        g_config.enableReplayPoolReadSkipGuard = false;
        g_config.enableEarlyCPoolsInitialiseRecovery = false;
        g_config.enableCPoolsInitialiseRecovery = false;
    }

    if (!g_config.enableModCompat) {
        g_config.enableCLEOPlusCompat = false;
        g_config.enableUrbanizeCompat = false;
        g_config.enableTaxi77Compat = false;
        g_config.enableSanPabloCompat = false;
        g_config.enableMixSetsCompat = false;
        g_config.enableRuntimeRewrite = false;
        g_config.enableRuntimeRewriteRescan = false;
        g_config.enableRuntimeRewriteRuleTable = false;
        g_config.enableCObjectCreateBridge = false;
        g_config.enableVehFuncsPoolAllocateGuard = false;
        g_config.enableAutoPoolAllocateGuard = false;
        g_config.enableDeferredPoolAllocateReplay = false;
        g_config.enableAnimUncompressGuard = false;
        g_config.enableAnimStaticAssocGuard = false;
        g_config.enableAnimFrameUpdateGuard = false;
        g_config.enableAnimEmptyUpdateGuard = false;
        g_config.enableAnimLifecycleDiagnostics = false;
        g_config.enableRpAnimBlendClumpInitGuard = false;
        g_config.enableRwClumpForAllAtomicsGuard = false;
        g_config.enableShouldModelBeStreamedGuard = false;
        g_config.enableGetBoundCentreInlineGuard = false;
        g_config.enableGetBoundRectColModelGuard = false;
        g_config.enablePlaceableRemoveMatrixGuard = false;
        g_config.enableRadarBlipHandleGuard = false;
        g_config.enableCleoTargetBlipCoordsBridge = false;
        g_config.enableRadarTraceRuntimeRewrite = false;
        g_config.enableClosestCarNode03D3Fallback = false;
        g_config.enableSanPabloSpecialActorBridge = false;
    }

    if (!g_config.enableCLEOPlusCompat) {
        g_config.enableCleoObjectCreateInlineRestore = false;
        g_config.enableCleoDispatchGuard = false;
        g_config.enableCleoDispatchLazyPoolRecovery = false;
        g_config.enableCleoThunk26720Guard = false;
        g_config.enableCleoPlusExtendedObjectVarGuard = false;
        g_config.enableCleoPlusPoolAllocateGuard = false;
    }

    if (!g_config.enableUrbanizeCompat) {
        g_config.enableUrbanizePoolAllocateGuard = false;
        g_config.enableUrbanizeProblemPedPreload = false;
    }

    if (!g_config.enableTaxi77Compat) {
        g_config.enableTaxi77SetCarCoordinatesGuard = false;
        g_config.enableTaxi77StateWatchdog = false;
        g_config.enableTaxi77StateWatchdogRecovery = false;
    }

    if (!g_config.enableSanPabloCompat) {
        g_config.enableSanPabloSpecialActorBridge = false;
    }
    if (!g_config.enableMixSetsCompat) {
        g_config.enableMixSetsPoolAllocateGuard = false;
    }

    if (g_config.specialActorMaxModelId < g_config.specialActorMinModelId) {
        g_config.specialActorMaxModelId = g_config.specialActorMinModelId;
    }
    if (g_config.specialActorAutoMaxModelId < g_config.specialActorMaxModelId) {
        g_config.specialActorAutoMaxModelId = g_config.specialActorMaxModelId;
    }
}

void LogBridgeConfig()
{
    Log("bridge config: general bridge=%d diagnosticsGroup=%d flaCompat=%d modCompat=%d cleoPlusCompat=%d urbanizeCompat=%d taxi77Compat=%d sanPabloCompat=%d",
        g_config.enableBridge ? 1 : 0,
        g_config.enableDiagnosticsGroup ? 1 : 0,
        g_config.enableFLACompat ? 1 : 0,
        g_config.enableModCompat ? 1 : 0,
        g_config.enableCLEOPlusCompat ? 1 : 0,
        g_config.enableUrbanizeCompat ? 1 : 0,
        g_config.enableTaxi77Compat ? 1 : 0,
        g_config.enableSanPabloCompat ? 1 : 0);
    Log("bridge config: modulePolicy=%d legacyAllowlist='%s' modernDenylist='%s' forceNoRuntimeRewrite='%s' forceNoAutoPoolGuard='%s' properShadersCompat=%d",
        g_config.enableModulePolicy ? 1 : 0,
        g_config.legacyModuleAllowlist,
        g_config.modernModuleDenylist,
        g_config.forceNoRuntimeRewrite,
        g_config.forceNoAutoPoolGuard,
        g_config.enableProperShadersCompat ? 1 : 0);
    Log("bridge config: VEH=%d exceptionDiag=%d boundCentreVEH=%d ptrNodeGuard=%d loopBreaker=%d loopTerminate=%d loopThreshold=%d maxExceptionLogs=%d moduleSnapshot=%d riskScan=%d relocatedDiag=%d poolDiag=%d populationPoolDiag=%d populationDelay=%d populationInterval=%d populationIterations=%d crashClass=%d rewriteAudit=%d olaOverlapAudit=%d pathNodeDiag=%d olaSaGuard=%d olaModuleGuard=%d olaSaAllowlist='%s' rewrite=%d rewriteRescan=%d rewriteRuleTable=%d rewriteRules=%u rescanDelay=%d rescanIterations=%d rescanInterval=%d defaultMaxPatches=%d allowlist='%s' denylist='%s'",
        g_config.enableVectoredExceptionHandler ? 1 : 0,
        g_config.enableExceptionDiagnostics ? 1 : 0,
        g_config.enableBoundCentreVehRecovery ? 1 : 0,
        g_config.enablePtrNodeExhaustionGuard ? 1 : 0,
        g_config.enableExceptionLoopBreaker ? 1 : 0,
        g_config.exceptionLoopBreakerTerminate ? 1 : 0,
        g_config.exceptionLoopBreakerThreshold,
        g_config.maxExceptionLogs,
        g_config.enableModuleSnapshot ? 1 : 0,
        g_config.enableRiskConstantScan ? 1 : 0,
        g_config.enableRelocatedAddressDiagnostics ? 1 : 0,
        g_config.enablePoolPointerDiagnostics ? 1 : 0,
        g_config.enablePopulationPoolDiagnostics ? 1 : 0,
        g_config.populationPoolDiagStartDelayMs,
        g_config.populationPoolDiagIntervalMs,
        g_config.populationPoolDiagIterations,
        g_config.enableCrashClassification ? 1 : 0,
        g_config.enableRuntimeRewriteAudit ? 1 : 0,
        g_config.enableOpenLimitAdjusterOverlapAudit ? 1 : 0,
        g_config.enableFlaPathNodeDiagnostics ? 1 : 0,
        g_config.enableOpenLimitAdjusterSaLimitGuard ? 1 : 0,
        g_config.enableOpenLimitAdjusterModuleGuard ? 1 : 0,
        g_config.openLimitAdjusterSaLimitAllowlist,
        g_config.enableRuntimeRewrite ? 1 : 0,
        g_config.enableRuntimeRewriteRescan ? 1 : 0,
        g_config.enableRuntimeRewriteRuleTable ? 1 : 0,
        g_runtimeRewriteRuleCount,
        g_config.runtimeRewriteStartDelayMs,
        g_config.runtimeRewriteIterations,
        g_config.runtimeRewriteIntervalMs,
        g_config.runtimeRewriteDefaultMaxPatchesPerModule,
        g_config.runtimeRewriteAllowlist,
        g_config.runtimeRewriteDenylist);
    Log("bridge config: gangOnlyPopulationGuard=%d guardDelay=%d guardInterval=%d guardIterations=%d clearGangLandCheat=%d pedZoneRepair=%d pedZoneCallOriginal=%d pedZoneDelay=%d pedZoneInterval=%d pedZoneIterations=%d pedZoneMaxCalls=%d pedZoneMaxLogs=%d streamingBusyPatch=%d streamingBusyThreshold=%d populationBudgetPatch=%d populationBudgetMs=%d",
        g_config.enableGangOnlyPopulationGuard ? 1 : 0,
        g_config.gangOnlyPopulationGuardStartDelayMs,
        g_config.gangOnlyPopulationGuardIntervalMs,
        g_config.gangOnlyPopulationGuardIterations,
        g_config.gangOnlyPopulationGuardClearCheatFlag ? 1 : 0,
        g_config.enablePedStreamingZoneRepair ? 1 : 0,
        g_config.pedStreamingZoneRepairCallOriginal ? 1 : 0,
        g_config.pedStreamingZoneRepairStartDelayMs,
        g_config.pedStreamingZoneRepairIntervalMs,
        g_config.pedStreamingZoneRepairIterations,
        g_config.pedStreamingZoneRepairMaxCalls,
        g_config.pedStreamingZoneRepairMaxLogs,
        g_config.enableStreamingBusyThresholdPatch ? 1 : 0,
        g_config.streamingBusyThreshold,
        g_config.enablePopulationUpdateBudgetPatch ? 1 : 0,
        g_config.populationUpdateBudgetMs);
    Log("bridge config: modelShadow=%d streamingShadow=%d flaExtendedIdApi=%d shadowDelay=%d shadowIterations=%d shadowInterval=%d",
        g_config.enableLegacyModelInfoShadow ? 1 : 0,
        g_config.enableLegacyStreamingInfoShadow ? 1 : 0,
        g_config.enableFlaExtendedIdApi ? 1 : 0,
        g_config.legacyShadowStartDelayMs,
        g_config.legacyShadowIterations,
        g_config.legacyShadowIntervalMs);
    Log("bridge config: specialActorSlots=[%d,%d] autoDetect=%d autoMax=%d autoScanImg=%d autoScanModloader=%d allowMissingTxd=%d requireModelInfo=%d autoFirstCode=%d maxAutoNames=%d catalogDelayMs=%d filter='%s'",
        g_config.specialActorMinModelId,
        g_config.specialActorMaxModelId,
        g_config.specialActorAutoDetectExtendedSlots ? 1 : 0,
        g_config.specialActorAutoMaxModelId,
        g_config.specialActorAutoScanImgArchives ? 1 : 0,
        g_config.specialActorAutoScanModloader ? 1 : 0,
        g_config.specialActorAllowMissingTxd ? 1 : 0,
        g_config.specialActorRequireModelInfoSlot ? 1 : 0,
        g_config.specialActorAutoCatalogFirstCode,
        g_config.specialActorMaxAutoCatalogNames,
        g_config.specialActorCatalogBuildDelayMs,
        g_config.specialActorAutoScanFilter);
    Log("bridge config: cobjectBridge=%d cleoObjectRestore=%d cleoDispatch=%d cleoDispatchLazyPoolRecovery=%d cleoThunk=%d cleoExtObjVarGuard=%d cleoPoolAllocGuard=%d mixSetsPoolAllocGuard=%d urbanizePoolAllocGuard=%d vehFuncsPoolAllocGuard=%d animUncompress=%d animStatic=%d animFrame=%d rpAnimClumpInit=%d rwClump=%d shouldStream=%d boundCentreInline=%d boundRectColModel=%d placeableRemoveMatrix=%d urbanizePedPreload=%d flaNoCollisionRestore=%d bridgeCheatLoader=%d trainInitRepair=%d flaObjectInitRestore=%d colAccelPoolGuard=%d colModelPoolNewGuard=%d lazyCPoolRegistry=%d batchLazyCPools=%d replayPoolSkip=%d earlyCPoolsRecovery=%d cPoolsRecovery=%d radarBlipGuard=%d cleoTargetBlipBridge=%d radarTraceRewrite=%d closestCarNode03D3=%d taxi77SetCarGuard=%d taxi77Watchdog=%d taxi77Recovery=%d sanPabloSpecialActor=%d taxi77PollMs=%d taxi77StuckSeconds=%d taxi77Start=0x%X taxi77Active=[0x%X,0x%X)",
        g_config.enableCObjectCreateBridge ? 1 : 0,
        g_config.enableCleoObjectCreateInlineRestore ? 1 : 0,
        g_config.enableCleoDispatchGuard ? 1 : 0,
        g_config.enableCleoDispatchLazyPoolRecovery ? 1 : 0,
        g_config.enableCleoThunk26720Guard ? 1 : 0,
        g_config.enableCleoPlusExtendedObjectVarGuard ? 1 : 0,
        g_config.enableCleoPlusPoolAllocateGuard ? 1 : 0,
        g_config.enableMixSetsPoolAllocateGuard ? 1 : 0,
        g_config.enableUrbanizePoolAllocateGuard ? 1 : 0,
        g_config.enableVehFuncsPoolAllocateGuard ? 1 : 0,
        g_config.enableAnimUncompressGuard ? 1 : 0,
        g_config.enableAnimStaticAssocGuard ? 1 : 0,
        g_config.enableAnimFrameUpdateGuard ? 1 : 0,
        g_config.enableRpAnimBlendClumpInitGuard ? 1 : 0,
        g_config.enableRwClumpForAllAtomicsGuard ? 1 : 0,
        g_config.enableShouldModelBeStreamedGuard ? 1 : 0,
        g_config.enableGetBoundCentreInlineGuard ? 1 : 0,
        g_config.enableGetBoundRectColModelGuard ? 1 : 0,
        g_config.enablePlaceableRemoveMatrixGuard ? 1 : 0,
        g_config.enableUrbanizeProblemPedPreload ? 1 : 0,
        g_config.enableFlaNoCollisionErrorRestore ? 1 : 0,
        g_config.enableBridgeCheatStringLoader ? 1 : 0,
        g_config.enableFlaTrainInitHookRepair ? 1 : 0,
        g_config.enableFlaObjectInitCollisionRestore ? 1 : 0,
        g_config.enableColAccelStartCachePoolGuard ? 1 : 0,
        g_config.enableColModelPoolNewGuard ? 1 : 0,
        g_config.enableLazyCPoolRegistry ? 1 : 0,
        g_config.enableBatchLazyCPoolInitialise ? 1 : 0,
        g_config.enableReplayPoolReadSkipGuard ? 1 : 0,
        g_config.enableEarlyCPoolsInitialiseRecovery ? 1 : 0,
        g_config.enableCPoolsInitialiseRecovery ? 1 : 0,
        g_config.enableRadarBlipHandleGuard ? 1 : 0,
        g_config.enableCleoTargetBlipCoordsBridge ? 1 : 0,
        g_config.enableRadarTraceRuntimeRewrite ? 1 : 0,
        g_config.enableClosestCarNode03D3Fallback ? 1 : 0,
        g_config.enableTaxi77SetCarCoordinatesGuard ? 1 : 0,
        g_config.enableTaxi77StateWatchdog ? 1 : 0,
        g_config.enableTaxi77StateWatchdogRecovery ? 1 : 0,
        g_config.enableSanPabloSpecialActorBridge ? 1 : 0,
        g_config.taxi77WatchdogPollMs,
        g_config.taxi77MainStuckSeconds,
        g_config.taxi77StartLabelOffset,
        g_config.taxi77ActiveMinOffset,
        g_config.taxi77ActiveMaxOffset);
    Log("bridge config: animHeavyFrameGuard=%d animEmptyUpdateGuard=%d animLifecycleDiagnostics=%d",
        g_config.enableAnimFrameUpdateGuard ? 1 : 0,
        g_config.enableAnimEmptyUpdateGuard ? 1 : 0,
        g_config.enableAnimLifecycleDiagnostics ? 1 : 0);
    Log("bridge config: pickupModelLoadGuard=%d pickupProtectHighIds=%d pickupAllowlist='%s' pickupFlags=0x%X pickupLoadNow=%d pickupMaxLogs=%d",
        g_config.enablePickupModelLoadGuard ? 1 : 0,
        g_config.pickupModelLoadGuardProtectHighIds ? 1 : 0,
        g_config.pickupModelLoadGuardAllowlist,
        g_config.pickupModelLoadGuardFlags,
        g_config.pickupModelLoadGuardLoadNow ? 1 : 0,
        g_config.pickupModelLoadGuardMaxLogs);
}

void LogIniValue(const char* key)
{
    char value[128]{};
    if (ReadSmallTextValue(g_flaIniPath, key, value, sizeof(value))) {
        Log("FLA ini: %s = %s", key, value);
    }
}


