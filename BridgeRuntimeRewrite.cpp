#include "FLACompatBridgeInternal.h"

void ScanRiskyAddressConstants(const char* modulePath)
{
    struct Pattern {
        const char* name;
        uint8_t bytes[4];
    };

    static const Pattern patterns[] = {
        { "CModelInfo::ms_modelInfoPtrs 0xA9B0C8", { 0xC8, 0xB0, 0xA9, 0x00 } },
        { "CStreaming::ms_aInfoForModel 0x8E4CC0", { 0xC0, 0x4C, 0x8E, 0x00 } },
        { "CAnimManager::ms_aAnimBlocks 0xB5D4A0", { 0xA0, 0xD4, 0xB5, 0x00 } },
        { "CRadar::ms_RadarTrace 0xBA86F0", { 0xF0, 0x86, 0xBA, 0x00 } },
        { "CDarkel::RegisteredKills 0x969A50", { 0x50, 0x9A, 0x96, 0x00 } },
        { "CPools::ms_pPedPool 0xB74490", { 0x90, 0x44, 0xB7, 0x00 } },
        { "CPools::ms_pVehiclePool 0xB74494", { 0x94, 0x44, 0xB7, 0x00 } },
        { "CPools::ms_pObjectPool 0xB7449C", { 0x9C, 0x44, 0xB7, 0x00 } },
    };

    bool any = false;
    for (const auto& pattern : patterns) {
        if (FileContainsPattern(modulePath, pattern.bytes, sizeof(pattern.bytes))) {
            Log("risk constant hit: %s -> %s", BaseName(modulePath), pattern.name);
            any = true;
        }
    }

    if (!any) {
        const char* base = BaseName(modulePath);
        if (strstr(base, ".asi") || strstr(base, ".cleo")) {
            Log("risk constant clean: %s", base);
        }
    }
}

bool ModuleMatchesRewriteList(const char* moduleName, const char* modulePath, const char* rawList)
{
    if (!rawList || !rawList[0]) {
        return false;
    }

    char list[512]{};
    strncpy_s(list, rawList, _TRUNCATE);

    char* context = nullptr;
    for (char* token = strtok_s(list, ";,", &context); token; token = strtok_s(nullptr, ";,", &context)) {
        while (*token == ' ' || *token == '\t') {
            ++token;
        }
        char* end = token + std::strlen(token);
        while (end > token && (end[-1] == ' ' || end[-1] == '\t')) {
            --end;
        }
        *end = '\0';
        if (!*token) {
            continue;
        }
        if (ContainsCaseInsensitive(moduleName, token) || ContainsCaseInsensitive(modulePath, token)) {
            return true;
        }
    }

    return false;
}

bool ModuleDeniedByCompatPolicy(const char* moduleName, const char* modulePath)
{
    if (!g_config.enableModulePolicy) {
        return false;
    }
    return ModuleMatchesRewriteList(moduleName, modulePath, g_config.modernModuleDenylist);
}

bool ModuleAllowedByLegacyPolicy(const char* moduleName, const char* modulePath)
{
    if (!g_config.enableModulePolicy) {
        return true;
    }
    if (ModuleDeniedByCompatPolicy(moduleName, modulePath)) {
        return false;
    }
    if (!g_config.legacyModuleAllowlist[0]) {
        return true;
    }
    return ModuleMatchesRewriteList(moduleName, modulePath, g_config.legacyModuleAllowlist);
}

bool ModuleAllowedForAutoPoolGuard(const char* moduleName, const char* modulePath)
{
    if (!moduleName || !modulePath) {
        return false;
    }
    if (ModuleDeniedByCompatPolicy(moduleName, modulePath) ||
        ModuleMatchesRewriteList(moduleName, modulePath, g_config.forceNoAutoPoolGuard) ||
        ModuleMatchesRewriteList(moduleName, modulePath, g_config.autoPoolAllocateGuardDenylist)) {
        return false;
    }
    if (!ModuleAllowedByLegacyPolicy(moduleName, modulePath)) {
        return false;
    }
    if (!g_config.autoPoolAllocateGuardAllowlist[0]) {
        return false;
    }
    return ModuleMatchesRewriteList(moduleName, modulePath, g_config.autoPoolAllocateGuardAllowlist);
}

