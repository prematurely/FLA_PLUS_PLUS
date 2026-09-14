#include "FLACompatBridgeInternal.h"

bool FileNameLooksLikeNodeDat(const char* name, uint32_t* slotOut)
{
    if (!name || _strnicmp(name, "nodes", 5) != 0) {
        return false;
    }

    const char* p = name + 5;
    if (*p < '0' || *p > '9') {
        return false;
    }

    uint32_t value = 0;
    while (*p >= '0' && *p <= '9') {
        value = value * 10u + static_cast<uint32_t>(*p - '0');
        ++p;
    }

    if (_stricmp(p, ".dat") != 0) {
        return false;
    }

    if (slotOut) {
        *slotOut = value;
    }
    return true;
}

void ScanLooseNodeDatFiles(const char* root, int depth, uint32_t* count, uint32_t* maxSlot)
{
    if (!root || !count || !maxSlot || depth > 12 || GetFileAttributesA(root) == INVALID_FILE_ATTRIBUTES) {
        return;
    }

    char pattern[MAX_PATH]{};
    sprintf_s(pattern, "%s\\*", root);

    WIN32_FIND_DATAA data{};
    HANDLE find = FindFirstFileA(pattern, &data);
    if (find == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        if (std::strcmp(data.cFileName, ".") == 0 || std::strcmp(data.cFileName, "..") == 0) {
            continue;
        }

        char path[MAX_PATH]{};
        sprintf_s(path, "%s\\%s", root, data.cFileName);

        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            ScanLooseNodeDatFiles(path, depth + 1, count, maxSlot);
            continue;
        }

        uint32_t slot = 0;
        if (FileNameLooksLikeNodeDat(data.cFileName, &slot)) {
            ++(*count);
            if (slot > *maxSlot) {
                *maxSlot = slot;
            }
        }
    } while (FindNextFileA(find, &data));

    FindClose(find);
}

void LogFlaPathNodeDiagnostics()
{
    if (!g_config.enableFlaPathNodeDiagnostics) {
        return;
    }

    const uint32_t dff = ReadIniU32("FILE_TYPE_DFF", 20000);
    const uint32_t txd = ReadIniU32("FILE_TYPE_TXD", 5000);
    const uint32_t col = ReadIniU32("FILE_TYPE_COL", 255);
    const uint32_t ipl = ReadIniU32("FILE_TYPE_IPL", 256);
    const uint32_t datBase = dff + txd + col + ipl;
    const uint32_t pathsMapSize = ReadIniU32("Paths map size", 6000);
    constexpr uint32_t kPathBlockSize = 750;
    const uint32_t tilesPerDimension = pathsMapSize / kPathBlockSize;
    const uint32_t requiredDatFiles = tilesPerDimension * tilesPerDimension;
    const uint32_t datLast = requiredDatFiles ? datBase + requiredDatFiles - 1 : datBase;

    char pathPatchText[64]{};
    char pathDebugText[64]{};
    const bool pathPatch = ReadSmallTextValue(g_flaIniPath, "Apply paths limit patch", pathPatchText, sizeof(pathPatchText)) &&
        IniValueLooksEnabled(pathPatchText);
    const bool pathDebug = ReadSmallTextValue(g_flaIniPath, "Enable path debugging", pathDebugText, sizeof(pathDebugText)) &&
        IniValueLooksEnabled(pathDebugText);

    uint32_t looseNodes = 0;
    uint32_t looseMaxSlot = 0;
    ScanLooseNodeDatFiles("data", 0, &looseNodes, &looseMaxSlot);
    ScanLooseNodeDatFiles("modloader", 0, &looseNodes, &looseMaxSlot);

    Log("FLA path diag: pathsMapSize=%u pathPatch=%d pathDebug=%d tilesPerDimension=%u requiredDatNodes=%u datBase=%u datLast=%u looseNodesDat=%u looseMaxSlot=%u",
        pathsMapSize,
        pathPatch ? 1 : 0,
        pathDebug ? 1 : 0,
        tilesPerDimension,
        requiredDatFiles,
        datBase,
        datLast,
        looseNodes,
        looseMaxSlot);

    if (pathsMapSize != 6000 || requiredDatFiles > 64) {
        Log("FLA path diag: expanded Paths map size requires a matching nodes0.dat..nodes%u.dat set; missing entries produce 0x4087EA undefined DAT ID requests",
            requiredDatFiles ? requiredDatFiles - 1 : 0);
        if (requiredDatFiles > 64) {
            const uint32_t sampleSlot = requiredDatFiles > 1890 ? 1890 : 64;
            Log("FLA path diag: example mapping globalID=%u -> nodes%u.dat",
                datBase + sampleSlot,
                sampleSlot);
        }
        if (looseNodes == 0 || looseMaxSlot + 1 < requiredDatFiles) {
            Log("FLA path diag: installed loose nodes*.dat files do not cover the configured expanded DAT range");
        }
    }
}

void LogOneRelocatedAddress(const char* label, const char* logPrefix, uintptr_t originalAddress)
{
    uintptr_t relocated = 0;
    if (!ReadLogAddress(logPrefix, &relocated)) {
        Log("relocated diag: %s not found in FLA log", label);
        return;
    }

    Log("relocated diag: %s original=0x%08X relocated=0x%08X moved=%d",
        label, originalAddress, relocated, relocated != originalAddress ? 1 : 0);
    if (relocated) {
        LogMemoryRegion(label, relocated);
    }
}

void LogRelocatedAddressDiagnostics()
{
    if (!g_config.enableRelocatedAddressDiagnostics) {
        return;
    }

    LogOneRelocatedAddress("CModelInfo::ms_modelInfoPtrs", "CModelInfo::ms_modelInfoPtrs:", kOriginalCModelInfoPtrs);
    LogOneRelocatedAddress("CStreaming::ms_aInfoForModel", "CStreaming::ms_aInfoForModel:", kOriginalStreamingInfo);
    LogOneRelocatedAddress("CAnimManager::ms_aAnimBlocks", "CAnimManager::ms_aAnimBlocks:", kOriginalAnimBlocks);
    LogOneRelocatedAddress("CVehicleRecording::StreamingArray", "CVehicleRecording::StreamingArray:", 0);
    LogOneRelocatedAddress("CTheScripts::StreamedScripts", "CTheScripts::StreamedScripts:", 0);
    LogOneRelocatedAddress("&mod_HandlingManager", "&mod_HandlingManager =", 0);
    Log("relocated diag: CRadar::ms_RadarTrace original=0x%08X relocated=0x%08X moved=%d limit=%u",
        kOriginalRadarTrace,
        g_radarTraceBase,
        g_radarTraceBase != kOriginalRadarTrace ? 1 : 0,
        g_radarTraceLimit);
    if (g_radarTraceBase) {
        LogMemoryRegion("CRadar::ms_RadarTrace", g_radarTraceBase);
    }
}

