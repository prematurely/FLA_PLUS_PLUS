#include "FLACompatBridgeInternal.h"

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_CObject_Create_5A1FA1()
{
    __asm
    {
        push 0;       // bCreate
        push edi;     // modelID
        mov ecx, esi; // this

        push continueTheCode;
        push 0x5A1D70; // CObject::CObject(CObject *this, int modelID, char bCreate)
        retn;

    continueTheCode:
        mov eax, esi;
        push 0x5A1FCE;
        retn;
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_CObject_Create_5A2016()
{
    __asm
    {
        push 0;       // bCreate
        push edi;     // modelID
        mov ecx, esi; // this

        push 0x5A203D;
        push 0x5A1D70; // CObject::CObject(CObject *this, int modelID, char bCreate)
        retn;
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_ShouldModelBeStreamed_ColModelGuard()
{
    __asm
    {
        cmp eax, 10000h
        jb invalidColModel

        fld dword ptr [eax + 0x24]
        fadd dword ptr [esp + 0x20]
        push dword ptr [g_shouldModelBeStreamedContinue]
        retn

    invalidColModel:
        pushad
        push dword ptr [esp + 12]
        push eax
        push edi
        push esi
        call Bridge_LogInvalidShouldModelBeStreamedColModel
        popad

        push dword ptr [g_shouldModelBeStreamedReturnFalse]
        retn
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_GetBoundCentre_NullGuard()
{
    __asm
    {
        pushad

        mov eax, [esp + 36] // original [esp+4], hidden/out CVector*
        push 12
        push eax
        call Bridge_IsWritableMemory
        test al, al
        jz invalidOut

        popad
        movsx eax, word ptr [ecx + 0x22]
        mov edx, dword ptr [eax * 4 + 0x00A9B0C8]
        push 0x0053365B
        retn

    invalidOut:
        mov eax, [esp + 36] // out vector
        mov edx, [esp + 32] // return address
        mov ecx, [esp + 24] // this/entity
        mov ebx, [esp + 12] // original esp
        push ebx
        push edx
        push eax
        push ecx
        call Bridge_LogInvalidBoundCentreOut

        popad
        xor eax, eax
        ret 4
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_GetBoundRect_ColModelGuard()
{
    __asm
    {
        // FLA has already resolved the extended model ID and loaded m_pColModel
        // into EAX at this point. Only replace the missing collision model; doing
        // the lookup here again puts FLA's extension map on every bounds query.
        test eax, eax
        jnz validColModel
        mov eax, offset g_fakeColModelForBounds

    validColModel:
        mov edx, [eax]
        mov [esp + 0x10], edx
        push 0x0053413A
        retn
    }
}
#endif

bool ClearZoneStreamingCheatsForMask(uint32_t mask)
{
    bool allCleared = true;
    const uint8_t zero = 0;
    for (const auto& flag : kZoneStreamingCheatFlags) {
        if ((mask & flag.mask) == 0) {
            continue;
        }
        if (!WriteBytesWithProtect(kCheatsActive + flag.index, &zero, sizeof(zero))) {
            allCleared = false;
        }
    }
    return allCleared;
}

DWORD WINAPI GangOnlyPopulationGuardThread(void*)
{
    if (!g_config.enableGangOnlyPopulationGuard || g_config.gangOnlyPopulationGuardIterations <= 0) {
        return 0;
    }

    Sleep(static_cast<DWORD>(g_config.gangOnlyPopulationGuardStartDelayMs));

    bool poolsReady = false;
    uint32_t pedPool = 0;
    uint32_t vehiclePool = 0;
    uint32_t objectPool = 0;
    uint32_t colModelPool = 0;
    for (int waitAttempt = 0; waitAttempt < 1800; ++waitAttempt) {
        if (AreCorePoolsReadyForDeferredReplay(&pedPool, &vehiclePool, &objectPool, &colModelPool)) {
            poolsReady = true;
            Log("population guard: core pools ready after waitAttempt=%d ped=0x%08X vehicle=0x%08X object=0x%08X colModel=0x%08X",
                waitAttempt,
                pedPool,
                vehiclePool,
                objectPool,
                colModelPool);
            break;
        }

        if (waitAttempt == 0 || waitAttempt == 30 || waitAttempt == 120 || (waitAttempt % 300) == 0) {
            Log("population guard: waiting for core pools waitAttempt=%d ped=0x%08X vehicle=0x%08X object=0x%08X colModel=0x%08X",
                waitAttempt,
                pedPool,
                vehiclePool,
                objectPool,
                colModelPool);
        }
        Sleep(1000);
    }

    if (!poolsReady) {
        Log("population guard: core pools still not ready; continuing runtime flag watch");
    }

    int lastOnlyGang = -2;
    int lastCheatGangLand = -2;
    uint32_t lastZoneStreamingMask = 0xFFFFFFFFu;
    int clearCount = 0;

    for (int i = 0; i < g_config.gangOnlyPopulationGuardIterations; ++i) {
        const int sample = i + 1;
        const int onlyGang = ReadRuntimeByteForLog(kPopulationOnlyCreateRandomGangMembers);
        const int cheatGangLand = ReadRuntimeByteForLog(kCheatsActive + kCheatGangsControlStreets);
        const uint32_t zoneStreamingMask = ReadZoneStreamingCheatMaskForLog();
        const uint32_t msNumPedsLoaded = ReadRuntimeU32ForLog(kStreamingNumPedsLoaded);
        const uint32_t streamZone = ReadRuntimeU32ForLog(kStreamingCurrentZoneType);
        const uint32_t popZone = ReadRuntimeU32ForLog(kPopCycleCurrentZoneType);
        const bool changed = onlyGang != lastOnlyGang || cheatGangLand != lastCheatGangLand || zoneStreamingMask != lastZoneStreamingMask;
        const bool shouldClearOnlyGang = onlyGang > 0;
        const bool shouldClearCheat = g_config.gangOnlyPopulationGuardClearCheatFlag && cheatGangLand > 0;
        const bool shouldClearZoneCheats =
            g_config.gangOnlyPopulationGuardClearCheatFlag &&
            zoneStreamingMask != 0 &&
            msNumPedsLoaded == 0 &&
            streamZone == 0xFFFFFFFFu &&
            popZone != 0xFFFFFFFFu;

        if (sample == 1 || sample <= 3 || changed || shouldClearOnlyGang || shouldClearCheat || shouldClearZoneCheats || (sample % 15) == 0) {
            LogPopulationRuntimeState("gang-only-guard", sample);
        }

        if (g_config.enablePopulationPoolDiagnostics &&
            (sample == 1 || changed || shouldClearOnlyGang || shouldClearCheat || shouldClearZoneCheats || (sample % 15) == 0)) {
            LogPopulationStreamingPedSlots("gang-only-guard", sample);
            LogPedPoolModelTypeSummary("gang-only-guard", sample);
        }

        if (shouldClearOnlyGang || shouldClearCheat || shouldClearZoneCheats) {
            bool clearedOnlyGang = false;
            bool clearedCheat = false;
            bool clearedZoneCheats = false;
            const uint8_t zero = 0;
            if (shouldClearOnlyGang) {
                clearedOnlyGang = WriteBytesWithProtect(kPopulationOnlyCreateRandomGangMembers, &zero, sizeof(zero));
            }
            if (shouldClearCheat) {
                clearedCheat = WriteBytesWithProtect(kCheatsActive + kCheatGangsControlStreets, &zero, sizeof(zero));
            }
            if (shouldClearZoneCheats) {
                clearedZoneCheats = ClearZoneStreamingCheatsForMask(zoneStreamingMask);
            }
            ++clearCount;
            Log("population guard: cleared stuck population state sample=%d clearCount=%d beforeOnlyGang=%d beforeCheatGangLand=%d beforeZoneCheats=0x%02X msPeds=%u streamZone=%u popZone=%u clearedOnlyGang=%d clearedCheatGangLand=%d clearedZoneCheats=%d",
                sample,
                clearCount,
                onlyGang,
                cheatGangLand,
                zoneStreamingMask,
                msNumPedsLoaded,
                streamZone,
                popZone,
                clearedOnlyGang ? 1 : 0,
                clearedCheat ? 1 : 0,
                clearedZoneCheats ? 1 : 0);
            LogPopulationRuntimeState("gang-only-guard-after-clear", sample);
            if (g_config.enablePopulationPoolDiagnostics) {
                LogPopulationStreamingPedSlots("gang-only-guard-after-clear", sample);
                LogPedPoolModelTypeSummary("gang-only-guard-after-clear", sample);
            }
        }

        lastOnlyGang = ReadRuntimeByteForLog(kPopulationOnlyCreateRandomGangMembers);
        lastCheatGangLand = ReadRuntimeByteForLog(kCheatsActive + kCheatGangsControlStreets);
        lastZoneStreamingMask = ReadZoneStreamingCheatMaskForLog();
        Sleep(static_cast<DWORD>(g_config.gangOnlyPopulationGuardIntervalMs));
    }

    Log("population guard: completed iterations=%d clearCount=%d",
        g_config.gangOnlyPopulationGuardIterations,
        clearCount);
    return 0;
}

DWORD WINAPI PedStreamingZoneRepairThread(void*)
{
    if (!g_config.enablePedStreamingZoneRepair || g_config.pedStreamingZoneRepairIterations <= 0) {
        return 0;
    }

    Sleep(static_cast<DWORD>(g_config.pedStreamingZoneRepairStartDelayMs));

    bool poolsReady = false;
    uint32_t pedPool = 0;
    uint32_t vehiclePool = 0;
    uint32_t objectPool = 0;
    uint32_t colModelPool = 0;
    for (int waitAttempt = 0; waitAttempt < 1800; ++waitAttempt) {
        if (AreCorePoolsReadyForDeferredReplay(&pedPool, &vehiclePool, &objectPool, &colModelPool)) {
            poolsReady = true;
            Log("ped zone repair: core pools ready after waitAttempt=%d ped=0x%08X vehicle=0x%08X object=0x%08X colModel=0x%08X",
                waitAttempt,
                pedPool,
                vehiclePool,
                objectPool,
                colModelPool);
            break;
        }

        if (waitAttempt == 0 || waitAttempt == 30 || waitAttempt == 120 || (waitAttempt % 300) == 0) {
            Log("ped zone repair: waiting for core pools waitAttempt=%d ped=0x%08X vehicle=0x%08X object=0x%08X colModel=0x%08X",
                waitAttempt,
                pedPool,
                vehiclePool,
                objectPool,
                colModelPool);
        }
        Sleep(1000);
    }

    if (!poolsReady) {
        Log("ped zone repair: core pools still not ready; continuing with guarded runtime checks");
    }

    LogStreamingPedFunctionEntryDiagnostics("ped-zone-repair-start");

    int wouldTriggerCount = 0;
    int triggerLogCount = 0;
    int skippedRetLogs = 0;

    for (int i = 0; i < g_config.pedStreamingZoneRepairIterations; ++i) {
        const int sample = i + 1;
        const uint32_t streamZone = ReadRuntimeU32ForLog(kStreamingCurrentZoneType);
        const uint32_t popZone = ReadRuntimeU32ForLog(kPopCycleCurrentZoneType);
        const uint32_t msPeds = ReadRuntimeU32ForLog(kStreamingNumPedsLoaded);
        const uint32_t zoneInfo = ReadRuntimeU32ForLog(kPopCycleCurrentZoneInfo);
        const uint32_t requested = ReadRuntimeU32ForLog(kStreamingNumModelsRequested);
        const uint32_t priorityReq = ReadRuntimeU32ForLog(kStreamingNumPriorityRequests);
        const int loadingPriority = ReadRuntimeByteForLog(kRendererLoadingPriority);
        const int disableStreaming = ReadRuntimeByteForLog(kStreamingDisableStreaming);
        const int cutsceneProcessing = ReadRuntimeByteForLog(kCCutsceneMgrCutsceneProcessing);
        const int replayMode = ReadRuntimeByteForLog(kCReplayMode);
        const uint32_t zoneCheats = ReadZoneStreamingCheatMaskForLog();
        const uint32_t currentAreaRaw = ReadRuntimeU32ForLog(kCGameCurrentArea);
        const uintptr_t playerPed = ReadFocusedPlayerPed();
        RuntimeVec3 playerPos{};
        uintptr_t playerMatrix = 0;
        bool playerFromMatrix = false;
        const bool playerPosOk = playerPed != 0 && ReadEntityPosition(playerPed, &playerPos, &playerMatrix, &playerFromMatrix);

        uint8_t firstByte = 0;
        const bool entryReadable = SafeReadU8(kCStreamingStreamZoneModels, &firstByte);
        const bool entryRet = entryReadable && firstByte == 0xC3;
        const bool wouldTrigger =
            wouldTriggerCount < g_config.pedStreamingZoneRepairMaxCalls &&
            popZone != 0xFFFFFFFFu &&
            streamZone == 0xFFFFFFFFu &&
            msPeds == 0 &&
            zoneInfo != 0 &&
            zoneCheats == 0 &&
            requested <= static_cast<uint32_t>(g_config.streamingBusyThreshold) &&
            priorityReq == 0 &&
            loadingPriority == 0 &&
            disableStreaming == 0 &&
            cutsceneProcessing == 0 &&
            replayMode != 1 &&
            currentAreaRaw == 0 &&
            playerPosOk &&
            entryReadable &&
            !entryRet;

        const bool shouldLog =
            triggerLogCount < g_config.pedStreamingZoneRepairMaxLogs &&
            (sample == 1 || sample <= 3 || wouldTrigger || (sample % 15) == 0 ||
             (popZone != 0xFFFFFFFFu && streamZone == 0xFFFFFFFFu && msPeds == 0));

        if (shouldLog) {
            ++triggerLogCount;
            Log("ped zone repair: sample=%d/%d wouldTrigger=%d offThreadCall=0 candidates=%d streamZone=%u popZone=%u msPeds=%u zoneInfo=0x%08X zoneCheats=0x%02X requested=%u priorityReq=%u loadingPriority=%d disableStreaming=%d cutscene=%d replayMode=%d currArea=0x%08X player=0x%08X playerPosOk=%d playerPos=%.1f,%.1f,%.1f playerMatrix=0x%08X matrixPos=%d entryReadable=%d entryByte=0x%02X",
                sample,
                g_config.pedStreamingZoneRepairIterations,
                wouldTrigger ? 1 : 0,
                wouldTriggerCount,
                streamZone,
                popZone,
                msPeds,
                zoneInfo,
                zoneCheats,
                requested,
                priorityReq,
                loadingPriority,
                disableStreaming,
                cutsceneProcessing,
                replayMode,
                currentAreaRaw,
                static_cast<unsigned>(playerPed),
                playerPosOk ? 1 : 0,
                playerPosOk ? playerPos.x : -9999.0f,
                playerPosOk ? playerPos.y : -9999.0f,
                playerPosOk ? playerPos.z : -9999.0f,
                static_cast<unsigned>(playerMatrix),
                playerFromMatrix ? 1 : 0,
                entryReadable ? 1 : 0,
                entryReadable ? static_cast<unsigned>(firstByte) : 0xFFu);
        }

        if (entryRet && skippedRetLogs < 4) {
            ++skippedRetLogs;
            Log("ped zone repair: StreamZoneModels entry is RET; not calling original sample=%d", sample);
            LogStreamingPedFunctionEntryDiagnostics("ped-zone-repair-entry-ret");
        }

        if (wouldTrigger) {
            ++wouldTriggerCount;
            if (g_config.enablePopulationPoolDiagnostics) {
                LogPopulationStreamingPedSlots("ped-zone-repair-diagnostic", sample);
            }
        }

        Sleep(static_cast<DWORD>(g_config.pedStreamingZoneRepairIntervalMs));
    }

    Log("ped zone repair: completed iterations=%d offThreadCalls=0 candidates=%d logs=%d",
        g_config.pedStreamingZoneRepairIterations,
        wouldTriggerCount,
        triggerLogCount);
    return 0;
}

bool TokenMatchesPickupModel(uint32_t modelId, const char* token)
{
    if (!token || !*token) {
        return false;
    }

    while (*token == ' ' || *token == '\t') {
        ++token;
    }

    if (std::strcmp(token, "*") == 0 || _stricmp(token, "all") == 0 || _stricmp(token, "highid") == 0) {
        return true;
    }

    if (_stricmp(token, "pickupsave") == 0 || _stricmp(token, "MI_PICKUP_SAVEGAME") == 0 ||
        _stricmp(token, "MODEL_PICKUPSAVE") == 0) {
        return modelId == 1277;
    }

    char* end = nullptr;
    const unsigned long parsed = std::strtoul(token, &end, 0);
    if (end && *end == '\0') {
        return parsed == modelId;
    }

    return false;
}

bool PickupModelGuardAllows(uint32_t modelId)
{
    char list[sizeof(g_config.pickupModelLoadGuardAllowlist)]{};
    strncpy_s(list, sizeof(list), g_config.pickupModelLoadGuardAllowlist, _TRUNCATE);

    char* context = nullptr;
    for (char* token = strtok_s(list, ";, \t", &context);
         token;
         token = strtok_s(nullptr, ";, \t", &context)) {
        if (TokenMatchesPickupModel(modelId, token)) {
            return true;
        }
    }

    return false;
}

bool TryLoadPickupModelNow(uint32_t modelId)
{
#if defined(_M_IX86)
    if (!IsStreamingModelDefined(modelId)) {
        return false;
    }
    if (IsModelRwObjectLoaded(modelId)) {
        return true;
    }

    using RequestModelFn = void(__cdecl*)(int, int);
    using LoadAllRequestedModelsFn = void(__cdecl*)(bool);

    auto requestModel = reinterpret_cast<RequestModelFn>(0x004087E0);
    auto loadAllRequestedModels = reinterpret_cast<LoadAllRequestedModelsFn>(0x0040EA10);

    __try {
        requestModel(static_cast<int>(modelId), g_config.pickupModelLoadGuardFlags);
        if (g_config.pickupModelLoadGuardLoadNow) {
            loadAllRequestedModels(false);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("pickup model guard: streaming request exception model=%u flags=0x%X", modelId, g_config.pickupModelLoadGuardFlags);
        return false;
    }

    return IsModelRwObjectLoaded(modelId);
#else
    (void)modelId;
    return false;
#endif
}

bool ShadowPickupModelToLegacyTables(uint32_t modelId)
{
    bool ok = true;
    uint32_t modelInfo = 0;
    const uintptr_t modelEntry = SafeModelInfoEntryAddress(modelId);
    if (!modelEntry || !SafeReadU32(modelEntry, &modelInfo) || !modelInfo) {
        return false;
    }

    if (modelId < kOriginalCModelInfoCount) {
        ok = CopyMemoryWithProtect(
            kOriginalCModelInfoPtrs + static_cast<uintptr_t>(modelId) * sizeof(uintptr_t),
            reinterpret_cast<uintptr_t>(&modelInfo),
            sizeof(modelInfo)) && ok;
    }

    const uintptr_t streamingEntry = SafeStreamingInfoEntryAddress(modelId);
    if (streamingEntry && modelId < kOriginalStreamingInfoCount) {
        ok = CopyMemoryWithProtect(
            kOriginalStreamingInfo + static_cast<uintptr_t>(modelId) * kStreamingInfoSize,
            streamingEntry,
            kStreamingInfoSize) && ok;
    }

    static LONG logCount = 0;
    const LONG count = InterlockedIncrement(&logCount);
    if (count <= g_config.pickupModelLoadGuardMaxLogs) {
        Log("pickup model guard: legacy shadow model=%u modelInfo=0x%08X streaming=0x%08X ok=%d",
            modelId,
            modelInfo,
            streamingEntry,
            ok ? 1 : 0);
    }

    return ok;
}

extern "C" int __stdcall Bridge_ShouldCreatePickupObject(uintptr_t pickup)
{
    if (!g_config.enablePickupModelLoadGuard || !pickup || !IsReadableCommitted(pickup + 0x18, sizeof(uint16_t))) {
        return 1;
    }

    const int32_t extendedModelId = ReadExtendedIdFrom16BitField(reinterpret_cast<const void*>(pickup + 0x18));
    if (extendedModelId < 0) {
        return 1;
    }

    const uint32_t modelId = static_cast<uint32_t>(extendedModelId);
    const bool protectHighId = g_config.pickupModelLoadGuardProtectHighIds &&
        (modelId >= kOriginalCModelInfoCount || IsFlaDifficultHighIdMode());
    if (!protectHighId && !PickupModelGuardAllows(modelId)) {
        return 1;
    }

    if (IsModelRwObjectLoaded(modelId)) {
        ShadowPickupModelToLegacyTables(modelId);
        return 1;
    }

    const uint8_t beforeState = GetStreamingLoadState(modelId);
    const bool loaded = TryLoadPickupModelNow(modelId);
    const uint8_t afterState = GetStreamingLoadState(modelId);

    static LONG logCount = 0;
    const LONG count = InterlockedIncrement(&logCount);
    if (count <= g_config.pickupModelLoadGuardMaxLogs) {
        Log("pickup model guard: model=%u beforeState=%u afterState=%u loadNow=%d loaded=%d pickup=0x%08X",
            modelId,
            static_cast<unsigned>(beforeState),
            static_cast<unsigned>(afterState),
            g_config.pickupModelLoadGuardLoadNow ? 1 : 0,
            loaded ? 1 : 0,
            pickup);
        if (!loaded) {
            DumpModelContext(modelId);
        }
    }

    if (loaded) {
        ShadowPickupModelToLegacyTables(modelId);
    }

    return loaded ? 1 : 0;
}

extern "C" __declspec(naked) void Bridge_CPickup_GiveUsAPickUpObject_Guard()
{
#if defined(_M_IX86)
    __asm {
        pushad
        push ecx
        call Bridge_ShouldCreatePickupObject
        test eax, eax
        jnz allow_original
        popad
        mov eax, [esp + 4]
        test eax, eax
        jz skip_store
        mov dword ptr [eax], 0
    skip_store:
        ret 8

    allow_original:
        popad
        sub esp, 0Ch
        push esi
        mov esi, [esp + 14h]
        push 004567E8h
        ret
    }
#endif
}

extern "C" bool __stdcall Bridge_HasValidBoundRectColModel(uintptr_t entity)
{
    if (!IsReadableCommitted(entity + 0x22, sizeof(uint16_t))) {
        return false;
    }

    const int32_t extendedModelId = ReadExtendedIdFrom16BitField(reinterpret_cast<const void*>(entity + 0x22));
    if (extendedModelId < 0) {
        return false;
    }

    const uint32_t modelId = static_cast<uint32_t>(extendedModelId);
    uintptr_t modelInfo = 0;
    if (modelId < kOriginalCModelInfoCount) {
        SafeReadU32(kOriginalCModelInfoPtrs + static_cast<uintptr_t>(modelId) * sizeof(uintptr_t), &modelInfo);
    }
    if (!modelInfo) {
        const uintptr_t entry = SafeModelInfoEntryAddress(modelId);
        if (entry) {
            SafeReadU32(entry, &modelInfo);
        }
    }
    if (!modelInfo || !IsReadableCommitted(modelInfo + 0x14, sizeof(uintptr_t))) {
        return false;
    }

    uint32_t colModel = 0;
    if (!SafeReadU32(modelInfo + 0x14, &colModel)) {
        return false;
    }

    return colModel && IsReadableCommitted(colModel, 0x18);
}

extern "C" void __stdcall Bridge_FillFallbackBoundRect(uintptr_t entity, BridgeRect* outRect)
{
    if (!outRect || !IsWritableCommitted(reinterpret_cast<uintptr_t>(outRect), sizeof(*outRect))) {
        return;
    }

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    if (IsReadableCommitted(entity + 0x04, sizeof(float) * 3)) {
        __try {
            x = *reinterpret_cast<const float*>(entity + 0x04);
            y = *reinterpret_cast<const float*>(entity + 0x08);
            z = *reinterpret_cast<const float*>(entity + 0x0C);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            x = 0.0f;
            y = 0.0f;
            z = 0.0f;
        }
    }

    if (!IsReasonableWorldCoord(x) || !IsReasonableWorldCoord(y)) {
        x = 0.0f;
        y = 0.0f;
    }

    int32_t modelId = -1;
    if (IsReadableCommitted(entity + 0x22, sizeof(uint16_t))) {
        modelId = ReadExtendedIdFrom16BitField(reinterpret_cast<const void*>(entity + 0x22));
    }

    constexpr float radius = 1.0f;
    outRect->left = x - radius;
    outRect->right = x + radius;
    outRect->bottom = y - radius;
    outRect->top = y + radius;

    static LONG fallbackCount = 0;
    const LONG count = InterlockedIncrement(&fallbackCount);
    if (count <= 64) {
        Log("bound rect guard: fallback rect model=%u entity=0x%08X pos=(%.3f, %.3f, %.3f) rect=(%.3f, %.3f, %.3f, %.3f)",
            modelId >= 0 ? static_cast<uint32_t>(modelId) : 0xFFFFFFFFu,
            entity,
            x,
            y,
            z,
            outRect->left,
            outRect->bottom,
            outRect->right,
            outRect->top);
        if (modelId < kOriginalCModelInfoCount) {
            DumpModelContext(modelId);
        }
    }
}

extern "C" void __stdcall Bridge_LogInvalidShouldModelBeStreamedColModel(uintptr_t entity, uintptr_t modelInfo, uintptr_t colModel, uintptr_t stack)
{
    int32_t modelId = -1;
    if (IsReadableCommitted(entity + 0x22, sizeof(uint16_t))) {
        modelId = ReadExtendedIdFrom16BitField(reinterpret_cast<const void*>(entity + 0x22));
    }

    static LONG invalidCount = 0;
    const LONG count = InterlockedIncrement(&invalidCount);
    if (count <= 64) {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        if (IsReadableCommitted(entity + 0x04, sizeof(float) * 3)) {
            __try {
                x = *reinterpret_cast<const float*>(entity + 0x04);
                y = *reinterpret_cast<const float*>(entity + 0x08);
                z = *reinterpret_cast<const float*>(entity + 0x0C);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                x = 0.0f;
                y = 0.0f;
                z = 0.0f;
            }
        }

        Log("should stream guard: skipped model=%u entity=0x%08X modelInfo=0x%08X colModel=0x%08X stack=0x%08X pos=(%.3f, %.3f, %.3f)",
            modelId >= 0 ? static_cast<uint32_t>(modelId) : 0xFFFFFFFFu,
            entity,
            modelInfo,
            colModel,
            stack,
            x,
            y,
            z);
        if (count <= 8 && modelId >= 0 && modelId < static_cast<int32_t>(kOriginalCModelInfoCount)) {
            DumpModelContext(static_cast<uint32_t>(modelId));
        }
    }
}

uintptr_t MatrixListBase()
{
    return g_config.matrixGuardListBase ? static_cast<uintptr_t>(g_config.matrixGuardListBase) : kDefaultMatrixLinkList;
}

uintptr_t MatrixListHead()
{
    return MatrixListBase() + kMatrixListHeadOffset;
}

uintptr_t MatrixListTail()
{
    return MatrixListBase() + kMatrixListTailOffset;
}

uintptr_t MatrixListAllocatedHead()
{
    return MatrixListBase() + kMatrixListAllocatedHeadOffset;
}

uintptr_t MatrixListAllocatedTail()
{
    return MatrixListBase() + kMatrixListAllocatedTailOffset;
}

uintptr_t MatrixListFreeHead()
{
    return MatrixListBase() + kMatrixListFreeHeadOffset;
}

uintptr_t MatrixListFreeTail()
{
    return MatrixListBase() + kMatrixListFreeTailOffset;
}

uintptr_t MatrixListDynamicTailPrevAddress()
{
    return MatrixListTail() + kMatrixLinkPrevOffset;
}

uintptr_t MatrixListStaticTailPrevAddress()
{
    return MatrixListAllocatedTail() + kMatrixLinkPrevOffset;
}

bool IsMatrixListSentinel(uintptr_t link)
{
    return link == MatrixListHead() ||
        link == MatrixListTail() ||
        link == MatrixListAllocatedHead() ||
        link == MatrixListAllocatedTail() ||
        link == MatrixListFreeHead() ||
        link == MatrixListFreeTail();
}

bool MatrixGuardCodeLooksCompatible(uintptr_t eip)
{
    if (!g_config.matrixGuardSignatureCheck) {
        return true;
    }

    bool matched = false;
    if (eip == g_config.matrixGuardRemoveMatrixEntry) {
        const uint8_t pattern[] = { 0x8B, 0x41, 0x14 };
        matched = BytePatternMatches(eip, pattern, sizeof(pattern));
    } else if (eip == g_config.matrixGuardRemoveMatrixNullLoad) {
        const uint8_t pattern[] = { 0x8B, 0x48, 0x04 };
        matched = BytePatternMatches(eip, pattern, sizeof(pattern));
    } else if (eip == g_config.matrixGuardAllocateStaticOwnerWrite) {
        const uint8_t pattern[] = { 0x89, 0x70, 0x48, 0x89, 0x46, 0x14 };
        matched = BytePatternMatches(eip, pattern, sizeof(pattern));
    } else {
        return false;
    }

    if (!matched) {
        static LONG mismatchLogs = 0;
        const LONG count = InterlockedIncrement(&mismatchLogs);
        if (count <= 8) {
            Log("matrix guard signature mismatch: eip=0x%08X removeEntry=0x%08X removeNull=0x%08X allocWrite=0x%08X signatureCheck=%d",
                eip,
                g_config.matrixGuardRemoveMatrixEntry,
                g_config.matrixGuardRemoveMatrixNullLoad,
                g_config.matrixGuardAllocateStaticOwnerWrite,
                g_config.matrixGuardSignatureCheck ? 1 : 0);
            LogBytes("matrix-guard-eip", eip, 16);
        }
    }

    return matched;
}

bool MoveMatrixLinkToFreeList(uintptr_t link, uintptr_t expectedOwner, uintptr_t* recoveredLink, uintptr_t* recoveredOwner)
{
    if (recoveredLink) {
        *recoveredLink = 0;
    }
    if (recoveredOwner) {
        *recoveredOwner = 0;
    }

    if (!link || IsMatrixListSentinel(link) || !IsWritableCommitted(link, 0x54)) {
        return false;
    }

    uintptr_t owner = 0;
    uintptr_t prev = 0;
    uintptr_t next = 0;
    uintptr_t freeNext = 0;
    if (!SafeReadU32(link + kMatrixLinkOwnerOffset, &owner) ||
        !SafeReadU32(link + kMatrixLinkPrevOffset, &prev) ||
        !SafeReadU32(link + kMatrixLinkNextOffset, &next) ||
        !SafeReadU32(MatrixListFreeHead() + kMatrixLinkNextOffset, &freeNext)) {
        return false;
    }

    if (expectedOwner && owner && owner != expectedOwner) {
        uintptr_t ownerMatrix = 0;
        SafeReadU32(expectedOwner + 0x14, &ownerMatrix);
        if (ownerMatrix != 0 && ownerMatrix != link) {
            return false;
        }
    }

    if (!prev || !next || !freeNext ||
        IsMatrixListSentinel(link) ||
        !IsWritableCommitted(prev + kMatrixLinkNextOffset, sizeof(uintptr_t)) ||
        !IsWritableCommitted(next + kMatrixLinkPrevOffset, sizeof(uintptr_t)) ||
        !IsWritableCommitted(MatrixListFreeHead() + kMatrixLinkNextOffset, sizeof(uintptr_t)) ||
        !IsWritableCommitted(freeNext + kMatrixLinkPrevOffset, sizeof(uintptr_t))) {
        return false;
    }

    __try {
        if (owner && IsWritableCommitted(owner + 0x14, sizeof(uintptr_t))) {
            uintptr_t ownerMatrix = 0;
            if (SafeReadU32(owner + 0x14, &ownerMatrix) && ownerMatrix == link) {
                *reinterpret_cast<uint32_t*>(owner + 0x14) = 0;
            }
        }

        *reinterpret_cast<uint32_t*>(next + kMatrixLinkPrevOffset) = static_cast<uint32_t>(prev);
        *reinterpret_cast<uint32_t*>(prev + kMatrixLinkNextOffset) = static_cast<uint32_t>(next);

        *reinterpret_cast<uint32_t*>(link + kMatrixLinkOwnerOffset) = 0;
        *reinterpret_cast<uint32_t*>(link + kMatrixLinkPrevOffset) = static_cast<uint32_t>(MatrixListFreeHead());
        *reinterpret_cast<uint32_t*>(link + kMatrixLinkNextOffset) = static_cast<uint32_t>(freeNext);
        *reinterpret_cast<uint32_t*>(freeNext + kMatrixLinkPrevOffset) = static_cast<uint32_t>(link);
        *reinterpret_cast<uint32_t*>(MatrixListFreeHead() + kMatrixLinkNextOffset) = static_cast<uint32_t>(link);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    if (recoveredLink) {
        *recoveredLink = link;
    }
    if (recoveredOwner) {
        *recoveredOwner = owner;
    }
    return true;
}

bool RecoverOldestMatrixLinkToFreeList(uintptr_t expectedOwner, uintptr_t* recoveredLink, uintptr_t* recoveredOwner, const char** sourceList)
{
    if (sourceList) {
        *sourceList = "none";
    }

    uintptr_t oldestDynamic = 0;
    if (SafeReadU32(MatrixListDynamicTailPrevAddress(), &oldestDynamic) &&
        MoveMatrixLinkToFreeList(oldestDynamic, expectedOwner, recoveredLink, recoveredOwner)) {
        if (sourceList) {
            *sourceList = "dynamic";
        }
        return true;
    }

    uintptr_t oldestStatic = 0;
    if (g_config.matrixGuardRecoverStaticList &&
        SafeReadU32(MatrixListStaticTailPrevAddress(), &oldestStatic) &&
        MoveMatrixLinkToFreeList(oldestStatic, expectedOwner, recoveredLink, recoveredOwner)) {
        if (sourceList) {
            *sourceList = "static";
        }
        return true;
    }

    return false;
}

bool AllocateStaticMatrixLinkFromFreeList(uintptr_t owner, uintptr_t* allocatedLink)
{
    if (allocatedLink) {
        *allocatedLink = 0;
    }

    if (!owner || !IsWritableCommitted(owner + 0x14, sizeof(uintptr_t))) {
        return false;
    }

    uintptr_t freeLink = 0;
    uintptr_t freeNext = 0;
    uintptr_t list2Next = 0;
    if (!SafeReadU32(MatrixListFreeHead() + kMatrixLinkNextOffset, &freeLink) ||
        !freeLink ||
        freeLink == MatrixListFreeTail() ||
        IsMatrixListSentinel(freeLink) ||
        !IsWritableCommitted(freeLink, 0x54) ||
        !SafeReadU32(freeLink + kMatrixLinkNextOffset, &freeNext) ||
        !SafeReadU32(MatrixListAllocatedHead() + kMatrixLinkNextOffset, &list2Next)) {
        return false;
    }

    if (!freeNext || !list2Next ||
        !IsWritableCommitted(freeNext + kMatrixLinkPrevOffset, sizeof(uintptr_t)) ||
        !IsWritableCommitted(MatrixListFreeHead() + kMatrixLinkNextOffset, sizeof(uintptr_t)) ||
        !IsWritableCommitted(MatrixListAllocatedHead() + kMatrixLinkNextOffset, sizeof(uintptr_t)) ||
        !IsWritableCommitted(list2Next + kMatrixLinkPrevOffset, sizeof(uintptr_t))) {
        return false;
    }

    __try {
        *reinterpret_cast<uint32_t*>(freeNext + kMatrixLinkPrevOffset) = static_cast<uint32_t>(MatrixListFreeHead());
        *reinterpret_cast<uint32_t*>(MatrixListFreeHead() + kMatrixLinkNextOffset) = static_cast<uint32_t>(freeNext);

        *reinterpret_cast<uint32_t*>(freeLink + kMatrixLinkPrevOffset) = static_cast<uint32_t>(MatrixListAllocatedHead());
        *reinterpret_cast<uint32_t*>(freeLink + kMatrixLinkNextOffset) = static_cast<uint32_t>(list2Next);
        *reinterpret_cast<uint32_t*>(list2Next + kMatrixLinkPrevOffset) = static_cast<uint32_t>(freeLink);
        *reinterpret_cast<uint32_t*>(MatrixListAllocatedHead() + kMatrixLinkNextOffset) = static_cast<uint32_t>(freeLink);

        *reinterpret_cast<uint32_t*>(freeLink + kMatrixLinkOwnerOffset) = static_cast<uint32_t>(owner);
        *reinterpret_cast<uint32_t*>(owner + 0x14) = static_cast<uint32_t>(freeLink);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    if (allocatedLink) {
        *allocatedLink = freeLink;
    }
    return true;
}

uintptr_t ResolvePlaceableRemoveMatrixReturnAddress(uintptr_t esp)
{
    if (!IsReadableCommitted(esp, 0x24)) {
        return 0;
    }

    uintptr_t fallback = 0;
    __try {
        for (uintptr_t offset = 0; offset <= 0x20; offset += sizeof(uintptr_t)) {
            const uintptr_t value = *reinterpret_cast<const uintptr_t*>(esp + offset);
            if (!value) {
                continue;
            }

            if (value >= g_config.matrixGuardAllocateStaticMin && value <= g_config.matrixGuardAllocateStaticMax) {
                return value;
            }

            if (!fallback && value >= 0x00400000 && value < 0x00880000 && IsExecutableCommitted(value)) {
                fallback = value;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }

    return fallback;
}

extern "C" void __stdcall Bridge_LogInvalidBoundCentreOut(uintptr_t entity, uintptr_t outVec, uintptr_t returnAddress, uintptr_t stack)
{
    static LONG logCount = 0;
    const LONG count = InterlockedIncrement(&logCount);
    if (count > 32) {
        return;
    }

    int32_t modelId = -1;
    if (IsReadableCommitted(entity + 0x22, sizeof(uint16_t))) {
        modelId = ReadExtendedIdFrom16BitField(reinterpret_cast<const void*>(entity + 0x22));
    }

    char moduleName[MAX_PATH]{};
    const uintptr_t moduleBase = ModuleBaseFromAddress(returnAddress, moduleName, sizeof(moduleName));
    Log("bound centre guard: skipped null/bad out entity=0x%08X out=0x%08X model=%u return=0x%08X %s+0x%X",
        entity,
        outVec,
        modelId >= 0 ? static_cast<uint32_t>(modelId) : 0xFFFFFFFFu,
        returnAddress,
        moduleName,
        moduleBase ? returnAddress - moduleBase : 0);
    LogMemoryRegion("bound-centre-entity", entity);
    LogMemoryRegion("bound-centre-out", outVec);
    if (modelId < kOriginalCModelInfoCount) {
        DumpModelContext(modelId);
    }
    LogStackModules(stack);
}

uint32_t GetUppercaseKeyViaGame(const char* text)
{
    using GetUppercaseKeyFn = uint32_t(__cdecl*)(const char*);
    auto getUppercaseKey = reinterpret_cast<GetUppercaseKeyFn>(0x0053CF30);
    return getUppercaseKey(text);
}

void ReverseString(const char* input, char* output, size_t outputSize)
{
    if (!input || !output || outputSize == 0) {
        return;
    }

    const size_t length = std::strlen(input);
    const size_t copyLength = length < outputSize - 1 ? length : outputSize - 1;
    for (size_t i = 0; i < copyLength; ++i) {
        output[i] = input[copyLength - 1 - i];
    }
    output[copyLength] = 0;
}

void StripLineCommentAndCommas(char* line)
{
    if (!line) {
        return;
    }

    for (char* c = line; *c; ++c) {
        if (*c == '#') {
            *c = 0;
            break;
        }
        if (*c == ',') {
            *c = ' ';
        }
        if (*c == '\r' || *c == '\n') {
            *c = 0;
            break;
        }
    }
}

void LoadBridgeCheatStrings()
{
    constexpr const char* cheatPath = "data\\cheatStrings.dat";
    FILE* file = nullptr;
    if (fopen_s(&file, cheatPath, "r") != 0 || !file) {
        Log("bridge cheat loader: %s not found; keeping original cheat hashes", cheatPath);
        return;
    }

    if (!IsReadableCommitted(kCheatHashKeys, kCheatHashKeyCount * sizeof(uint32_t))) {
        Log("bridge cheat loader: cheat hash table unreadable at 0x%08X", kCheatHashKeys);
        std::fclose(file);
        return;
    }

    uint32_t newHashes[kCheatHashKeyCount]{};
    __try {
        std::memcpy(newHashes, reinterpret_cast<const void*>(kCheatHashKeys), sizeof(newHashes));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("bridge cheat loader: failed to copy original cheat hash table");
        std::fclose(file);
        return;
    }

    char line[1024]{};
    int lineNumber = 0;
    int applied = 0;
    int skipped = 0;

    while (std::fgets(line, sizeof(line), file)) {
        ++lineNumber;
        StripLineCommentAndCommas(line);

        char* context = nullptr;
        char* idToken = strtok_s(line, " \t", &context);
        if (!idToken) {
            continue;
        }

        char* end = nullptr;
        const long id = std::strtol(idToken, &end, 10);
        if (!end || *end != 0 || id < 0 || id >= static_cast<long>(kCheatHashKeyCount)) {
            Log("bridge cheat loader: skip line=%d invalid ID token='%s'", lineNumber, idToken);
            ++skipped;
            continue;
        }

        char* cheatText = strtok_s(nullptr, " \t", &context);
        if (!cheatText) {
            cheatText = const_cast<char*>("");
        }

        const size_t length = std::strlen(cheatText);
        if (length != 0 && (length < 6 || length > 30)) {
            Log("bridge cheat loader: skip line=%d id=%ld invalid length=%u text='%s'",
                lineNumber, id, static_cast<unsigned>(length), cheatText);
            ++skipped;
            continue;
        }

        uint32_t hash = 0;
        if (length != 0) {
            char reversed[31]{};
            ReverseString(cheatText, reversed, sizeof(reversed));
            hash = GetUppercaseKeyViaGame(reversed);
        }

        newHashes[id] = hash;
        ++applied;
        Log("bridge cheat loader: id=%ld hash=0x%08X text='%s'", id, hash, cheatText);
    }

    std::fclose(file);

    if (WriteBytesWithProtect(kCheatHashKeys, reinterpret_cast<const uint8_t*>(newHashes), sizeof(newHashes))) {
        Log("bridge cheat loader: loaded applied=%d skipped=%d table=0x%08X source=%s",
            applied, skipped, kCheatHashKeys, cheatPath);
    }
}

extern "C" bool __cdecl Bridge_CStreaming_IsVeryBusy()
{
    PumpDeferredPoolAllocatesOnGameThread(4);

    const uintptr_t chainedTarget = g_chainedStreamingBusyTarget;
    if (chainedTarget) {
        __try {
            return reinterpret_cast<bool(__cdecl*)()>(chainedTarget)();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            g_chainedStreamingBusyTarget = 0;
            Log("streaming busy patch: chained target faulted target=0x%08X code=0x%08X; using bridge fallback",
                chainedTarget,
                GetExceptionCode());
        }
    }

    uint8_t loadingPriority = 0;
    uint32_t requested = 0;

    SafeReadU8(kRendererLoadingPriority, &loadingPriority);
    SafeReadU32(kStreamingNumModelsRequested, &requested);

    const uint32_t configuredThreshold = static_cast<uint32_t>(g_config.streamingBusyThreshold < 5 ? 5 : g_config.streamingBusyThreshold);
    const uint32_t threshold = g_config.enableStreamingBusyThresholdPatch ? configuredThreshold : 5u;
    return loadingPriority != 0 || requested > threshold;
}

void InstallStreamingBusyThresholdPatch()
{
#if defined(_M_IX86)
    if (!g_config.enableStreamingBusyThresholdPatch && !g_config.enableDeferredPoolAllocateReplay) {
        return;
    }

    const uint32_t configuredThreshold = static_cast<uint32_t>(
        g_config.streamingBusyThreshold < 5 ? 5 : g_config.streamingBusyThreshold);

    uint8_t current[8]{};
    if (!IsReadableCommitted(kCStreamingIsVeryBusy, sizeof(current))) {
        Log("streaming busy patch: CStreaming::IsVeryBusy unreadable address=0x%08X", kCStreamingIsVeryBusy);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(kCStreamingIsVeryBusy), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("streaming busy patch: byte read fault address=0x%08X", kCStreamingIsVeryBusy);
        return;
    }

    const uintptr_t target = reinterpret_cast<uintptr_t>(Bridge_CStreaming_IsVeryBusy);
    if (current[0] == 0xE9) {
        int32_t existingRel = 0;
        std::memcpy(&existingRel, current + 1, sizeof(existingRel));
        const uintptr_t existingTarget = kCStreamingIsVeryBusy + 5 + static_cast<intptr_t>(existingRel);
        if (existingTarget == target) {
            Log("streaming busy patch: already installed threshold=%u replayPump=%d address=0x%08X bridge=0x%08X",
                g_config.enableStreamingBusyThresholdPatch ? configuredThreshold : 5u,
                g_config.enableDeferredPoolAllocateReplay ? 1 : 0,
                kCStreamingIsVeryBusy,
                target);
        } else if (existingTarget == kHoodlumCStreamingIsVeryBusy && IsExecutableCommitted(existingTarget)) {
            g_chainedStreamingBusyTarget = existingTarget;
            if (WriteRel32Jump(kCStreamingIsVeryBusy, target)) {
                Log("streaming busy patch: chained FLA Hoodlum target=0x%08X thresholdOwner=FLA replayPump=%d address=0x%08X bridge=0x%08X",
                    existingTarget,
                    g_config.enableDeferredPoolAllocateReplay ? 1 : 0,
                    kCStreamingIsVeryBusy,
                    target);
            } else {
                g_chainedStreamingBusyTarget = 0;
                Log("streaming busy patch: failed to chain FLA Hoodlum target=0x%08X address=0x%08X",
                    existingTarget,
                    kCStreamingIsVeryBusy);
            }
        } else {
            Log("streaming busy patch: existing hook detected at 0x%08X target=0x%08X previous=%02X %02X %02X %02X %02X %02X %02X %02X; skipping to preserve hook chain",
                kCStreamingIsVeryBusy,
                existingTarget,
                current[0], current[1], current[2], current[3],
                current[4], current[5], current[6], current[7]);
        }
        return;
    }

    if (WriteRel32Jump(kCStreamingIsVeryBusy, target)) {
        Log("streaming busy patch: installed threshold=%u replayPump=%d address=0x%08X bridge=0x%08X previous=%02X %02X %02X %02X %02X %02X %02X %02X",
            g_config.enableStreamingBusyThresholdPatch ? configuredThreshold : 5u,
            g_config.enableDeferredPoolAllocateReplay ? 1 : 0,
            kCStreamingIsVeryBusy,
            target,
            current[0], current[1], current[2], current[3],
            current[4], current[5], current[6], current[7]);
    }
#else
    Log("streaming busy patch: unsupported architecture");
#endif
}

void InstallPopulationUpdateBudgetPatch()
{
#if defined(_M_IX86)
    if (!g_config.enablePopulationUpdateBudgetPatch) {
        return;
    }

    uint8_t current[3]{};
    if (!IsReadableCommitted(kCGamePopulationUpdateBudgetCmp, sizeof(current))) {
        Log("population update budget patch: comparison unreadable address=0x%08X", kCGamePopulationUpdateBudgetCmp);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(kCGamePopulationUpdateBudgetCmp), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("population update budget patch: byte read fault address=0x%08X", kCGamePopulationUpdateBudgetCmp);
        return;
    }

    if (current[0] != 0x83 || current[1] != 0xFE) {
        Log("population update budget patch: unexpected bytes at 0x%08X old=%02X %02X %02X",
            kCGamePopulationUpdateBudgetCmp,
            current[0],
            current[1],
            current[2]);
        return;
    }

    int budget = g_config.populationUpdateBudgetMs;
    if (budget < 1) {
        budget = 1;
    } else if (budget > 127) {
        budget = 127;
    }

    const uint8_t budgetByte = static_cast<uint8_t>(budget);
    if (WriteBytesWithProtect(kCGamePopulationUpdateBudgetImmediate, &budgetByte, sizeof(budgetByte))) {
        Log("population update budget patch: installed budgetMs=%d address=0x%08X previous=%u",
            budget,
            kCGamePopulationUpdateBudgetImmediate,
            static_cast<unsigned>(current[2]));
    }
#else
    Log("population update budget patch: unsupported architecture");
#endif
}

void InstallShouldModelBeStreamedGuard()
{
#if defined(_M_IX86)
    constexpr uintptr_t patchAddress = 0x00554F62;
    constexpr uintptr_t continueAddress = 0x00554F69;
    constexpr uintptr_t returnFalseAddress = 0x00554F76;
    static const uint8_t expectedBytes[] = {
        0xD9, 0x40, 0x24,       // fld dword ptr [eax+24h]
        0xD8, 0x44, 0x24, 0x20  // fadd dword ptr [esp+20h]
    };

    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_ShouldModelBeStreamed_ColModelGuard);
    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    if (currentTarget == guardTarget) {
        Log("should stream guard: already installed at 0x%08X", patchAddress);
        return;
    }

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("should stream guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("should stream guard: read exception at 0x%08X", patchAddress);
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("should stream guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X %02X %02X currentTarget=0x%08X",
            patchAddress,
            current[0], current[1], current[2], current[3],
            current[4], current[5], current[6],
            currentTarget);
        return;
    }

    g_shouldModelBeStreamedContinue = continueAddress;
    g_shouldModelBeStreamedReturnFalse = returnFalseAddress;
    uint8_t patch[sizeof(expectedBytes)]{};
    patch[0] = 0xE9;
    const int32_t rel = static_cast<int32_t>(guardTarget - (patchAddress + 5));
    std::memcpy(patch + 1, &rel, sizeof(rel));
    for (size_t i = 5; i < sizeof(patch); ++i) {
        patch[i] = 0x90;
    }

    if (WriteBytesWithProtect(patchAddress, patch, sizeof(patch))) {
        Log("should stream guard: installed at 0x%08X target=0x%08X continue=0x%08X false=0x%08X",
            patchAddress,
            guardTarget,
            g_shouldModelBeStreamedContinue,
            g_shouldModelBeStreamedReturnFalse);
    }
#else
    Log("should stream guard: unsupported architecture");
#endif
}

float AbsFloat(float value)
{
    return value < 0.0f ? -value : value;
}

bool IsReasonableWorldCoord(float value)
{
    return value == value && value > -20000.0f && value < 20000.0f;
}

bool IsNearWorldOrigin2D(float x, float y)
{
    return AbsFloat(x) < 5.0f && AbsFloat(y) < 5.0f;
}

bool IsNearWorldOrigin2DLoose(float x, float y)
{
    return AbsFloat(x) < 100.0f && AbsFloat(y) < 100.0f;
}

bool IsValidScriptCoord3D(float x, float y, float z)
{
    return IsReasonableWorldCoord(x) && IsReasonableWorldCoord(y) &&
        IsReasonableWorldCoord(z) && !IsNearWorldOrigin2D(x, y);
}

void RequestPathStreamingForCoord(float x, float y, float z)
{
    if (!IsValidScriptCoord3D(x, y, z)) {
        return;
    }

    using SetPathsNeededAtPositionFn = void(__thiscall*)(void*, const float*);
    auto setPathsNeededAtPosition = reinterpret_cast<SetPathsNeededAtPositionFn>(kCPathFindSetPathsNeededAtPosition);
    float pos[3]{ x, y, z };

    __try {
        setPathsNeededAtPosition(reinterpret_cast<void*>(kThePaths), pos);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        const long count = InterlockedIncrement(&g_closestCarNode03D3FallbackLogs);
        if (count <= 16) {
            Log("03D3 fallback: SetPathsNeededAtPosition exception target=(%.2f, %.2f, %.2f)", x, y, z);
        }
    }
}

float SafeFindGroundZForCoord(float x, float y, float fallback)
{
    using FindGroundZForCoordFn = float(__cdecl*)(float, float);
    auto findGroundZForCoord = reinterpret_cast<FindGroundZForCoordFn>(kCWorldFindGroundZForCoord);

    __try {
        const float z = findGroundZForCoord(x, y);
        if (IsReasonableWorldCoord(z) && AbsFloat(z) > 0.01f) {
            return z + 1.0f;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }

    return fallback;
}

bool IsScriptNamed(const RunningScriptLite* script, const char* expected)
{
    if (!script || !expected) {
        return false;
    }

    char name[9]{};
    __try {
        std::memcpy(name, script->name, 8);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    return _strnicmp(name, expected, 8) == 0;
}

uint32_t ReadPatchedRadarTraceLimit()
{
    uint32_t limit = 0;
    constexpr uintptr_t patchedCountAddress = 0x0058384C + 2;
    if (IsReadableCommitted(patchedCountAddress, sizeof(limit))) {
        __try {
            std::memcpy(&limit, reinterpret_cast<const void*>(patchedCountAddress), sizeof(limit));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            limit = 0;
        }
    }

    if (limit >= 1 && limit <= 65535) {
        return limit;
    }
    return 0;
}

uintptr_t ReadPatchedRadarTraceBase()
{
    uintptr_t indexField = 0;
    constexpr uintptr_t patchedIndexPointer = 0x00582889 + 3;
    if (IsReadableCommitted(patchedIndexPointer, sizeof(indexField))) {
        __try {
            std::memcpy(&indexField, reinterpret_cast<const void*>(patchedIndexPointer), sizeof(indexField));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            indexField = 0;
        }
    }

    if (indexField >= kRadarTraceCounterOffset) {
        const uintptr_t base = indexField - kRadarTraceCounterOffset;
        if (IsReadableCommitted(base, kRadarTraceSize)) {
            return base;
        }
    }
    return kOriginalRadarTrace;
}

void RefreshRadarTraceRuntimeState()
{
    g_radarTraceBase = ReadPatchedRadarTraceBase();

    const uint32_t iniLimit = static_cast<uint32_t>(ReadFlaInt("Radar traces", 175, 1, 65535));
    const uint32_t patchedLimit = ReadPatchedRadarTraceLimit();
    g_radarTraceLimit = patchedLimit ? patchedLimit : iniLimit;
    if (iniLimit > g_radarTraceLimit) {
        g_radarTraceLimit = iniLimit;
    }
}

bool IsCleoGameEntitiesCaller(uintptr_t returnAddress, char* moduleName, size_t moduleNameSize)
{
    if (moduleName && moduleNameSize) {
        moduleName[0] = '\0';
    }

    char name[MAX_PATH]{};
    if (!ModuleBaseFromAddress(returnAddress, name, sizeof(name))) {
        if (moduleName && moduleNameSize) {
            strncpy_s(moduleName, moduleNameSize, name, _TRUNCATE);
        }
        return false;
    }

    if (moduleName && moduleNameSize) {
        strncpy_s(moduleName, moduleNameSize, name, _TRUNCATE);
    }

    return ContainsCaseInsensitive(name, "GameEntities") &&
        ContainsCaseInsensitive(name, ".cleo");
}

bool CallerAlreadyUsesRelocatedRadarTrace(uintptr_t returnAddress, const char* moduleName)
{
    if (!g_radarTraceBase || g_radarTraceBase == kOriginalRadarTrace) {
        return false;
    }

    static uintptr_t cachedBase = 0;
    static uintptr_t cachedRadarBase = 0;
    static int cachedResult = -1;

    uintptr_t moduleBase = 0;
    uintptr_t moduleEnd = 0;
    char resolvedName[MAX_PATH]{};
    if (!FindModuleRangeFromAddress(returnAddress, resolvedName, sizeof(resolvedName), &moduleBase, &moduleEnd)) {
        return false;
    }

    if (cachedBase == moduleBase && cachedRadarBase == g_radarTraceBase && cachedResult != -1) {
        return cachedResult != 0;
    }

    const bool hasRelocated = ModuleMemoryContainsU32(moduleBase, moduleEnd, static_cast<uint32_t>(g_radarTraceBase));
    cachedBase = moduleBase;
    cachedRadarBase = g_radarTraceBase;
    cachedResult = hasRelocated ? 1 : 0;

    Log("target blip bridge: caller radar trace mode module=%s base=0x%08X relocatedTrace=0x%08X alreadyRelocated=%d",
        moduleName && *moduleName ? moduleName : resolvedName,
        moduleBase,
        g_radarTraceBase,
        hasRelocated ? 1 : 0);

    return hasRelocated;
}

bool ReadRadarTraceFields(uintptr_t trace, float* outX, float* outY, float* outZ, uint16_t* outCounter, uint8_t* outSprite, uint8_t* outFlags)
{
    if (!IsReadableCommitted(trace, kRadarTraceSize)) {
        return false;
    }

    __try {
        if (outX) {
            std::memcpy(outX, reinterpret_cast<const void*>(trace + kRadarTracePositionOffset + 0x00), sizeof(*outX));
        }
        if (outY) {
            std::memcpy(outY, reinterpret_cast<const void*>(trace + kRadarTracePositionOffset + 0x04), sizeof(*outY));
        }
        if (outZ) {
            std::memcpy(outZ, reinterpret_cast<const void*>(trace + kRadarTracePositionOffset + 0x08), sizeof(*outZ));
        }
        if (outCounter) {
            std::memcpy(outCounter, reinterpret_cast<const void*>(trace + kRadarTraceCounterOffset), sizeof(*outCounter));
        }
        if (outSprite) {
            std::memcpy(outSprite, reinterpret_cast<const void*>(trace + kRadarTraceSpriteOffset), sizeof(*outSprite));
        }
        if (outFlags) {
            std::memcpy(outFlags, reinterpret_cast<const void*>(trace + kRadarTraceFlagOffset), sizeof(*outFlags));
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void RememberTargetBlipCoords(float x, float y, float z)
{
    if (!IsSaneRadarCoord(x, y, z)) {
        return;
    }

    g_lastTargetBlipX = x;
    g_lastTargetBlipY = y;
    g_lastTargetBlipZ = z;
    InterlockedExchange(&g_hasLastTargetBlipCoords, 1);
}

bool IsSaneRadarCoord(float x, float y, float z)
{
    if (x != x || y != y || z != z) {
        return false;
    }
    if (x > -1.0f && x < 1.0f && y > -1.0f && y < 1.0f) {
        return false;
    }
    return x > -60000.0f && x < 60000.0f &&
        y > -60000.0f && y < 60000.0f &&
        z > -10000.0f && z < 10000.0f;
}

bool FindRelocatedWaypointTrace(uint16_t wantedCounter, uint32_t* outIndex, uintptr_t* outTrace)
{
    if (!outIndex || !outTrace || !g_radarTraceBase || !g_radarTraceLimit) {
        return false;
    }

    struct Candidate {
        uint32_t index = 0;
        uintptr_t trace = 0;
        uint16_t counter = 0;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    Candidate fallback{};
    bool hasFallback = false;

    for (uint32_t i = 0; i < g_radarTraceLimit; ++i) {
        const uintptr_t trace = g_radarTraceBase + static_cast<uintptr_t>(i) * kRadarTraceSize;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        uint16_t actualCounter = 0;
        uint8_t sprite = 0;
        uint8_t flags = 0;
        if (!ReadRadarTraceFields(trace, &x, &y, &z, &actualCounter, &sprite, &flags)) {
            continue;
        }
        if (!(flags & kRadarTraceTrackingFlag) || sprite != kRadarSpriteWaypoint || !IsSaneRadarCoord(x, y, z)) {
            continue;
        }

        if (actualCounter == wantedCounter) {
            *outIndex = i;
            *outTrace = trace;
            static LONG exactLogs = 0;
            const LONG count = InterlockedIncrement(&exactLogs);
            if (count <= 32) {
                Log("target blip bridge: found waypoint by counter index=%u counter=%u coords=(%.2f, %.2f, %.2f)",
                    i, actualCounter, x, y, z);
            }
            return true;
        }

        if (!hasFallback || static_cast<uint16_t>(actualCounter - fallback.counter) < 0x8000u) {
            fallback.index = i;
            fallback.trace = trace;
            fallback.counter = actualCounter;
            fallback.x = x;
            fallback.y = y;
            fallback.z = z;
            hasFallback = true;
        }
    }

    if (!hasFallback) {
        for (uint32_t i = 0; i < g_radarTraceLimit; ++i) {
            const uintptr_t trace = g_radarTraceBase + static_cast<uintptr_t>(i) * kRadarTraceSize;
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            uint16_t actualCounter = 0;
            uint8_t sprite = 0;
            uint8_t flags = 0;
            uint32_t entityHandle = 0;
            if (!ReadRadarTraceFields(trace, &x, &y, &z, &actualCounter, &sprite, &flags) ||
                !ReadU32(trace + kRadarTraceEntityHandleOffset, &entityHandle)) {
                continue;
            }
            if (!(flags & kRadarTraceTrackingFlag) || entityHandle != 0 || !IsSaneRadarCoord(x, y, z)) {
                continue;
            }

            if (actualCounter == wantedCounter) {
                *outIndex = i;
                *outTrace = trace;
                static LONG relaxedExactLogs = 0;
                const LONG count = InterlockedIncrement(&relaxedExactLogs);
                if (count <= 32) {
                    Log("target blip bridge: found coord blip by counter index=%u counter=%u sprite=%u coords=(%.2f, %.2f, %.2f)",
                        i, actualCounter, sprite, x, y, z);
                }
                return true;
            }

            if (!hasFallback || static_cast<uint16_t>(actualCounter - fallback.counter) < 0x8000u) {
                fallback.index = i;
                fallback.trace = trace;
                fallback.counter = actualCounter;
                fallback.x = x;
                fallback.y = y;
                fallback.z = z;
                hasFallback = true;
            }
        }

        if (!hasFallback) {
            return false;
        }
    }

    *outIndex = fallback.index;
    *outTrace = fallback.trace;
    static LONG fallbackLogs = 0;
    const LONG count = InterlockedIncrement(&fallbackLogs);
    if (count <= 32) {
        Log("target blip bridge: found waypoint by scan index=%u counter=%u wantedCounter=%u coords=(%.2f, %.2f, %.2f)",
            fallback.index, fallback.counter, wantedCounter, fallback.x, fallback.y, fallback.z);
    }
    return true;
}

bool ShadowRadarTraceForLegacyCleo(uint32_t blip, uint32_t traceIndex, uintptr_t trace, uintptr_t returnAddress, int* outIndex)
{
    if (!g_config.enableCleoTargetBlipCoordsBridge || !outIndex) {
        return false;
    }

    uint32_t targetBlip = 0;
    if (!ReadU32(kFrontEndTargetBlipIndex, &targetBlip) || targetBlip != blip) {
        return false;
    }

    char moduleName[MAX_PATH]{};
    if (!IsCleoGameEntitiesCaller(returnAddress, moduleName, sizeof(moduleName))) {
        return false;
    }

    if (CallerAlreadyUsesRelocatedRadarTrace(returnAddress, moduleName)) {
        return false;
    }

    const uint32_t shadowIndex = traceIndex < kOriginalRadarTraceCount ?
        traceIndex : kLegacyCleoRadarTraceShadowSlot;
    const uintptr_t destination = kOriginalRadarTrace + shadowIndex * kRadarTraceSize;

    if (!CopyMemoryWithProtect(destination, trace, kRadarTraceSize)) {
        static LONG failedLogs = 0;
        const LONG count = InterlockedIncrement(&failedLogs);
        if (count <= 16) {
            Log("target blip bridge: failed shadow copy blip=0x%08X srcIndex=%u dstIndex=%u src=0x%08X dst=0x%08X caller=%s+0x%08X",
                blip, traceIndex, shadowIndex, trace, destination, moduleName, returnAddress);
        }
        return false;
    }

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    ReadRadarTraceFields(trace, &x, &y, &z, nullptr, nullptr, nullptr);

    static LONG shadowLogs = 0;
    const LONG count = InterlockedIncrement(&shadowLogs);
    if (count <= 64) {
        Log("target blip bridge: shadowed blip=0x%08X srcIndex=%u dstIndex=%u coords=(%.2f, %.2f, %.2f) caller=%s+0x%08X",
            blip, traceIndex, shadowIndex, x, y, z, moduleName, returnAddress);
    }

    *outIndex = static_cast<int>(shadowIndex);
    return true;
}

bool ResolveTargetBlipByWaypointScan(uint32_t blip, uint16_t counter, uintptr_t returnAddress, int* outIndex)
{
    if (!g_config.enableCleoTargetBlipCoordsBridge || !outIndex) {
        return false;
    }

    uint32_t targetBlip = 0;
    if (!ReadU32(kFrontEndTargetBlipIndex, &targetBlip) || targetBlip != blip) {
        return false;
    }

    uint32_t foundIndex = 0;
    uintptr_t foundTrace = 0;
    if (!FindRelocatedWaypointTrace(counter, &foundIndex, &foundTrace)) {
        static LONG missLogs = 0;
        const LONG count = InterlockedIncrement(&missLogs);
        if (count <= 32) {
            Log("target blip bridge: waypoint scan miss blip=0x%08X counter=%u base=0x%08X limit=%u",
                blip, counter, g_radarTraceBase, g_radarTraceLimit);
        }
        return false;
    }

    int shadowIndex = -1;
    if (ShadowRadarTraceForLegacyCleo(blip, foundIndex, foundTrace, returnAddress, &shadowIndex)) {
        *outIndex = shadowIndex;
        return true;
    }

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    uint16_t actualCounter = 0;
    uint8_t sprite = 0;
    uint8_t flags = 0;
    ReadRadarTraceFields(foundTrace, &x, &y, &z, &actualCounter, &sprite, &flags);

    char moduleName[MAX_PATH]{};
    ModuleBaseFromAddress(returnAddress, moduleName, sizeof(moduleName));

    static LONG repairLogs = 0;
    const LONG count = InterlockedIncrement(&repairLogs);
    if (count <= 64) {
        Log("target blip bridge: repaired target blip=0x%08X badIndex=%u returnIndex=%u counter=%u actualCounter=%u sprite=%u flags=0x%02X coords=(%.2f, %.2f, %.2f) caller=%s+0x%08X",
            blip,
            blip & 0xFFFFu,
            foundIndex,
            counter,
            actualCounter,
            sprite,
            flags,
            x,
            y,
            z,
            moduleName,
            returnAddress);
    }

    *outIndex = static_cast<int>(foundIndex);
    return true;
}

extern "C" int __cdecl Bridge_CRadar_GetActualBlipArrayIndex(uint32_t blip)
{
    const uintptr_t returnAddress = reinterpret_cast<uintptr_t>(_ReturnAddress());

    if (blip == 0xFFFFFFFFu) {
        return -1;
    }

    const uint32_t traceIndex = blip & 0xFFFFu;
    const uint16_t counter = static_cast<uint16_t>(blip >> 16);
    const uint32_t limit = g_radarTraceLimit ? g_radarTraceLimit : 175;

    if (traceIndex >= limit) {
        int repairedIndex = -1;
        if (ResolveTargetBlipByWaypointScan(blip, counter, returnAddress, &repairedIndex)) {
            return repairedIndex;
        }

        static LONG invalidHighHandleLogs = 0;
        const LONG count = InterlockedIncrement(&invalidHighHandleLogs);
        if (count <= 32) {
            Log("radar blip guard: rejected out-of-range blip=0x%08X index=%u counter=%u limit=%u base=0x%08X",
                blip, traceIndex, counter, limit, g_radarTraceBase);
        }
        return -1;
    }

    const uintptr_t trace = (g_radarTraceBase ? g_radarTraceBase : kOriginalRadarTrace) + traceIndex * kRadarTraceSize;
    if (!IsReadableCommitted(trace + kRadarTraceCounterOffset, sizeof(uint16_t)) ||
        !IsReadableCommitted(trace + kRadarTraceFlagOffset, sizeof(uint8_t))) {
        static LONG unreadableLogs = 0;
        const LONG count = InterlockedIncrement(&unreadableLogs);
        if (count <= 16) {
            Log("radar blip guard: trace unreadable blip=0x%08X index=%u trace=0x%08X limit=%u",
                blip, traceIndex, trace, limit);
        }
        return -1;
    }

    uint16_t actualCounter = 0;
    uint8_t flags = 0;
    __try {
        std::memcpy(&actualCounter, reinterpret_cast<const void*>(trace + kRadarTraceCounterOffset), sizeof(actualCounter));
        std::memcpy(&flags, reinterpret_cast<const void*>(trace + kRadarTraceFlagOffset), sizeof(flags));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }

    if (counter != actualCounter || !(flags & kRadarTraceTrackingFlag)) {
        int repairedIndex = -1;
        if (ResolveTargetBlipByWaypointScan(blip, counter, returnAddress, &repairedIndex)) {
            return repairedIndex;
        }
        return -1;
    }

    uint32_t targetBlip = 0;
    if (ReadU32(kFrontEndTargetBlipIndex, &targetBlip) && targetBlip == blip) {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        uint8_t sprite = 0;
        uint8_t currentFlags = 0;
        if (ReadRadarTraceFields(trace, &x, &y, &z, nullptr, &sprite, &currentFlags)) {
            RememberTargetBlipCoords(x, y, z);
            static LONG targetTraceLogs = 0;
            const LONG count = InterlockedIncrement(&targetTraceLogs);
            if (count <= 64) {
                Log("target blip bridge: valid target trace blip=0x%08X index=%u counter=%u sprite=%u flags=0x%02X coords=(%.2f, %.2f, %.2f)",
                    blip, traceIndex, counter, sprite, currentFlags, x, y, z);
            }

            if (!IsSaneRadarCoord(x, y, z) || sprite != kRadarSpriteWaypoint) {
                int repairedIndex = -1;
                if (ResolveTargetBlipByWaypointScan(blip, counter, returnAddress, &repairedIndex)) {
                    return repairedIndex;
                }
            }
        }
    }

    int shadowIndex = -1;
    if (ShadowRadarTraceForLegacyCleo(blip, traceIndex, trace, returnAddress, &shadowIndex)) {
        return shadowIndex;
    }

    return static_cast<int>(traceIndex);
}

void InstallRadarBlipHandleGuard()
{
#if defined(_M_IX86)
    RefreshRadarTraceRuntimeState();

    const uintptr_t target = reinterpret_cast<uintptr_t>(Bridge_CRadar_GetActualBlipArrayIndex);
    const uintptr_t currentTarget = DecodeRel32JumpTarget(kCRadarGetActualBlipArrayIndex);

    Log("radar blip guard: base=0x%08X limit=%u iniLimit=%u patchedLimit=%u currentTarget=0x%08X bridge=0x%08X",
        g_radarTraceBase,
        g_radarTraceLimit,
        static_cast<uint32_t>(ReadFlaInt("Radar traces", 175, 1, 65535)),
        ReadPatchedRadarTraceLimit(),
        currentTarget,
        target);

    if (currentTarget == target) {
        return;
    }

    if (WriteRel32Jump(kCRadarGetActualBlipArrayIndex, target)) {
        Log("radar blip guard: installed at 0x%08X", kCRadarGetActualBlipArrayIndex);
    }
#else
    Log("radar blip guard: unsupported architecture");
#endif
}

void InstallPickupModelLoadGuard()
{
#if defined(_M_IX86)
    constexpr uintptr_t patchAddress = kCPickupGiveUsAPickUpObject;
    static const uint8_t expectedBytes[] = {
        0x83, 0xEC, 0x0C,       // sub esp, 0Ch
        0x56,                   // push esi
        0x8B, 0x74, 0x24, 0x14  // mov esi, [esp+14h]
    };

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("pickup model guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("pickup model guard: read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_CPickup_GiveUsAPickUpObject_Guard);
    if (currentTarget == guardTarget) {
        Log("pickup model guard: already installed at CPickup::GiveUsAPickUpObject");
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("pickup model guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X %02X %02X %02X currentTarget=0x%08X",
            patchAddress,
            current[0],
            current[1],
            current[2],
            current[3],
            current[4],
            current[5],
            current[6],
            current[7],
            currentTarget);
        return;
    }

    uint8_t patch[sizeof(expectedBytes)]{};
    patch[0] = 0xE9;
    const int32_t rel = static_cast<int32_t>(guardTarget - (patchAddress + 5));
    std::memcpy(patch + 1, &rel, sizeof(rel));
    for (size_t i = 5; i < sizeof(patch); ++i) {
        patch[i] = 0x90;
    }

    if (WriteBytesWithProtect(patchAddress, patch, sizeof(patch))) {
        Log("pickup model guard: installed at CPickup::GiveUsAPickUpObject target=0x%08X allowlist='%s' flags=0x%X loadNow=%d",
            guardTarget,
            g_config.pickupModelLoadGuardAllowlist,
            g_config.pickupModelLoadGuardFlags,
            g_config.pickupModelLoadGuardLoadNow ? 1 : 0);
    }
#else
    Log("pickup model guard: unsupported architecture");
#endif
}

void InstallGetBoundCentreNullGuard()
{
#if defined(_M_IX86)
    constexpr uintptr_t patchAddress = 0x00533650;
    constexpr uintptr_t continueAddress = 0x0053365B;
    static const uint8_t expectedBytes[] = {
        0x0F, 0xBF, 0x41, 0x22,             // movsx eax, word ptr [ecx+22h]
        0x8B, 0x14, 0x85, 0xC8, 0xB0, 0xA9, 0x00 // mov edx, [eax*4+00A9B0C8h]
    };

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("bound centre guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("bound centre guard: read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_GetBoundCentre_NullGuard);
    if (currentTarget == guardTarget) {
        Log("bound centre guard: already installed at 0x%08X", patchAddress);
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("bound centre guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X currentTarget=0x%08X",
            patchAddress,
            current[0], current[1], current[2], current[3], current[4], current[5],
            current[6], current[7], current[8], current[9], current[10],
            currentTarget);
        return;
    }

    uint8_t patch[sizeof(expectedBytes)]{};
    patch[0] = 0xE9;
    const int32_t rel = static_cast<int32_t>(guardTarget - (patchAddress + 5));
    std::memcpy(patch + 1, &rel, sizeof(rel));
    for (size_t i = 5; i < sizeof(patch); ++i) {
        patch[i] = 0x90;
    }

    if (WriteBytesWithProtect(patchAddress, patch, sizeof(patch))) {
        Log("bound centre guard: installed at 0x%08X target=0x%08X continue=0x%08X",
            patchAddress, guardTarget, continueAddress);
    }
#else
    Log("bound centre guard: unsupported architecture");
#endif
}

void InstallGetBoundRectColModelGuard()
{
#if defined(_M_IX86)
    // FLA's 0x534126 patch performs the required extended-ID lookup. Hook the
    // first collision-model dereference after that lookup so normal entities
    // pay only a null test instead of a second hash lookup and VirtualQuery set.
    constexpr uintptr_t patchAddress = kFlaNoCollisionErrorPatch;
    constexpr uintptr_t continueAddress = 0x0053413A;
    static const uint8_t expectedBytes[] = {
        0x8B, 0x10,                   // mov edx, [eax]
        0x89, 0x54, 0x24, 0x10        // mov [esp+10h], edx
    };

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("bound rect guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("bound rect guard: read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_GetBoundRect_ColModelGuard);
    if (currentTarget == guardTarget) {
        Log("bound rect guard: already installed at 0x%08X", patchAddress);
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("bound rect guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X %02X currentTarget=0x%08X",
            patchAddress,
            current[0], current[1], current[2], current[3], current[4], current[5],
            currentTarget);
        return;
    }

    uint8_t patch[sizeof(expectedBytes)]{};
    patch[0] = 0xE9;
    const int32_t rel = static_cast<int32_t>(guardTarget - (patchAddress + 5));
    std::memcpy(patch + 1, &rel, sizeof(rel));
    for (size_t i = 5; i < sizeof(patch); ++i) {
        patch[i] = 0x90;
    }

    if (WriteBytesWithProtect(patchAddress, patch, sizeof(patch))) {
        Log("bound rect guard: installed at 0x%08X target=0x%08X continue=0x%08X",
            patchAddress, guardTarget, continueAddress);
    }
#else
    Log("bound rect guard: unsupported architecture");
#endif
}