bool ModuleDeniedForRuntimeRewrite(const char* moduleName, const char* modulePath)
{
    return ModuleDeniedByCompatPolicy(moduleName, modulePath) ||
        ModuleMatchesRewriteList(moduleName, modulePath, g_config.forceNoRuntimeRewrite) ||
        ModuleMatchesRewriteList(moduleName, modulePath, g_config.runtimeRewriteDenylist);
}

bool ModuleAllowedForRuntimeRewrite(const char* moduleName, const char* modulePath)
{
    return ModuleAllowedByLegacyPolicy(moduleName, modulePath) &&
        ModuleMatchesRewriteList(moduleName, modulePath, g_config.runtimeRewriteAllowlist) &&
        !ModuleDeniedForRuntimeRewrite(moduleName, modulePath);
}

RuntimeRewriteTarget ParseRuntimeRewriteTarget(const char* raw)
{
    if (!raw || !raw[0]) {
        return RUNTIME_REWRITE_TARGET_STATIC;
    }
    if (_stricmp(raw, "CModelInfo") == 0 || _stricmp(raw, "ModelInfo") == 0 ||
        _stricmp(raw, "CModelInfo::ms_modelInfoPtrs") == 0) {
        return RUNTIME_REWRITE_TARGET_MODEL_INFO;
    }
    if (_stricmp(raw, "CStreaming") == 0 || _stricmp(raw, "StreamingInfo") == 0 ||
        _stricmp(raw, "CStreaming::ms_aInfoForModel") == 0) {
        return RUNTIME_REWRITE_TARGET_STREAMING_INFO;
    }
    if (_stricmp(raw, "RadarTrace") == 0 || _stricmp(raw, "CRadar::ms_RadarTrace") == 0) {
        return RUNTIME_REWRITE_TARGET_RADAR_TRACE;
    }
    if (_stricmp(raw, "AnimBlocks") == 0 || _stricmp(raw, "CAnimManager::ms_aAnimBlocks") == 0) {
        return RUNTIME_REWRITE_TARGET_ANIM_BLOCKS;
    }
    if (_stricmp(raw, "StreamedScripts") == 0 || _stricmp(raw, "CTheScripts::StreamedScripts") == 0) {
        return RUNTIME_REWRITE_TARGET_STREAMED_SCRIPTS;
    }
    if (_stricmp(raw, "HandlingManager") == 0 || _stricmp(raw, "mod_HandlingManager") == 0) {
        return RUNTIME_REWRITE_TARGET_HANDLING_MANAGER;
    }
    if (_stricmp(raw, "VehicleRecording") == 0 || _stricmp(raw, "CVehicleRecording::StreamingArray") == 0) {
        return RUNTIME_REWRITE_TARGET_VEHICLE_RECORDING;
    }
    return RUNTIME_REWRITE_TARGET_STATIC;
}

const char* RuntimeRewriteTargetName(RuntimeRewriteTarget target)
{
    switch (target) {
    case RUNTIME_REWRITE_TARGET_MODEL_INFO: return "CModelInfo";
    case RUNTIME_REWRITE_TARGET_STREAMING_INFO: return "CStreaming";
    case RUNTIME_REWRITE_TARGET_RADAR_TRACE: return "RadarTrace";
    case RUNTIME_REWRITE_TARGET_ANIM_BLOCKS: return "AnimBlocks";
    case RUNTIME_REWRITE_TARGET_STREAMED_SCRIPTS: return "StreamedScripts";
    case RUNTIME_REWRITE_TARGET_HANDLING_MANAGER: return "HandlingManager";
    case RUNTIME_REWRITE_TARGET_VEHICLE_RECORDING: return "VehicleRecording";
    default: return "Static";
    }
}