void LogOnePoolPointer(const char* label, uintptr_t pointerAddress)
{
    uint32_t value = 0;
    if (!SafeReadU32(pointerAddress, &value)) {
        Log("pool diag: %s pointerAddress=0x%08X unreadable", label, pointerAddress);
        return;
    }

    Log("pool diag: %s pointerAddress=0x%08X value=0x%08X", label, pointerAddress, value);
    if (value) {
        LogMemoryRegion(label, value);
        LogDwords(label, value, 12);
    }
}

void LogPoolPointerDiagnostics()
{
    if (!g_config.enablePoolPointerDiagnostics) {
        return;
    }

    LogOnePoolPointer("CPools::ms_pPedPool", kOriginalPedPoolPtr);
    LogOnePoolPointer("CPools::ms_pVehiclePool", kOriginalVehiclePoolPtr);
    LogOnePoolPointer("CPools::ms_pBuildingPool", kOriginalBuildingPoolPtr);
    LogOnePoolPointer("CPools::ms_pObjectPool", kOriginalObjectPoolPtr);
    LogOnePoolPointer("CPools::ms_pDummyPool", kOriginalDummyPoolPtr);
    LogOnePoolPointer("CPools::ms_pColModelPool", kOriginalColModelPoolPtr);
    LogOnePoolPointer("CColStore::ms_pColPool", kCColStorePoolPtr);
}