uint32_t ResolveRuntimeRewriteNewValue(const RuntimeRewriteRule& rule)
{
    switch (rule.target) {
    case RUNTIME_REWRITE_TARGET_MODEL_INFO:
        return static_cast<uint32_t>(g_relocatedCModelInfoPtrs);
    case RUNTIME_REWRITE_TARGET_STREAMING_INFO:
        return static_cast<uint32_t>(g_relocatedStreamingInfo);
    case RUNTIME_REWRITE_TARGET_RADAR_TRACE:
        if (!g_config.enableRadarTraceRuntimeRewrite || !g_radarTraceBase || g_radarTraceBase == kOriginalRadarTrace) {
            return 0;
        }
        return static_cast<uint32_t>(g_radarTraceBase);
    case RUNTIME_REWRITE_TARGET_ANIM_BLOCKS:
        return static_cast<uint32_t>(g_relocatedAnimBlocks);
    case RUNTIME_REWRITE_TARGET_STREAMED_SCRIPTS:
        return static_cast<uint32_t>(g_relocatedStreamedScripts);
    case RUNTIME_REWRITE_TARGET_HANDLING_MANAGER:
        return static_cast<uint32_t>(g_relocatedHandlingManager);
    case RUNTIME_REWRITE_TARGET_VEHICLE_RECORDING:
        return static_cast<uint32_t>(g_relocatedVehicleRecordingStreamingArray);
    default:
        return rule.staticNewValue;
    }
}

void AddRuntimeRewriteRule(
    const char* name,
    uint32_t oldValue,
    RuntimeRewriteTarget target,
    uint32_t staticNewValue,
    bool align4,
    bool executableOnly,
    bool auditOnly,
    uint32_t maxPatchesPerModule,
    const char* allowlist = "",
    const char* denylist = "")
{
    if (g_runtimeRewriteRuleCount >= kMaxRuntimeRewriteRules || !oldValue) {
        return;
    }

    RuntimeRewriteRule& rule = g_runtimeRewriteRules[g_runtimeRewriteRuleCount++];
    rule.enabled = true;
    rule.auditOnly = auditOnly;
    rule.align4 = align4;
    rule.executableOnly = executableOnly;
    rule.oldValue = oldValue;
    rule.target = target;
    rule.staticNewValue = staticNewValue;
    rule.maxPatchesPerModule = maxPatchesPerModule ? maxPatchesPerModule : static_cast<uint32_t>(g_config.runtimeRewriteDefaultMaxPatchesPerModule);
    strncpy_s(rule.name, name && name[0] ? name : RuntimeRewriteTargetName(target), _TRUNCATE);
    strncpy_s(rule.allowlist, allowlist ? allowlist : "", _TRUNCATE);
    strncpy_s(rule.denylist, denylist ? denylist : "", _TRUNCATE);
}

void AddBuiltInRuntimeRewriteRules()
{
    AddRuntimeRewriteRule("CModelInfo::ms_modelInfoPtrs", static_cast<uint32_t>(kOriginalCModelInfoPtrs),
        RUNTIME_REWRITE_TARGET_MODEL_INFO, 0, true, true, false, 256);
    AddRuntimeRewriteRule("CStreaming::ms_aInfoForModel", static_cast<uint32_t>(kOriginalStreamingInfo),
        RUNTIME_REWRITE_TARGET_STREAMING_INFO, 0, true, true, false, 256);
    AddRuntimeRewriteRule("CRadar::ms_RadarTrace", static_cast<uint32_t>(kOriginalRadarTrace),
        RUNTIME_REWRITE_TARGET_RADAR_TRACE, 0, true, true, false, 512);
}

void LoadRuntimeRewriteRules()
{
    std::memset(g_runtimeRewriteRules, 0, sizeof(g_runtimeRewriteRules));
    g_runtimeRewriteRuleCount = 0;

    if (!g_config.enableRuntimeRewriteRuleTable || g_config.runtimeRewriteRuleCount <= 0) {
        AddBuiltInRuntimeRewriteRules();
        return;
    }

    for (int i = 1; i <= g_config.runtimeRewriteRuleCount && g_runtimeRewriteRuleCount < kMaxRuntimeRewriteRules; ++i) {
        char prefix[32]{};
        sprintf_s(prefix, "Rule%03d", i);

        char key[96]{};
        sprintf_s(key, "%sEnabled", prefix);
        if (!ReadBridgeBool(key, true)) {
            continue;
        }

        sprintf_s(key, "%sOld", prefix);
        const uint32_t oldValue = ReadBridgeU32(key, 0);
        if (!oldValue) {
            Log("runtime rewrite rule: skipped %s missing/invalid Old", prefix);
            continue;
        }

        char name[64]{};
        sprintf_s(key, "%sName", prefix);
        ReadBridgeText(key, name, sizeof(name), prefix);

        char targetText[64]{};
        sprintf_s(key, "%sTarget", prefix);
        ReadBridgeText(key, targetText, sizeof(targetText), "Static");

        sprintf_s(key, "%sNew", prefix);
        const uint32_t staticNewValue = ReadBridgeU32(key, 0);

        sprintf_s(key, "%sAlign4", prefix);
        const bool align4 = ReadBridgeBool(key, true);
        sprintf_s(key, "%sExecutableOnly", prefix);
        const bool requestedExecutableOnly = ReadBridgeBool(key, true);
        if (!requestedExecutableOnly) {
            Log("runtime rewrite rule: %s requested data-page scanning; forcing executable-only safety policy", prefix);
        }
        sprintf_s(key, "%sAuditOnly", prefix);
        const bool auditOnly = ReadBridgeBool(key, false);
        sprintf_s(key, "%sMaxPatchesPerModule", prefix);
        const uint32_t maxPatches = static_cast<uint32_t>(ReadBridgeInt(key, g_config.runtimeRewriteDefaultMaxPatchesPerModule, 1, 100000));

        char allowlist[256]{};
        sprintf_s(key, "%sAllowlist", prefix);
        ReadBridgeText(key, allowlist, sizeof(allowlist));
        char denylist[256]{};
        sprintf_s(key, "%sDenylist", prefix);
        ReadBridgeText(key, denylist, sizeof(denylist));

        AddRuntimeRewriteRule(name, oldValue, ParseRuntimeRewriteTarget(targetText), staticNewValue,
            align4, true, auditOnly, maxPatches, allowlist, denylist);
    }

    if (!g_runtimeRewriteRuleCount) {
        AddBuiltInRuntimeRewriteRules();
    }

    Log("runtime rewrite rules: loaded count=%u source=%s configuredCount=%d",
        g_runtimeRewriteRuleCount,
        g_config.enableRuntimeRewriteRuleTable && g_config.runtimeRewriteRuleCount > 0 ? "ini" : "built-in",
        g_config.runtimeRewriteRuleCount);
}

bool RuleDeniedForRuntimeRewrite(const RuntimeRewriteRule& rule, const char* moduleName, const char* modulePath)
{
    return ModuleDeniedForRuntimeRewrite(moduleName, modulePath) ||
        ModuleMatchesRewriteList(moduleName, modulePath, rule.denylist);
}

bool RuleAllowedForRuntimeRewrite(const RuntimeRewriteRule& rule, const char* moduleName, const char* modulePath)
{
    const char* allow = rule.allowlist[0] ? rule.allowlist : g_config.runtimeRewriteAllowlist;
    return ModuleMatchesRewriteList(moduleName, modulePath, allow) &&
        !RuleDeniedForRuntimeRewrite(rule, moduleName, modulePath);
}

bool ModuleInterestingForRuntimeRewrite(const char* moduleName, const char* modulePath)
{
    if (!moduleName || !modulePath) {
        return false;
    }
    if (ContainsCaseInsensitive(moduleName, "FLACompatBridge") ||
        ContainsCaseInsensitive(moduleName, "fastman92") ||
        ContainsCaseInsensitive(moduleName, "DINPUT8") ||
        ContainsCaseInsensitive(moduleName, "vorbis")) {
        return false;
    }

    if (ModuleAllowedForRuntimeRewrite(moduleName, modulePath) ||
        ModuleDeniedForRuntimeRewrite(moduleName, modulePath)) {
        return true;
    }

    for (uint32_t i = 0; i < g_runtimeRewriteRuleCount; ++i) {
        const RuntimeRewriteRule& rule = g_runtimeRewriteRules[i];
        if (!rule.enabled) {
            continue;
        }
        if (RuleAllowedForRuntimeRewrite(rule, moduleName, modulePath) ||
            RuleDeniedForRuntimeRewrite(rule, moduleName, modulePath)) {
            return true;
        }
    }

    return strstr(moduleName, ".asi") || strstr(moduleName, ".cleo") || strstr(moduleName, "modloader") ||
        strstr(moduleName, "SilentPatch") || strstr(moduleName, "MixSets") || strstr(moduleName, "Urbanize") ||
        ContainsCaseInsensitive(modulePath, "modloader");
}