void LogPopulationPoolUsage(const char* label, uintptr_t poolPtrAddress)
{
    uint32_t poolValue = 0;
    if (!SafeReadU32(poolPtrAddress, &poolValue) || !poolValue) {
        Log("population diag: %s poolPtr=0x%08X pool=0x%08X unreadable-or-null",
            label, poolPtrAddress, poolValue);
        return;
    }

    uintptr_t objects = 0;
    uintptr_t byteMap = 0;
    uint32_t size = 0;
    uint32_t firstFree = 0xFFFFFFFF;
    if (!ReadPoolByteMap(poolValue, &objects, &byteMap, &size, &firstFree)) {
        Log("population diag: %s poolPtr=0x%08X pool=0x%08X invalid-header",
            label, poolPtrAddress, poolValue);
        return;
    }

    if (!objects || !byteMap || !IsReadableCommitted(byteMap, static_cast<size_t>(size))) {
        Log("population diag: %s pool=0x%08X objects=0x%08X byteMap=0x%08X size=%u firstFree=%u byteMapReadable=0",
            label, poolValue, objects, byteMap, size, firstFree);
        return;
    }

    uint32_t usedByHighEmptyBit = 0;
    uint32_t freeByHighEmptyBit = 0;
    uint32_t usedByLowEmptyBit = 0;
    uint32_t freeByLowEmptyBit = 0;
    uint32_t nonZeroFlags = 0;
    uint8_t firstFlags[16]{};
    const uint32_t firstCount = size < static_cast<uint32_t>(sizeof(firstFlags)) ? size : static_cast<uint32_t>(sizeof(firstFlags));

    __try {
        const auto* flags = reinterpret_cast<const uint8_t*>(byteMap);
        for (uint32_t i = 0; i < size; ++i) {
            const uint8_t flag = flags[i];
            if (i < firstCount) {
                firstFlags[i] = flag;
            }
            if (flag) {
                ++nonZeroFlags;
            }
            if (flag & 0x80) {
                ++freeByHighEmptyBit;
            } else {
                ++usedByHighEmptyBit;
            }
            if (flag & 0x01) {
                ++freeByLowEmptyBit;
            } else {
                ++usedByLowEmptyBit;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("population diag: %s pool=0x%08X objects=0x%08X byteMap=0x%08X size=%u read-exception",
            label, poolValue, objects, byteMap, size);
        return;
    }

    char firstBytes[3 * sizeof(firstFlags) + 1]{};
    size_t pos = 0;
    for (uint32_t i = 0; i < firstCount && pos + 4 < sizeof(firstBytes); ++i) {
        const int written = sprintf_s(firstBytes + pos, sizeof(firstBytes) - pos, "%02X ", firstFlags[i]);
        if (written <= 0) {
            break;
        }
        pos += static_cast<size_t>(written);
    }

    Log("population diag: %s pool=0x%08X objects=0x%08X byteMap=0x%08X size=%u used=%u free=%u firstFree=%u nonZeroFlags=%u usedLowBit=%u freeLowBit=%u firstFlags=%s",
        label,
        poolValue,
        objects,
        byteMap,
        size,
        usedByHighEmptyBit,
        freeByHighEmptyBit,
        firstFree,
        nonZeroFlags,
        usedByLowEmptyBit,
        freeByLowEmptyBit,
        firstBytes);
}

int ReadRuntimeByteForLog(uintptr_t address)
{
    uint8_t value = 0;
    return SafeReadU8(address, &value) ? static_cast<int>(value) : -1;
}

uint32_t ReadRuntimeU32ForLog(uintptr_t address)
{
    uint32_t value = 0;
    return SafeReadU32(address, &value) ? value : 0xFFFFFFFFu;
}

float ReadRuntimeFloatForLog(uintptr_t address)
{
    float value = 0.0f;
    return SafeReadF32(address, &value) ? value : -9999.0f;
}

bool IsReasonableRuntimeFloat(float value)
{
    return value > -1000000.0f && value < 1000000.0f;
}

bool IsReasonableRuntimeVec3(const RuntimeVec3& value)
{
    return IsReasonableRuntimeFloat(value.x) &&
        IsReasonableRuntimeFloat(value.y) &&
        IsReasonableRuntimeFloat(value.z);
}

bool SafeReadVec3(uintptr_t address, RuntimeVec3* out)
{
    if (!out) {
        return false;
    }

    RuntimeVec3 value{};
    if (!SafeReadF32(address + 0x00, &value.x) ||
        !SafeReadF32(address + 0x04, &value.y) ||
        !SafeReadF32(address + 0x08, &value.z) ||
        !IsReasonableRuntimeVec3(value)) {
        return false;
    }

    *out = value;
    return true;
}

bool ReadEntityPosition(uintptr_t entity, RuntimeVec3* out, uintptr_t* matrixOut, bool* fromMatrixOut)
{
    if (!out) {
        return false;
    }

    if (matrixOut) {
        *matrixOut = 0;
    }
    if (fromMatrixOut) {
        *fromMatrixOut = false;
    }

    uint32_t matrix32 = 0;
    if (SafeReadU32(entity + kCPlaceableMatrixOffset, &matrix32) && matrix32) {
        const uintptr_t matrix = static_cast<uintptr_t>(matrix32);
        if (matrixOut) {
            *matrixOut = matrix;
        }
        if (SafeReadVec3(matrix + kCMatrixPositionOffset, out)) {
            if (fromMatrixOut) {
                *fromMatrixOut = true;
            }
            return true;
        }
    }

    return SafeReadVec3(entity + kCPlaceablePlacementPosOffset, out);
}

uintptr_t ReadFocusedPlayerPed()
{
    uint32_t ped = 0;
    if (!SafeReadU32(kCWorldPlayers + kCPlayerInfoPedOffset, &ped)) {
        return 0;
    }
    return static_cast<uintptr_t>(ped);
}

float DistanceSquared2D(const RuntimeVec3& a, const RuntimeVec3& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

uint32_t ReadZoneStreamingCheatMaskForLog()
{
    uint32_t mask = 0;
    for (const auto& flag : kZoneStreamingCheatFlags) {
        if (ReadRuntimeByteForLog(kCheatsActive + flag.index) > 0) {
            mask |= flag.mask;
        }
    }
    return mask;
}

void LogPopulationRuntimeState(const char* reason, int sampleIndex)
{
    const int onlyGang = ReadRuntimeByteForLog(kPopulationOnlyCreateRandomGangMembers);
    const int dontGang = ReadRuntimeByteForLog(kPopulationDontCreateRandomGangMembers);
    const int dontCops = ReadRuntimeByteForLog(kPopulationDontCreateRandomCops);
    const int cheatGangEverywhere = ReadRuntimeByteForLog(kCheatsActive + kCheatGangMembersEverywhere);
    const int cheatGangLand = ReadRuntimeByteForLog(kCheatsActive + kCheatGangsControlStreets);
    const uint32_t streamRequested = ReadRuntimeU32ForLog(kStreamingNumModelsRequested);
    const int loadingPriority = ReadRuntimeByteForLog(kRendererLoadingPriority);
    const int disableStreaming = ReadRuntimeByteForLog(kStreamingDisableStreaming);
    const int loadingBigModel = ReadRuntimeByteForLog(kStreamingLoadingBigModel);
    const uint32_t zoneStreamingMask = ReadZoneStreamingCheatMaskForLog();
    const bool streamBusy = loadingPriority > 0 || streamRequested > static_cast<uint32_t>(g_config.streamingBusyThreshold);

    Log("population runtime: reason=%s sample=%d onlyGang=%d dontGang=%d dontCops=%d cheatGangEverywhere=%d cheatGangLand=%d zoneCheats=0x%02X streamBusy=%d requested=%u threshold=%d priorityReq=%u loadingPriority=%d disableStreaming=%d bigModel=%d msPeds=%u streamZone=%u density=%.3f maxPeds=%u popTotals total=%u mission=%u gang=%u civ=%u civMale=%u civFemale=%u cops=%u dealers=%u worldZone=%u popcycle other=%.2f cops=%.2f gangs=%.2f dealers=%.2f percOther=%.2f percCops=%.2f percGangs=%.2f zoneType=%u",
        reason ? reason : "",
        sampleIndex,
        onlyGang,
        dontGang,
        dontCops,
        cheatGangEverywhere,
        cheatGangLand,
        zoneStreamingMask,
        streamBusy ? 1 : 0,
        streamRequested,
        g_config.streamingBusyThreshold,
        ReadRuntimeU32ForLog(kStreamingNumPriorityRequests),
        loadingPriority,
        disableStreaming,
        loadingBigModel,
        ReadRuntimeU32ForLog(kStreamingNumPedsLoaded),
        ReadRuntimeU32ForLog(kStreamingCurrentZoneType),
        ReadRuntimeFloatForLog(kPopulationPedDensityMultiplier),
        ReadRuntimeU32ForLog(kPopulationMaxNumberOfPedsInUse),
        ReadRuntimeU32ForLog(kPopulationTotalPeds),
        ReadRuntimeU32ForLog(kPopulationTotalMissionPeds),
        ReadRuntimeU32ForLog(kPopulationTotalGangPeds),
        ReadRuntimeU32ForLog(kPopulationTotalCivPeds),
        ReadRuntimeU32ForLog(kPopulationNumCivMale),
        ReadRuntimeU32ForLog(kPopulationNumCivFemale),
        ReadRuntimeU32ForLog(kPopulationNumCops),
        ReadRuntimeU32ForLog(kPopulationNumDealers),
        ReadRuntimeU32ForLog(kPopulationCurrentWorldZone),
        ReadRuntimeFloatForLog(kPopCycleNumOtherPeds),
        ReadRuntimeFloatForLog(kPopCycleNumCopsPeds),
        ReadRuntimeFloatForLog(kPopCycleNumGangsPeds),
        ReadRuntimeFloatForLog(kPopCycleNumDealersPeds),
        ReadRuntimeFloatForLog(kPopCyclePercOther),
        ReadRuntimeFloatForLog(kPopCyclePercCops),
        ReadRuntimeFloatForLog(kPopCyclePercGangs),
        ReadRuntimeU32ForLog(kPopCycleCurrentZoneType));
}

void AddPedModelCounter(PedModelCounter* counters, size_t count, int32_t modelId)
{
    if (!counters || modelId < 0) {
        return;
    }

    for (size_t i = 0; i < count; ++i) {
        if (counters[i].modelId == modelId) {
            ++counters[i].count;
            return;
        }
    }

    for (size_t i = 0; i < count; ++i) {
        if (counters[i].modelId < 0) {
            counters[i].modelId = modelId;
            counters[i].count = 1;
            return;
        }
    }
}

void AppendTopPedModels(char* output, size_t outputSize, PedModelCounter* counters, size_t counterCount)
{
    if (!output || outputSize == 0 || !counters) {
        return;
    }

    output[0] = '\0';
    size_t used = 0;
    bool selected[64]{};
    const size_t maxCounters = counterCount < 64 ? counterCount : 64;
    for (size_t rank = 0; rank < 12; ++rank) {
        size_t best = maxCounters;
        for (size_t i = 0; i < maxCounters; ++i) {
            if (selected[i] || counters[i].modelId < 0 || counters[i].count == 0) {
                continue;
            }
            if (best == maxCounters || counters[i].count > counters[best].count) {
                best = i;
            }
        }

        if (best == maxCounters) {
            break;
        }

        selected[best] = true;
        const int written = sprintf_s(output + used,
            outputSize - used,
            "%s%d:%u",
            used ? "," : "",
            counters[best].modelId,
            counters[best].count);
        if (written <= 0) {
            break;
        }
        used += static_cast<size_t>(written);
        if (used + 16 >= outputSize) {
            break;
        }
    }
}

bool IsPedRaceAllowedByCurrentZone(uint32_t race, uint8_t zoneRaces, bool hasZoneInfo)
{
    if (race == 0) {
        return true;
    }
    if (!hasZoneInfo || race > 4) {
        return false;
    }
    return (zoneRaces & (1u << (race - 1))) != 0;
}

void AppendCurrentPopcycleGroupRow(char* output, size_t outputSize)
{
    if (!output || outputSize == 0) {
        return;
    }
    output[0] = '\0';

    const uint32_t timeIndex = ReadRuntimeU32ForLog(kPopCycleCurrentTimeIndex);
    const uint32_t timeOfWeek = ReadRuntimeU32ForLog(kPopCycleCurrentTimeOfWeek);
    const uint32_t zoneType = ReadRuntimeU32ForLog(kPopCycleCurrentZoneType);
    if (timeIndex >= 12 || timeOfWeek >= 2 || zoneType >= 20) {
        sprintf_s(output, outputSize, "invalid time=%u week=%u zone=%u", timeIndex, timeOfWeek, zoneType);
        return;
    }

    const uintptr_t row = kPopCyclePercTypeGroup +
        (((static_cast<uintptr_t>(timeIndex) * 2u + timeOfWeek) * 20u + zoneType) * 18u);
    size_t used = 0;
    for (uint32_t i = 0; i < 18; ++i) {
        uint8_t value = 0xFF;
        SafeReadU8(row + i, &value);
        const int written = sprintf_s(output + used,
            outputSize - used,
            "%s%u",
            i ? "," : "",
            static_cast<unsigned>(value));
        if (written <= 0) {
            break;
        }
        used += static_cast<size_t>(written);
        if (used + 8 >= outputSize) {
            break;
        }
    }
}

void LogPopulationStreamingPedSlots(const char* reason, int sampleIndex)
{
    uint32_t zoneInfo32 = 0;
    SafeReadU32(kPopCycleCurrentZoneInfo, &zoneInfo32);
    const uintptr_t zoneInfo = static_cast<uintptr_t>(zoneInfo32);
    const bool hasZoneInfo = zoneInfo && IsReadableCommitted(zoneInfo, 0x11);

    uint8_t zoneFlags = 0xFF;
    uint8_t zoneRaces = 0xFF;
    if (hasZoneInfo) {
        SafeReadU8(zoneInfo + 0x0F, &zoneFlags);
        SafeReadU8(zoneInfo + 0x10, &zoneRaces);
    }

    uint32_t loadedCount = ReadRuntimeU32ForLog(kStreamingNumPedsLoaded);
    uint32_t loadedGangs = 0xFFFFFFFFu;
    SafeReadU32(kStreamingLoadedGangs, &loadedGangs);

    uint32_t activeSlots = 0;
    uint32_t streamLoadedSlots = 0;
    uint32_t pedInfoSlots = 0;
    uint32_t rwSlots = 0;
    uint32_t refOkSlots = 0;
    uint32_t raceOkSlots = 0;
    uint32_t defaultCandidatesFirst3 = 0;
    uint32_t defaultCandidatesAll = 0;
    char slots[4096]{};
    size_t used = 0;

    for (uint32_t i = 0; i < 8; ++i) {
        uint32_t rawModel = 0xFFFFFFFFu;
        SafeReadU32(kStreamingPedsLoaded + i * sizeof(uint32_t), &rawModel);
        const int32_t modelId = static_cast<int32_t>(rawModel);
        if (modelId < 0) {
            const int written = sprintf_s(slots + used, sizeof(slots) - used, "%ss%u=-1", used ? "," : "", i);
            if (written > 0) {
                used += static_cast<size_t>(written);
            }
            continue;
        }

        ++activeSlots;
        const uint8_t streamState = GetStreamingLoadState(static_cast<uint32_t>(modelId));
        const bool streamLoaded = streamState == 1;
        if (streamLoaded) {
            ++streamLoadedSlots;
        }

        uint32_t streamFlags = 0xFFFFFFFFu;
        uint32_t cdSize = 0xFFFFFFFFu;
        const uintptr_t streamingEntry = SafeStreamingInfoEntryAddress(static_cast<uint32_t>(modelId));
        if (streamingEntry) {
            SafeReadU32(streamingEntry + 0x06, &streamFlags);
            SafeReadU32(streamingEntry + 0x0C, &cdSize);
        }

        uint32_t modelInfo = 0;
        const uintptr_t modelEntry = SafeModelInfoEntryAddress(static_cast<uint32_t>(modelId));
        if (modelEntry) {
            SafeReadU32(modelEntry, &modelInfo);
        }
        if (!modelInfo && static_cast<uint32_t>(modelId) < kOriginalCModelInfoCount) {
            SafeReadU32(kOriginalCModelInfoPtrs + static_cast<uintptr_t>(modelId) * sizeof(uintptr_t), &modelInfo);
        }

        uint32_t vtable = 0;
        uint32_t refAndTxd = 0xFFFFFFFFu;
        uint32_t rwObject = 0;
        uint32_t animType = 0xFFFFFFFFu;
        uint32_t pedType = 0xFFFFFFFFu;
        uint32_t statType = 0xFFFFFFFFu;
        uint32_t carsAndPedFlags = 0xFFFFFFFFu;
        uint8_t race = 0xFF;
        if (modelInfo && IsReadableCommitted(modelInfo, 0x44)) {
            SafeReadU32(modelInfo, &vtable);
            SafeReadU32(modelInfo + 0x08, &refAndTxd);
            SafeReadU32(modelInfo + 0x1C, &rwObject);
            SafeReadU32(modelInfo + 0x24, &animType);
            SafeReadU32(modelInfo + 0x28, &pedType);
            SafeReadU32(modelInfo + 0x2C, &statType);
            SafeReadU32(modelInfo + 0x30, &carsAndPedFlags);
            SafeReadU8(modelInfo + 0x3A, &race);
        }

        const bool pedInfo = vtable == kPedModelInfoVtable;
        const uint32_t refCount = refAndTxd & 0xFFFFu;
        const bool refOk = refCount == i;
        const bool raceOk = IsPedRaceAllowedByCurrentZone(race, zoneRaces & 0x0Fu, hasZoneInfo);
        const bool defaultCandidate = pedInfo && streamLoaded && rwObject && refOk && raceOk;

        if (pedInfo) {
            ++pedInfoSlots;
        }
        if (rwObject) {
            ++rwSlots;
        }
        if (refOk) {
            ++refOkSlots;
        }
        if (raceOk) {
            ++raceOkSlots;
        }
        if (defaultCandidate) {
            ++defaultCandidatesAll;
            if (i < 3) {
                ++defaultCandidatesFirst3;
            }
        }

        if (used + 360 < sizeof(slots)) {
            const int written = sprintf_s(slots + used,
                sizeof(slots) - used,
                "%ss%u=m%d/st%u/fl%02X/cd%u/mi%08X/vt%08X/rw%08X/ref%u/ped%u/stat%u/anim%u/cars%04X/race%u/raceOk%d/refOk%d/cand%d",
                used ? "," : "",
                i,
                modelId,
                static_cast<unsigned>(streamState),
                static_cast<unsigned>(streamFlags & 0xFFu),
                cdSize,
                modelInfo,
                vtable,
                rwObject,
                refCount,
                pedType,
                statType,
                animType,
                carsAndPedFlags & 0xFFFFu,
                static_cast<unsigned>(race),
                raceOk ? 1 : 0,
                refOk ? 1 : 0,
                defaultCandidate ? 1 : 0);
            if (written > 0) {
                used += static_cast<size_t>(written);
            }
        }
    }

    char groupRow[128]{};
    AppendCurrentPopcycleGroupRow(groupRow, sizeof(groupRow));

    Log("population streaming peds: reason=%s sample=%d ms_num=%u active=%u loaded=%u pedInfo=%u rw=%u refOk=%u raceOk=%u candFirst3=%u candAll=%u streamZone=%u popZone=%u time=%u week=%u zoneCheats=0x%02X requested=%u threshold=%d zoneInfo=0x%08X zoneFlags=0x%02X zoneRaces=0x%02X loadedGangs=0x%04X groups=%s slots=%s",
        reason ? reason : "",
        sampleIndex,
        loadedCount,
        activeSlots,
        streamLoadedSlots,
        pedInfoSlots,
        rwSlots,
        refOkSlots,
        raceOkSlots,
        defaultCandidatesFirst3,
        defaultCandidatesAll,
        ReadRuntimeU32ForLog(kStreamingCurrentZoneType),
        ReadRuntimeU32ForLog(kPopCycleCurrentZoneType),
        ReadRuntimeU32ForLog(kPopCycleCurrentTimeIndex),
        ReadRuntimeU32ForLog(kPopCycleCurrentTimeOfWeek),
        ReadZoneStreamingCheatMaskForLog(),
        ReadRuntimeU32ForLog(kStreamingNumModelsRequested),
        g_config.streamingBusyThreshold,
        zoneInfo,
        static_cast<unsigned>(zoneFlags),
        static_cast<unsigned>(zoneRaces),
        loadedGangs & 0xFFFFu,
        groupRow,
        slots);
}

void LogPedPoolModelTypeSummary(const char* reason, int sampleIndex)
{
    uint32_t poolValue = 0;
    if (!SafeReadU32(kOriginalPedPoolPtr, &poolValue) || !poolValue) {
        Log("population ped summary: reason=%s sample=%d pedPool=0x%08X unavailable",
            reason ? reason : "",
            sampleIndex,
            poolValue);
        return;
    }

    uintptr_t objects = 0;
    uintptr_t byteMap = 0;
    uint32_t size = 0;
    uint32_t firstFree = 0xFFFFFFFF;
    if (!ReadPoolByteMap(poolValue, &objects, &byteMap, &size, &firstFree) ||
        !objects ||
        !byteMap ||
        !IsReadableCommitted(byteMap, static_cast<size_t>(size))) {
        Log("population ped summary: reason=%s sample=%d pedPool=0x%08X invalid objects=0x%08X byteMap=0x%08X size=%u",
            reason ? reason : "",
            sampleIndex,
            poolValue,
            objects,
            byteMap,
            size);
        return;
    }

    uint32_t used = 0;
    uint32_t readable = 0;
    uint32_t unreadable = 0;
    uint32_t typePlayers = 0;
    uint32_t typeCivMale = 0;
    uint32_t typeCivFemale = 0;
    uint32_t typeCops = 0;
    uint32_t typeGangs = 0;
    uint32_t typeDealers = 0;
    uint32_t typeEmergency = 0;
    uint32_t typeCriminal = 0;
    uint32_t typeBum = 0;
    uint32_t typeProstitute = 0;
    uint32_t typeSpecial = 0;
    uint32_t typeMission = 0;
    uint32_t typeOther = 0;
    uint32_t createdZero = 0;
    uint32_t createdGame = 0;
    uint32_t createdMission = 0;
    uint32_t createdGameMission = 0;
    uint32_t createdOther = 0;
    uint32_t alive = 0;
    uint32_t notAlive = 0;
    uint32_t positionReadable = 0;
    uint32_t matrixBacked = 0;
    uint32_t rwObjectPresent = 0;
    uint32_t visibleFlagSet = 0;
    uint32_t removeFromWorldSet = 0;
    uint32_t dontRenderSet = 0;
    uint32_t addToPopulationSet = 0;
    uint32_t fadeOutSet = 0;
    uint32_t inVehicleSet = 0;
    uint32_t areaMatch = 0;
    uint32_t collisionNodePresent = 0;
    uint32_t movingNodePresent = 0;
    uint32_t nearAlive40 = 0;
    uint32_t nearAlive80 = 0;
    uint32_t nearAlive120 = 0;
    uint32_t nearRenderable80 = 0;
    uint32_t nearCiv80 = 0;
    uint32_t nearGang80 = 0;
    uint32_t nearMission80 = 0;
    char sampleSlots[4096]{};
    size_t sampleSlotTextUsed = 0;
    uint32_t sampleSlotCount = 0;
    PedModelCounter modelCounters[64]{};
    for (size_t i = 0; i < 64; ++i) {
        modelCounters[i].modelId = -1;
    }

    const uintptr_t playerPed = ReadFocusedPlayerPed();
    RuntimeVec3 playerPos{};
    uintptr_t playerMatrix = 0;
    bool playerFromMatrix = false;
    const bool playerPosOk = playerPed != 0 && ReadEntityPosition(playerPed, &playerPos, &playerMatrix, &playerFromMatrix);
    uint8_t playerArea = 0xFF;
    if (playerPed) {
        SafeReadU8(playerPed + kCEntityAreaCodeOffset, &playerArea);
    }
    const uint32_t currentAreaRaw = ReadRuntimeU32ForLog(kCGameCurrentArea);
    const uint8_t currentArea = currentAreaRaw <= 0xFFu ? static_cast<uint8_t>(currentAreaRaw) : 0xFFu;

    __try {
        const auto* flags = reinterpret_cast<const uint8_t*>(byteMap);
        for (uint32_t i = 0; i < size; ++i) {
            if (flags[i] & 0x80) {
                continue;
            }

            ++used;
            const uintptr_t ped = objects + static_cast<uintptr_t>(i) * kPedPoolSlotSize;
            if (!IsReadableCommitted(ped, 0x5A0)) {
                ++unreadable;
                continue;
            }

            ++readable;
            const int32_t modelId = ReadExtendedIdFrom16BitField(reinterpret_cast<const void*>(ped + 0x22));
            AddPedModelCounter(modelCounters, sizeof(modelCounters) / sizeof(modelCounters[0]), modelId);

            uint32_t pedType = 0xFFFFFFFFu;
            SafeReadU32(ped + kCPedTypeOffset, &pedType);
            if (pedType <= 3) {
                ++typePlayers;
            } else if (pedType == 4) {
                ++typeCivMale;
            } else if (pedType == 5) {
                ++typeCivFemale;
            } else if (pedType == 6) {
                ++typeCops;
            } else if (pedType >= 7 && pedType <= 16) {
                ++typeGangs;
            } else if (pedType == 17) {
                ++typeDealers;
            } else if (pedType == 18 || pedType == 19) {
                ++typeEmergency;
            } else if (pedType == 20) {
                ++typeCriminal;
            } else if (pedType == 21) {
                ++typeBum;
            } else if (pedType == 22) {
                ++typeProstitute;
            } else if (pedType == 23) {
                ++typeSpecial;
            } else if (pedType >= 24 && pedType <= 31) {
                ++typeMission;
            } else {
                ++typeOther;
            }

            uint8_t createdBy = 0;
            if (SafeReadU8(ped + kCPedCreatedByOffset, &createdBy)) {
                if (createdBy == 0) {
                    ++createdZero;
                } else if (createdBy == 1) {
                    ++createdGame;
                } else if (createdBy == 2) {
                    ++createdMission;
                } else if (createdBy == 3) {
                    ++createdGameMission;
                } else {
                    ++createdOther;
                }
            }

            float health = 0.0f;
            const bool isAlive = SafeReadF32(ped + kCPedHealthOffset, &health) && health > 0.0f && health < 100000.0f;
            if (isAlive) {
                ++alive;
            } else {
                ++notAlive;
            }

            uint32_t entityFlags = 0;
            uint32_t rwObject = 0;
            uint32_t physicalFlags = 0;
            uint32_t collisionNode = 0;
            uint32_t movingNode = 0;
            uint32_t pedState = 0xFFFFFFFFu;
            uint32_t moveState = 0xFFFFFFFFu;
            uint8_t entityInfo = 0xFF;
            uint8_t areaCode = 0xFF;
            uint8_t pedFlags1 = 0;
            uint8_t pedFlags4 = 0;
            uint8_t pedFlags8 = 0;
            uint8_t pedFlags11 = 0;
            SafeReadU32(ped + kCEntityFlagsOffset, &entityFlags);
            SafeReadU32(ped + kCEntityRwObjectOffset, &rwObject);
            SafeReadU32(ped + kCPhysicalFlagsOffset, &physicalFlags);
            SafeReadU32(ped + kCPhysicalCollisionListOffset, &collisionNode);
            SafeReadU32(ped + kCPhysicalMovingListOffset, &movingNode);
            SafeReadU32(ped + kCPedStateOffset, &pedState);
            SafeReadU32(ped + kCPedMoveStateOffset, &moveState);
            SafeReadU8(ped + kCEntityInfoOffset, &entityInfo);
            SafeReadU8(ped + kCEntityAreaCodeOffset, &areaCode);
            SafeReadU8(ped + kCPedFlagsOffset + 1, &pedFlags1);
            SafeReadU8(ped + kCPedFlagsOffset + 4, &pedFlags4);
            SafeReadU8(ped + kCPedFlagsOffset + 8, &pedFlags8);
            SafeReadU8(ped + kCPedFlagsOffset + 11, &pedFlags11);

            const bool visible = (entityFlags & (1u << 7)) != 0;
            const bool removeFromWorld = (entityFlags & (1u << 11)) != 0;
            const bool inVehicle = (pedFlags1 & 0x01u) != 0;
            const bool fadeOut = (pedFlags4 & 0x08u) != 0;
            const bool dontRender = (pedFlags8 & 0x02u) != 0;
            const bool addedToPopulation = (pedFlags8 & 0x04u) != 0;
            const bool hasBeenRendered = (pedFlags11 & 0x20u) != 0;
            const bool areaMatches =
                (currentArea != 0xFFu && areaCode == currentArea) ||
                (playerArea != 0xFFu && areaCode == playerArea);

            RuntimeVec3 pedPos{};
            uintptr_t matrix = 0;
            bool fromMatrix = false;
            const bool posOk = ReadEntityPosition(ped, &pedPos, &matrix, &fromMatrix);
            float distance2D2 = -1.0f;
            const bool hasDistance = playerPosOk && posOk;
            if (posOk) {
                ++positionReadable;
            }
            if (fromMatrix) {
                ++matrixBacked;
            }
            if (rwObject) {
                ++rwObjectPresent;
            }
            if (visible) {
                ++visibleFlagSet;
            }
            if (removeFromWorld) {
                ++removeFromWorldSet;
            }
            if (dontRender) {
                ++dontRenderSet;
            }
            if (addedToPopulation) {
                ++addToPopulationSet;
            }
            if (fadeOut) {
                ++fadeOutSet;
            }
            if (inVehicle) {
                ++inVehicleSet;
            }
            if (areaMatches) {
                ++areaMatch;
            }
            if (collisionNode) {
                ++collisionNodePresent;
            }
            if (movingNode) {
                ++movingNodePresent;
            }

            if (hasDistance) {
                distance2D2 = DistanceSquared2D(pedPos, playerPos);
                const bool nonPlayerAlive = isAlive && pedType > 3;
                if (nonPlayerAlive && distance2D2 <= 40.0f * 40.0f) {
                    ++nearAlive40;
                }
                if (nonPlayerAlive && distance2D2 <= 80.0f * 80.0f) {
                    ++nearAlive80;
                    if (visible && rwObject && !dontRender && !removeFromWorld && areaMatches) {
                        ++nearRenderable80;
                    }
                    if (pedType == 4 || pedType == 5) {
                        ++nearCiv80;
                    } else if (pedType >= 7 && pedType <= 16) {
                        ++nearGang80;
                    } else if (pedType >= 24 && pedType <= 31) {
                        ++nearMission80;
                    }
                }
                if (nonPlayerAlive && distance2D2 <= 120.0f * 120.0f) {
                    ++nearAlive120;
                }
            }

            if (sampleSlotCount < 16 && sampleSlotTextUsed + 220 < sizeof(sampleSlots)) {
                const int written = sprintf_s(sampleSlots + sampleSlotTextUsed,
                    sizeof(sampleSlots) - sampleSlotTextUsed,
                    "%s%u:m%d/t%u/c%u/h%.0f/st%u/mv%u/a%u/ei%02X/pf%08X/v%d/rw%d/mat%d/add%d/dr%d/fade%d/veh%d/col%d/rend%d/d2%.0f/pos%.1f,%.1f,%.1f",
                    sampleSlotTextUsed ? "," : "",
                    i,
                    modelId,
                    pedType,
                    static_cast<unsigned>(createdBy),
                    health,
                    pedState,
                    moveState,
                    static_cast<unsigned>(areaCode),
                    static_cast<unsigned>(entityInfo),
                    physicalFlags,
                    visible ? 1 : 0,
                    rwObject ? 1 : 0,
                    matrix ? 1 : 0,
                    addedToPopulation ? 1 : 0,
                    dontRender ? 1 : 0,
                    fadeOut ? 1 : 0,
                    inVehicle ? 1 : 0,
                    collisionNode ? 1 : 0,
                    hasBeenRendered ? 1 : 0,
                    distance2D2,
                    posOk ? pedPos.x : -9999.0f,
                    posOk ? pedPos.y : -9999.0f,
                    posOk ? pedPos.z : -9999.0f);
                if (written > 0) {
                    sampleSlotTextUsed += static_cast<size_t>(written);
                    ++sampleSlotCount;
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("population ped summary: reason=%s sample=%d pedPool=0x%08X scan exception used=%u readable=%u unreadable=%u",
            reason ? reason : "",
            sampleIndex,
            poolValue,
            used,
            readable,
            unreadable);
        return;
    }

    char topModels[256]{};
    AppendTopPedModels(topModels, sizeof(topModels), modelCounters, sizeof(modelCounters) / sizeof(modelCounters[0]));

    Log("population ped summary: reason=%s sample=%d pool=0x%08X objects=0x%08X size=%u slotSize=0x%X used=%u readable=%u unreadable=%u firstFree=%u type players=%u civMale=%u civFemale=%u cops=%u gangs=%u dealers=%u emergency=%u criminal=%u bum=%u prostitute=%u special=%u mission=%u other=%u created zero=%u game=%u mission=%u gameMission=%u other=%u alive=%u notAlive=%u topModels=%s",
        reason ? reason : "",
        sampleIndex,
        poolValue,
        objects,
        size,
        static_cast<unsigned>(kPedPoolSlotSize),
        used,
        readable,
        unreadable,
        firstFree,
        typePlayers,
        typeCivMale,
        typeCivFemale,
        typeCops,
        typeGangs,
        typeDealers,
        typeEmergency,
        typeCriminal,
        typeBum,
        typeProstitute,
        typeSpecial,
        typeMission,
        typeOther,
        createdZero,
        createdGame,
        createdMission,
        createdGameMission,
        createdOther,
        alive,
        notAlive,
        topModels);
    Log("population ped visibility: reason=%s sample=%d player=0x%08X playerPosOk=%d playerPos=%.1f,%.1f,%.1f playerMatrix=0x%08X playerMatrixPos=%d currArea=%u currAreaRaw=0x%08X playerArea=%u pos=%u matrix=%u rw=%u visible=%u removeWorld=%u dontRender=%u addPop=%u fade=%u inVeh=%u areaMatch=%u colNode=%u movingNode=%u nearAlive40=%u nearAlive80=%u nearAlive120=%u nearRenderable80=%u nearCiv80=%u nearGang80=%u nearMission80=%u",
        reason ? reason : "",
        sampleIndex,
        static_cast<unsigned>(playerPed),
        playerPosOk ? 1 : 0,
        playerPosOk ? playerPos.x : -9999.0f,
        playerPosOk ? playerPos.y : -9999.0f,
        playerPosOk ? playerPos.z : -9999.0f,
        static_cast<unsigned>(playerMatrix),
        playerFromMatrix ? 1 : 0,
        static_cast<unsigned>(currentArea),
        currentAreaRaw,
        static_cast<unsigned>(playerArea),
        positionReadable,
        matrixBacked,
        rwObjectPresent,
        visibleFlagSet,
        removeFromWorldSet,
        dontRenderSet,
        addToPopulationSet,
        fadeOutSet,
        inVehicleSet,
        areaMatch,
        collisionNodePresent,
        movingNodePresent,
        nearAlive40,
        nearAlive80,
        nearAlive120,
        nearRenderable80,
        nearCiv80,
        nearGang80,
        nearMission80);
    if (sampleSlots[0]) {
        Log("population ped slots: reason=%s sample=%d %s",
            reason ? reason : "",
            sampleIndex,
            sampleSlots);
    }
}

DWORD WINAPI PopulationPoolDiagnosticsThread(void*)
{
    if (!g_config.enablePopulationPoolDiagnostics || g_config.populationPoolDiagIterations <= 0) {
        return 0;
    }

    Sleep(static_cast<DWORD>(g_config.populationPoolDiagStartDelayMs));
    bool poolsReady = false;
    uint32_t pedPool = 0;
    uint32_t vehiclePool = 0;
    uint32_t objectPool = 0;
    uint32_t colModelPool = 0;
    for (int waitAttempt = 0; waitAttempt < 1800; ++waitAttempt) {
        if (AreCorePoolsReadyForDeferredReplay(&pedPool, &vehiclePool, &objectPool, &colModelPool)) {
            poolsReady = true;
            Log("population diag: core pools ready after waitAttempt=%d ped=0x%08X vehicle=0x%08X object=0x%08X colModel=0x%08X",
                waitAttempt,
                pedPool,
                vehiclePool,
                objectPool,
                colModelPool);
            break;
        }

        if (waitAttempt == 0 || waitAttempt == 30 || waitAttempt == 120 || (waitAttempt % 300) == 0) {
            Log("population diag: waiting for core pools waitAttempt=%d ped=0x%08X vehicle=0x%08X object=0x%08X colModel=0x%08X",
                waitAttempt,
                pedPool,
                vehiclePool,
                objectPool,
                colModelPool);
        }
        Sleep(1000);
    }

    if (!poolsReady) {
        Log("population diag: core pools still not ready; sampling null/invalid pointers for diagnostics");
    }

    for (int i = 0; i < g_config.populationPoolDiagIterations; ++i) {
        Log("population diag: sample=%d/%d", i + 1, g_config.populationPoolDiagIterations);
        LogPopulationPoolUsage("Peds", kOriginalPedPoolPtr);
        LogPopulationPoolUsage("Vehicles", kOriginalVehiclePoolPtr);
        LogPopulationPoolUsage("Objects", kOriginalObjectPoolPtr);
        LogPopulationRuntimeState("population-diag", i + 1);
        LogPopulationStreamingPedSlots("population-diag", i + 1);
        LogPedPoolModelTypeSummary("population-diag", i + 1);
        Sleep(static_cast<DWORD>(g_config.populationPoolDiagIntervalMs));
    }
    Log("population diag: completed iterations=%d", g_config.populationPoolDiagIterations);
    return 0;
}

void LogOneStreamingPedFunctionEntry(const char* reason, const char* label, uintptr_t address)
{
    uint8_t bytes[8]{};
    bool readable = false;
    if (IsReadableCommitted(address, sizeof(bytes))) {
        __try {
            std::memcpy(bytes, reinterpret_cast<const void*>(address), sizeof(bytes));
            readable = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            readable = false;
        }
    }

    if (!readable) {
        Log("ped zone function: reason=%s label=%s address=0x%08X unreadable",
            reason ? reason : "",
            label ? label : "",
            address);
        return;
    }

    uintptr_t target = 0;
    char targetModule[MAX_PATH]{};
    uintptr_t targetModuleBase = 0;
    if (bytes[0] == 0xE8 || bytes[0] == 0xE9) {
        target = DecodeRel32JumpTarget(address);
        if (target) {
            targetModuleBase = ModuleBaseFromAddress(target, targetModule, sizeof(targetModule));
        }
    }

    Log("ped zone function: reason=%s label=%s address=0x%08X bytes=%02X %02X %02X %02X %02X %02X %02X %02X first=%s target=0x%08X targetModule=%s+0x%X",
        reason ? reason : "",
        label ? label : "",
        address,
        bytes[0],
        bytes[1],
        bytes[2],
        bytes[3],
        bytes[4],
        bytes[5],
        bytes[6],
        bytes[7],
        bytes[0] == 0xC3 ? "ret" : (bytes[0] == 0xE9 ? "jmp" : (bytes[0] == 0xE8 ? "call" : "code")),
        target,
        target ? targetModule : "",
        target && targetModuleBase ? static_cast<unsigned>(target - targetModuleBase) : 0);
}

void LogStreamingPedFunctionEntryDiagnostics(const char* reason)
{
    LogOneStreamingPedFunctionEntry(reason, "CStreaming::StreamZoneModels", kCStreamingStreamZoneModels);
    LogOneStreamingPedFunctionEntry(reason, "CStreaming::StreamZoneModels_Gangs", kCStreamingStreamZoneModelsGangs);
    LogOneStreamingPedFunctionEntry(reason, "CStreaming::StreamVehiclesAndPeds_Always", kCStreamingStreamVehiclesAndPedsAlways);
    LogOneStreamingPedFunctionEntry(reason, "CStreaming::StreamVehiclesAndPeds", kCStreamingStreamVehiclesAndPeds);
    LogOneStreamingPedFunctionEntry(reason, "CStreaming::Update", kCStreamingUpdate);
    LogOneStreamingPedFunctionEntry(reason, "CStreaming::IsVeryBusy", kCStreamingIsVeryBusy);
}

void DumpModelContext(uint32_t modelId)
{
    Log("model-dump: modelId=%u oldModelInfoTable=0x%08X newModelInfoTable=0x%08X oldStreaming=0x%08X newStreaming=0x%08X",
        modelId, kOriginalCModelInfoPtrs, g_relocatedCModelInfoPtrs, kOriginalStreamingInfo, g_relocatedStreamingInfo);

    if (modelId >= (g_fileIdCapacity ? g_fileIdCapacity : kOriginalCModelInfoCount)) {
        Log("model-dump: modelId outside file ID capacity=%u", g_fileIdCapacity);
        return;
    }

    uint32_t oldMi = 0;
    uint32_t newMi = 0;
    if (modelId < kOriginalCModelInfoCount) {
        SafeReadU32(kOriginalCModelInfoPtrs + modelId * sizeof(uintptr_t), &oldMi);
    }
    const uintptr_t modelInfoEntry = SafeModelInfoEntryAddress(modelId);
    if (modelInfoEntry) {
        SafeReadU32(modelInfoEntry, &newMi);
    }
    Log("model-dump: CModelInfo[%u] old=0x%08X new=0x%08X", modelId, oldMi, newMi);

    if (oldMi) {
        LogDwords("old-model-info", oldMi, 16);
        uint32_t vtable = 0;
        uint32_t rwObject = 0;
        SafeReadU32(oldMi, &vtable);
        SafeReadU32(oldMi + 0x1C, &rwObject);
        Log("model-dump: old vtable=0x%08X rwObject(+0x1C)=0x%08X", vtable, rwObject);
        if (vtable) {
            LogDwords("old-model-vtable", vtable, 16);
        }
        if (rwObject) {
            LogDwords("old-model-rwobject", rwObject, 12);
        }
    }

    if (newMi && newMi != oldMi) {
        LogDwords("new-model-info", newMi, 16);
        uint32_t vtable = 0;
        uint32_t rwObject = 0;
        SafeReadU32(newMi, &vtable);
        SafeReadU32(newMi + 0x1C, &rwObject);
        Log("model-dump: new vtable=0x%08X rwObject(+0x1C)=0x%08X", vtable, rwObject);
        if (vtable) {
            LogDwords("new-model-vtable", vtable, 16);
        }
        if (rwObject) {
            LogDwords("new-model-rwobject", rwObject, 12);
        }
    }

    if (modelId < kOriginalStreamingInfoCount) {
        LogDwords("old-streaming-entry", kOriginalStreamingInfo + modelId * kStreamingInfoSize, 5);
    }
    const uintptr_t streamingEntry = SafeStreamingInfoEntryAddress(modelId);
    if (streamingEntry) {
        LogDwords("safe-streaming-entry", streamingEntry, 5);
    }
}

void LogModuleSnapshot()
{
    MODULEENTRY32 me{};
    me.dwSize = sizeof(me);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE) {
        Log("module snapshot failed: gle=%lu", GetLastError());
        return;
    }

    Log("module snapshot begin");
    if (Module32First(snapshot, &me)) {
        do {
            const char* name = me.szModule;
            const char* path = me.szExePath;
            const bool interesting =
                strstr(name, ".asi") || strstr(name, ".cleo") || strstr(name, "DINPUT8") ||
                strstr(name, "vorbis") || strstr(name, "fastman92") || strstr(name, "cleo") ||
                strstr(name, "modloader") || strstr(name, "SilentPatch") || strstr(name, "MixSets") ||
                strstr(name, "Urbanize") || ContainsCaseInsensitive(path, "modloader");

            if (interesting) {
                Log("module: base=0x%08X size=0x%08X name=%s path=%s",
                    reinterpret_cast<uintptr_t>(me.modBaseAddr), me.modBaseSize, me.szModule, me.szExePath);
                if (g_config.enableRiskConstantScan) {
                    ScanRiskyAddressConstants(me.szExePath);
                }
            }
        } while (Module32Next(snapshot, &me));
    }
    CloseHandle(snapshot);
    Log("module snapshot end");
}

void LogFLAExports()
{
    HMODULE fla = FindFlaModule();
    if (!fla) {
        Log("FLA module not loaded yet");
        return;
    }

    using GetProjectVersionFn = const char* (__cdecl*)();
    using GetNumberOfFileIDsFn = int32_t(__cdecl*)();
    using AreDifficultIDsExtendedFn = bool(__cdecl*)();

    auto getProjectVersion = reinterpret_cast<GetProjectVersionFn>(GetProcAddress(fla, "GetProjectVersion"));
    auto getNumberOfFileIDs = reinterpret_cast<GetNumberOfFileIDsFn>(GetProcAddress(fla, "GetNumberOfFileIDs"));
    auto areDifficultIDsExtended = reinterpret_cast<AreDifficultIDsExtendedFn>(GetProcAddress(fla, "AreDifficultIDsExtended"));

    Log("FLA module: base=0x%08X GetProjectVersion=%p GetNumberOfFileIDs=%p AreDifficultIDsExtended=%p",
        reinterpret_cast<uintptr_t>(fla), getProjectVersion, getNumberOfFileIDs, areDifficultIDsExtended);

    if (getProjectVersion) {
        Log("FLA export: ProjectVersion=%s", getProjectVersion());
    }
    if (getNumberOfFileIDs) {
        Log("FLA export: NumberOfFileIDs=%d", getNumberOfFileIDs());
    }
    if (areDifficultIDsExtended) {
        Log("FLA export: DifficultIDsExtended=%d", areDifficultIDsExtended() ? 1 : 0);
    }

    Log("FLA new ABI present: GetFLACompatibilityInfo=%p GetFLARuntimeAddress=%p",
        GetProcAddress(fla, "GetFLACompatibilityInfo"),
        GetProcAddress(fla, "GetFLARuntimeAddress"));
}