void RewriteOneModuleConstants(const MODULEENTRY32& me, bool logDetails)
{
    const bool denied = ModuleDeniedForRuntimeRewrite(me.szModule, me.szExePath);
    const bool allowed = ModuleAllowedForRuntimeRewrite(me.szModule, me.szExePath);
    uintptr_t base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
    uintptr_t end = base + me.modBaseSize;
    uintptr_t cursor = base;
    uint32_t hits = 0;
    uint32_t patched = 0;
    uint32_t rulePatched[kMaxRuntimeRewriteRules]{};

    while (cursor < end) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(reinterpret_cast<const void*>(cursor), &mbi, sizeof(mbi))) {
            cursor += 0x1000;
            continue;
        }

        const uintptr_t regionBase = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        const uintptr_t regionEnd = regionBase + mbi.RegionSize;
        cursor = regionEnd;

        if (mbi.State != MEM_COMMIT || (mbi.Protect & PAGE_NOACCESS) || (mbi.Protect & PAGE_GUARD)) {
            continue;
        }

        const bool regionExecutable = IsExecutableProtect(mbi.Protect);
        if (!regionExecutable) {
            continue;
        }
        uintptr_t scanStart = regionBase < base ? base : regionBase;
        uintptr_t scanEnd = regionEnd > end ? end : regionEnd;
        if (scanEnd <= scanStart || scanEnd - scanStart < sizeof(uint32_t)) {
            continue;
        }

        const size_t size = scanEnd - scanStart;
        uint8_t* buffer = new uint8_t[size];
        bool copied = false;
        __try {
            std::memcpy(buffer, reinterpret_cast<const void*>(scanStart), size);
            copied = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            copied = false;
        }
        if (!copied) {
            delete[] buffer;
            continue;
        }

        for (size_t i = 0; i + sizeof(uint32_t) <= size; ++i) {
            uint32_t value = 0;
            std::memcpy(&value, buffer + i, sizeof(value));
            for (uint32_t ruleIndex = 0; ruleIndex < g_runtimeRewriteRuleCount; ++ruleIndex) {
                RuntimeRewriteRule& rule = g_runtimeRewriteRules[ruleIndex];
                const uint32_t newValue = ResolveRuntimeRewriteNewValue(rule);
                if (!rule.enabled || !newValue || value != rule.oldValue) {
                    continue;
                }
                const uintptr_t patchAddress = scanStart + i;
                if (rule.align4 && (patchAddress & 3)) {
                    continue;
                }
                ++hits;
                const bool ruleDenied = RuleDeniedForRuntimeRewrite(rule, me.szModule, me.szExePath);
                const bool ruleAllowed = RuleAllowedForRuntimeRewrite(rule, me.szModule, me.szExePath);
                if (logDetails && hits <= 64) {
                    Log("runtime rewrite %s: module=%s rule=%s target=%s address=0x%08X old=0x%08X new=0x%08X allowed=%d denied=%d auditOnly=%d",
                        g_config.enableRuntimeRewrite && !rule.auditOnly ? "candidate" : "audit",
                        me.szModule,
                        rule.name,
                        RuntimeRewriteTargetName(rule.target),
                        patchAddress,
                        rule.oldValue,
                        newValue,
                        ruleAllowed ? 1 : 0,
                        ruleDenied ? 1 : 0,
                        rule.auditOnly ? 1 : 0);
                }
                if (g_config.enableRuntimeRewrite && !rule.auditOnly && ruleAllowed &&
                    rulePatched[ruleIndex] < rule.maxPatchesPerModule &&
                    WriteBytesWithProtect(patchAddress, reinterpret_cast<const uint8_t*>(&newValue), sizeof(newValue))) {
                    ++patched;
                    ++rulePatched[ruleIndex];
                    Log("runtime rewrite patched: module=%s rule=%s target=%s address=0x%08X countForRule=%u",
                        me.szModule, rule.name, RuntimeRewriteTargetName(rule.target), patchAddress, rulePatched[ruleIndex]);
                }
            }
        }

        delete[] buffer;
    }

    if ((hits || patched) && (logDetails || patched)) {
        Log("runtime rewrite summary: module=%s hits=%u patched=%u allowed=%d denied=%d",
            me.szModule, hits, patched, allowed ? 1 : 0, denied ? 1 : 0);
    }
}

void RuntimeRewriteHardcodedAddressConstants(bool logDetails)
{
    if (!g_config.enableRuntimeRewriteAudit && !g_config.enableRuntimeRewrite) {
        return;
    }

    if (g_config.enableRuntimeRewrite && !g_config.runtimeRewriteAllowlist[0]) {
        Log("runtime rewrite: enabled but RuntimeRewriteAllowlist is empty; refusing to patch modules");
    }

    bool hasResolvableRule = false;
    for (uint32_t i = 0; i < g_runtimeRewriteRuleCount; ++i) {
        if (g_runtimeRewriteRules[i].enabled && ResolveRuntimeRewriteNewValue(g_runtimeRewriteRules[i]) != 0) {
            hasResolvableRule = true;
            break;
        }
    }

    if (!hasResolvableRule) {
        Log("runtime rewrite: relocated tables unavailable");
        return;
    }

    MODULEENTRY32 me{};
    me.dwSize = sizeof(me);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE) {
        Log("runtime rewrite: module snapshot failed gle=%lu", GetLastError());
        return;
    }

    if (logDetails) {
        Log("runtime rewrite begin: audit=%d rewrite=%d allowlist='%s' denylist='%s'",
            g_config.enableRuntimeRewriteAudit ? 1 : 0,
            g_config.enableRuntimeRewrite ? 1 : 0,
            g_config.runtimeRewriteAllowlist,
            g_config.runtimeRewriteDenylist);
        Log("runtime rewrite state: modelInfo=0x%08X streaming=0x%08X radarTrace=0x%08X radarLimit=%u radarTraceRewrite=%d",
            g_relocatedCModelInfoPtrs,
            g_relocatedStreamingInfo,
            g_radarTraceBase,
            g_radarTraceLimit,
            g_config.enableRadarTraceRuntimeRewrite ? 1 : 0);
    }

    if (Module32First(snapshot, &me)) {
        do {
            const char* name = me.szModule;
            const char* path = me.szExePath;
            if (!ModuleInterestingForRuntimeRewrite(name, path)) {
                continue;
            }
            RewriteOneModuleConstants(me, logDetails);
        } while (Module32Next(snapshot, &me));
    }

    CloseHandle(snapshot);
    if (logDetails) {
        Log("runtime rewrite end");
    }
}

DWORD WINAPI RuntimeRewriteRescanThread(void*)
{
    if (!g_config.enableRuntimeRewriteRescan ||
        (!g_config.enableRuntimeRewriteAudit && !g_config.enableRuntimeRewrite) ||
        g_config.runtimeRewriteIterations <= 0) {
        return 0;
    }

    Sleep(static_cast<DWORD>(g_config.runtimeRewriteStartDelayMs));
    Log("runtime rewrite rescan: begin iterations=%d intervalMs=%d",
        g_config.runtimeRewriteIterations, g_config.runtimeRewriteIntervalMs);

    for (int i = 0; i < g_config.runtimeRewriteIterations; ++i) {
        RuntimeRewriteHardcodedAddressConstants(false);
        Sleep(static_cast<DWORD>(g_config.runtimeRewriteIntervalMs));
    }

    Log("runtime rewrite rescan: end");
    return 0;
}

