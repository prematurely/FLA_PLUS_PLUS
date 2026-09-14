#include "FLACompatBridgeInternal.h"

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_FlaTrainInit_LoadPreserveEax()
{
    __asm
    {
        push eax
        mov ecx, dword ptr [g_flaTrainTypeCarriagesLoaderThis]
        call dword ptr [g_flaTrainTypeCarriagesLoadFunc]
        pop eax

        push ebx
        xor ebx, ebx
        cmp eax, ebx

        push 0x006F744B
        retn
    }
}
#endif

bool ReadLogAddress(const char* prefix, uintptr_t* out)
{
    if (!out) {
        return false;
    }

    HANDLE file = CreateFileA(g_flaLogPath, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        static DWORD lastError = 0;
        const DWORD error = GetLastError();
        if (lastError != error) {
            lastError = error;
            Log("FLA log read: open failed gle=%lu", error);
        }
        return false;
    }

    FILETIME logWriteTime{};
    FILETIME processCreateTime{};
    FILETIME processExitTime{};
    FILETIME processKernelTime{};
    FILETIME processUserTime{};
    if (GetFileTime(file, nullptr, nullptr, &logWriteTime) &&
        GetProcessTimes(GetCurrentProcess(), &processCreateTime, &processExitTime, &processKernelTime, &processUserTime)) {
        ULARGE_INTEGER logWrite{};
        ULARGE_INTEGER processCreate{};
        logWrite.LowPart = logWriteTime.dwLowDateTime;
        logWrite.HighPart = logWriteTime.dwHighDateTime;
        processCreate.LowPart = processCreateTime.dwLowDateTime;
        processCreate.HighPart = processCreateTime.dwHighDateTime;
        if (logWrite.QuadPart < processCreate.QuadPart) {
            static LONG staleLogCount = 0;
            if (InterlockedIncrement(&staleLogCount) == 1) {
                Log("FLA log read: stale file ignored path=%s", g_flaLogPath);
            }
            CloseHandle(file);
            return false;
        }
    }

    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > 2ll * 1024ll * 1024ll) {
        Log("FLA log read: invalid size=%lld gle=%lu", size.QuadPart, GetLastError());
        CloseHandle(file);
        return false;
    }

    const DWORD bytesToRead = static_cast<DWORD>(size.QuadPart);
    char* buffer = new char[bytesToRead + 1];
    DWORD bytesRead = 0;
    const bool readOk = ReadFile(file, buffer, bytesToRead, &bytesRead, nullptr) && bytesRead > 0;
    CloseHandle(file);
    if (!readOk) {
        Log("FLA log read: ReadFile failed gle=%lu", GetLastError());
        delete[] buffer;
        return false;
    }
    buffer[bytesRead] = '\0';

    bool found = false;
    char* search = buffer;
    while (char* match = std::strstr(search, prefix)) {
        char* p = std::strstr(match, "0x");
        if (p) {
            unsigned long value = std::strtoul(p + 2, nullptr, 16);
            if (value) {
                *out = static_cast<uintptr_t>(value);
                found = true;
            }
        }
        search = match + 1;
    }

    delete[] buffer;
    return found;
}

bool SyncShadowTable(const char* label, const char* logPrefix, uintptr_t originalAddress, size_t count, size_t elementSize)
{
    uintptr_t relocatedAddress = 0;
    if (!ReadLogAddress(logPrefix, &relocatedAddress)) {
        Log("legacy shadow: relocated %s address not found in FLA log", label);
        return false;
    }

    if (relocatedAddress == originalAddress) {
        Log("legacy shadow: %s not relocated", label);
        return true;
    }

    struct LastLog {
        const char* label;
        uintptr_t address;
    };
    static LastLog lastLogs[4]{};
    bool alreadyLogged = false;
    for (auto& entry : lastLogs) {
        if (entry.label == label) {
            if (entry.address == relocatedAddress) {
                alreadyLogged = true;
            } else {
                entry.address = relocatedAddress;
            }
            break;
        }
        if (!entry.label) {
            entry.label = label;
            entry.address = relocatedAddress;
            break;
        }
    }
    if (!alreadyLogged) {
        Log("legacy shadow: %s source from FLA log old=0x%08X new=0x%08X count=%u elem=0x%X",
            label, originalAddress, relocatedAddress, static_cast<unsigned>(count), static_cast<unsigned>(elementSize));
        LogMemoryRegion(label, relocatedAddress);
        LogMemoryRegion("legacy-shadow-destination", originalAddress);
    }

    const size_t copySize = count * elementSize;
    const bool ok = CopyMemoryWithProtect(originalAddress, relocatedAddress, copySize);
    return ok;
}

bool SyncLegacyModelInfoPointers()
{
    if (!g_config.enableLegacyModelInfoShadow) {
        return false;
    }

    const bool ok = SyncShadowTable("CModelInfo", "CModelInfo::ms_modelInfoPtrs:",
        kOriginalCModelInfoPtrs, kOriginalCModelInfoCount, sizeof(uintptr_t));
    ReadLogAddress("CModelInfo::ms_modelInfoPtrs:", &g_relocatedCModelInfoPtrs);
    return ok;
}

bool SyncLegacyStreamingInfo()
{
    if (!g_config.enableLegacyStreamingInfoShadow) {
        return false;
    }

    const bool ok = SyncShadowTable("CStreamingInfo", "CStreaming::ms_aInfoForModel:",
        kOriginalStreamingInfo, kOriginalStreamingInfoCount, kStreamingInfoSize);
    ReadLogAddress("CStreaming::ms_aInfoForModel:", &g_relocatedStreamingInfo);
    return ok;
}

DWORD WINAPI LegacyShadowThread(void*)
{
    if (!g_config.enableLegacyModelInfoShadow && !g_config.enableLegacyStreamingInfoShadow) {
        Log("legacy shadow: disabled by config");
        return 0;
    }

    Sleep(static_cast<DWORD>(g_config.legacyShadowStartDelayMs));
    for (int i = 0; i < g_config.legacyShadowIterations; ++i) {
        SyncLegacyModelInfoPointers();
        SyncLegacyStreamingInfo();
        Sleep(static_cast<DWORD>(g_config.legacyShadowIntervalMs));
    }
    return 0;
}

#if 0
bool ReadLogAddress_Old(const char* prefix, uintptr_t* out)
{
    if (!out) {
        return false;
    }

    FILE* file = nullptr;
    if (fopen_s(&file, g_flaLogPath, "r") != 0 || !file) {
        return false;
    }

    char line[512];
    bool found = false;
    while (std::fgets(line, sizeof(line), file)) {
        char* match = std::strstr(line, prefix);
        if (!match) {
            continue;
        }

        char* p = std::strstr(match, "0x");
        if (!p) {
            continue;
        }

        unsigned long value = std::strtoul(p + 2, nullptr, 16);
        if (value) {
            *out = static_cast<uintptr_t>(value);
            found = true;
        }
    }

    std::fclose(file);
    return found;
}
#endif

uint32_t ReadIniU32(const char* key, uint32_t defaultValue)
{
    char value[64]{};
    if (!ReadSmallTextValue(g_flaIniPath, key, value, sizeof(value))) {
        return defaultValue;
    }

    char* end = nullptr;
    const unsigned long parsed = std::strtoul(value, &end, 10);
    if (end == value) {
        return defaultValue;
    }
    return static_cast<uint32_t>(parsed);
}

uint32_t CalculateFileIdCapacityFromIni()
{
    const uint32_t dff = ReadIniU32("FILE_TYPE_DFF", 0);
    const uint32_t txd = ReadIniU32("FILE_TYPE_TXD", 0);
    const uint32_t col = ReadIniU32("FILE_TYPE_COL", 0);
    const uint32_t ipl = ReadIniU32("FILE_TYPE_IPL", 0);
    const uint32_t ifp = ReadIniU32("FILE_TYPE_IFP", 0);
    const uint32_t rrr = ReadIniU32("FILE_TYPE_RRR", 0);
    const uint32_t scm = ReadIniU32("FILE_TYPE_SCM", 0);
    const uint64_t sum = static_cast<uint64_t>(dff) + txd + col + ipl + ifp + rrr + scm;
    if (sum > 0x7FFFFFFFu) {
        return 0x7FFFFFFFu;
    }
    return static_cast<uint32_t>(sum);
}

HMODULE FindFlaModule()
{
    HMODULE module = GetModuleHandleA("$fastman92limitAdjuster.asi");
    if (module) {
        return module;
    }

    module = GetModuleHandleA("fastman92limitAdjuster.asi");
    if (module) {
        return module;
    }

    MODULEENTRY32 me{};
    me.dwSize = sizeof(me);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE) {
        return nullptr;
    }

    if (Module32First(snapshot, &me)) {
        do {
            if (strstr(me.szModule, "fastman92") || strstr(me.szExePath, "fastman92")) {
                module = me.hModule;
                break;
            }
        } while (Module32Next(snapshot, &me));
    }

    CloseHandle(snapshot);
    return module;
}

bool ResolveFlaExtendedIdApi()
{
    if (!g_config.enableFlaExtendedIdApi) {
        return false;
    }

    // FLA remains loaded for the lifetime of gta_sa.exe. Avoid resolving its
    // module again on hot paths such as entity bounds and streaming checks.
    if (g_flaExtendedIdApi.resolved && g_flaExtendedIdApi.module) {
        return g_flaExtendedIdApi.getExtendedIDFrom16BitBefore != nullptr;
    }

    HMODULE module = FindFlaModule();
    if (!module) {
        return false;
    }

    if (g_flaExtendedIdApi.resolved && g_flaExtendedIdApi.module == module) {
        return g_flaExtendedIdApi.getExtendedIDFrom16BitBefore != nullptr;
    }

    g_flaExtendedIdApi = {};
    g_flaExtendedIdApi.module = module;
    g_flaExtendedIdApi.areDifficultIDsExtended = reinterpret_cast<FlaAreDifficultIDsExtendedFn>(
        GetProcAddress(module, "AreDifficultIDsExtended"));
    g_flaExtendedIdApi.getNumberOfFileIDs = reinterpret_cast<FlaGetNumberOfFileIDsFn>(
        GetProcAddress(module, "GetNumberOfFileIDs"));
    g_flaExtendedIdApi.getExtendedIDFrom16BitBefore = reinterpret_cast<FlaGetExtendedIDFrom16BitBeforeFn>(
        GetProcAddress(module, "GetExtendedIDfrom16bitBefore"));
    g_flaExtendedIdApi.setExtendedIDFrom16BitBefore = reinterpret_cast<FlaSetExtendedIDFrom16BitBeforeFn>(
        GetProcAddress(module, "SetExtendedIDfrom16bitBefore"));
    g_flaExtendedIdApi.resolved = true;

    if (!g_flaExtendedIdApi.logged) {
        g_flaExtendedIdApi.logged = true;
        Log("FLA extended ID API: module=0x%08X difficult=%p count=%p get16Before=%p set16Before=%p",
            reinterpret_cast<uintptr_t>(module),
            g_flaExtendedIdApi.areDifficultIDsExtended,
            g_flaExtendedIdApi.getNumberOfFileIDs,
            g_flaExtendedIdApi.getExtendedIDFrom16BitBefore,
            g_flaExtendedIdApi.setExtendedIDFrom16BitBefore);
    }

    return g_flaExtendedIdApi.getExtendedIDFrom16BitBefore != nullptr;
}

bool IsFlaDifficultHighIdMode()
{
    if (!ResolveFlaExtendedIdApi()) {
        return g_fileIdCapacity > 65532;
    }

    __try {
        if (g_flaExtendedIdApi.areDifficultIDsExtended && g_flaExtendedIdApi.areDifficultIDsExtended()) {
            return true;
        }
        if (g_flaExtendedIdApi.getNumberOfFileIDs && g_flaExtendedIdApi.getNumberOfFileIDs() > 65532) {
            return true;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("FLA extended ID API: high-ID mode query exception");
    }

    return g_fileIdCapacity > 65532;
}

int32_t ReadExtendedIdFrom16BitField(const void* field)
{
    if (!field) {
        return -1;
    }

    if (ResolveFlaExtendedIdApi()) {
        __try {
            const int32_t id = g_flaExtendedIdApi.getExtendedIDFrom16BitBefore(field);
            if (id >= 0) {
                return id;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Log("FLA extended ID API: GetExtendedIDfrom16bitBefore exception field=0x%08X",
                reinterpret_cast<uintptr_t>(field));
        }
    }

    if (!IsReadableCommitted(reinterpret_cast<uintptr_t>(field), sizeof(uint16_t))) {
        return -1;
    }

    uint16_t id = 0xFFFF;
    __try {
        id = *reinterpret_cast<const uint16_t*>(field);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
    return static_cast<int32_t>(id);
}

bool RefreshFlaRuntimeStateFromAbi()
{
    HMODULE fla = FindFlaModule();
    if (!fla) {
        return false;
    }

    using GetFLACompatibilityInfoFn = bool(__cdecl*)(FLACompatibilityInfo*);
    using GetFLARuntimeAddressFn = uintptr_t(__cdecl*)(uint32_t);
    using GetNumberOfFileIDsFn = int32_t(__cdecl*)();

    auto getCompatibilityInfo = reinterpret_cast<GetFLACompatibilityInfoFn>(
        GetProcAddress(fla, "GetFLACompatibilityInfo"));
    auto getRuntimeAddress = reinterpret_cast<GetFLARuntimeAddressFn>(
        GetProcAddress(fla, "GetFLARuntimeAddress"));
    auto getNumberOfFileIDs = reinterpret_cast<GetNumberOfFileIDsFn>(
        GetProcAddress(fla, "GetNumberOfFileIDs"));

    bool usedAbi = false;

    if (getCompatibilityInfo) {
        FLACompatibilityInfo info{};
        info.dwSize = sizeof(info);
        if (getCompatibilityInfo(&info)) {
            g_flaAbiVersion = info.abiVersion;
            g_flaCompatFlags = info.flags;

            if (info.countOfAllFileIDs > 0) {
                g_fileIdCapacity = static_cast<uint32_t>(info.countOfAllFileIDs);
            }
            if (info.modelInfoPtrs) {
                g_relocatedCModelInfoPtrs = info.modelInfoPtrs;
            }
            if (info.streamingInfoForModel) {
                g_relocatedStreamingInfo = info.streamingInfoForModel;
            }
            if (info.streamingInfoExtension) {
                g_relocatedStreamingInfoExtension = info.streamingInfoExtension;
            }
            if (info.animBlocks) {
                g_relocatedAnimBlocks = info.animBlocks;
            }
            if (info.registeredKills) {
                g_relocatedRegisteredKills = info.registeredKills;
            }

            usedAbi = true;
            Log("FLA ABI: compatibility info abi=%u flags=0x%08X game=%d count=%d defaultCount=%d maxCount=%d modelInfo=0x%08X streaming=0x%08X streamingExt=0x%08X animBlocks=0x%08X registeredKills=0x%08X",
                info.abiVersion,
                info.flags,
                info.gameVersion,
                info.countOfAllFileIDs,
                info.defaultCountOfAllFileIDs,
                info.maxCountOfFileIDs,
                info.modelInfoPtrs,
                info.streamingInfoForModel,
                info.streamingInfoExtension,
                info.animBlocks,
                info.registeredKills);
        } else {
            Log("FLA ABI: GetFLACompatibilityInfo returned false");
        }
    }

    if (getRuntimeAddress) {
        const uintptr_t modelInfo = getRuntimeAddress(FLA_RUNTIME_ADDRESS_CMODELINFO_MODEL_INFO_PTRS);
        const uintptr_t streaming = getRuntimeAddress(FLA_RUNTIME_ADDRESS_CSTREAMING_INFO_FOR_MODEL);
        const uintptr_t streamingExtension = getRuntimeAddress(FLA_RUNTIME_ADDRESS_CSTREAMING_INFO_EXTENSION);
        const uintptr_t animBlocks = getRuntimeAddress(FLA_RUNTIME_ADDRESS_CANIMMANAGER_ANIM_BLOCKS);
        const uintptr_t registeredKills = getRuntimeAddress(FLA_RUNTIME_ADDRESS_CDARKEL_REGISTERED_KILLS);

        if (modelInfo) {
            g_relocatedCModelInfoPtrs = modelInfo;
        }
        if (streaming) {
            g_relocatedStreamingInfo = streaming;
        }
        if (streamingExtension) {
            g_relocatedStreamingInfoExtension = streamingExtension;
        }
        if (animBlocks) {
            g_relocatedAnimBlocks = animBlocks;
        }
        if (registeredKills) {
            g_relocatedRegisteredKills = registeredKills;
        }

        usedAbi = usedAbi || modelInfo || streaming || streamingExtension || animBlocks || registeredKills;
        Log("FLA ABI: runtime addresses modelInfo=0x%08X streaming=0x%08X streamingExt=0x%08X animBlocks=0x%08X registeredKills=0x%08X",
            modelInfo, streaming, streamingExtension, animBlocks, registeredKills);
    }

    if (!g_fileIdCapacity && getNumberOfFileIDs) {
        const int32_t count = getNumberOfFileIDs();
        if (count > 0) {
            g_fileIdCapacity = static_cast<uint32_t>(count);
            usedAbi = true;
            Log("FLA ABI: GetNumberOfFileIDs=%d", count);
        }
    }

    if (usedAbi) {
        g_runtimeStateSource = RUNTIME_SOURCE_FLA_ABI;
    }

    return usedAbi;
}

void RefreshFlaRuntimeState(bool logState)
{
    const bool usedAbi = RefreshFlaRuntimeStateFromAbi();

    bool usedLog = false;
    uintptr_t logAddress = 0;
    if (!g_relocatedCModelInfoPtrs &&
        ReadLogAddress("CModelInfo::ms_modelInfoPtrs:", &logAddress) &&
        IsReadableCommitted(logAddress, sizeof(uintptr_t))) {
        g_relocatedCModelInfoPtrs = logAddress;
        usedLog = true;
    }
    if (!g_relocatedStreamingInfo &&
        ReadLogAddress("CStreaming::ms_aInfoForModel:", &logAddress) &&
        IsReadableCommitted(logAddress, kStreamingInfoSize)) {
        g_relocatedStreamingInfo = logAddress;
        usedLog = true;
    }
    if (!g_relocatedAnimBlocks && ReadLogAddress("CAnimManager::ms_aAnimBlocks:", &logAddress)) {
        g_relocatedAnimBlocks = logAddress;
        usedLog = true;
    }
    if (!g_relocatedVehicleRecordingStreamingArray && ReadLogAddress("CVehicleRecording::StreamingArray:", &logAddress)) {
        g_relocatedVehicleRecordingStreamingArray = logAddress;
        usedLog = true;
    }
    if (!g_relocatedStreamedScripts && ReadLogAddress("CTheScripts::StreamedScripts:", &logAddress)) {
        g_relocatedStreamedScripts = logAddress;
        usedLog = true;
    }
    if (!g_relocatedHandlingManager && ReadLogAddress("&mod_HandlingManager =", &logAddress)) {
        g_relocatedHandlingManager = logAddress;
        usedLog = true;
    }

    if (!g_fileIdCapacity) {
        g_fileIdCapacity = CalculateFileIdCapacityFromIni();
        if (g_fileIdCapacity && !usedAbi && !usedLog) {
            g_runtimeStateSource = RUNTIME_SOURCE_INI_OR_VANILLA;
        }
    }
    if (!g_fileIdCapacity) {
        g_fileIdCapacity = static_cast<uint32_t>(kOriginalCModelInfoCount);
        if (!usedAbi && !usedLog) {
            g_runtimeStateSource = RUNTIME_SOURCE_INI_OR_VANILLA;
        }
    }
    if (!usedAbi && usedLog) {
        g_runtimeStateSource = RUNTIME_SOURCE_FLA_LOG;
    }

    g_pedPoolCapacity = ReadIniU32("Peds", 140);
    g_vehiclePoolCapacity = ReadIniU32("Vehicles", 110);
    g_objectPoolCapacity = ReadIniU32("Objects", 350);
    g_buildingPoolCapacity = ReadIniU32("Buildings", 13000);
    g_dummyPoolCapacity = ReadIniU32("Dummies", 2500);
    g_colModelPoolCapacity = ReadIniU32("ColModels", 10150);
    g_collisionStoreCapacity = ReadIniU32("Collision size", ReadIniU32("FILE_TYPE_COL", 500));

    if (logState) {
        Log("runtime state: source=%u abi=%u flags=0x%08X fileIdCapacity=%u CModelInfo=0x%08X CStreaming=0x%08X CStreamingExt=0x%08X AnimBlocks=0x%08X VehicleRecording=0x%08X StreamedScripts=0x%08X Handling=0x%08X RegisteredKills=0x%08X",
            g_runtimeStateSource,
            g_flaAbiVersion,
            g_flaCompatFlags,
            g_fileIdCapacity,
            g_relocatedCModelInfoPtrs,
            g_relocatedStreamingInfo,
            g_relocatedStreamingInfoExtension,
            g_relocatedAnimBlocks,
            g_relocatedVehicleRecordingStreamingArray,
            g_relocatedStreamedScripts,
            g_relocatedHandlingManager,
            g_relocatedRegisteredKills);
        Log("runtime state: poolCapacity peds=%u vehicles=%u objects=%u buildings=%u dummies=%u colModels=%u colStore=%u",
            g_pedPoolCapacity,
            g_vehiclePoolCapacity,
            g_objectPoolCapacity,
            g_buildingPoolCapacity,
            g_dummyPoolCapacity,
            g_colModelPoolCapacity,
            g_collisionStoreCapacity);
    }
}

bool TryRecoverCriticalFlaRuntimeAddresses()
{
    RefreshFlaRuntimeStateFromAbi();

    bool usedLog = false;
    uintptr_t logAddress = 0;
    if (!g_relocatedCModelInfoPtrs &&
        ReadLogAddress("CModelInfo::ms_modelInfoPtrs:", &logAddress) &&
        IsReadableCommitted(logAddress, sizeof(uintptr_t))) {
        g_relocatedCModelInfoPtrs = logAddress;
        usedLog = true;
    }
    if (!g_relocatedStreamingInfo &&
        ReadLogAddress("CStreaming::ms_aInfoForModel:", &logAddress) &&
        IsReadableCommitted(logAddress, kStreamingInfoSize)) {
        g_relocatedStreamingInfo = logAddress;
        usedLog = true;
    }
    if (usedLog && g_runtimeStateSource != RUNTIME_SOURCE_FLA_ABI) {
        g_runtimeStateSource = RUNTIME_SOURCE_FLA_LOG;
    }

    return g_relocatedCModelInfoPtrs && g_relocatedStreamingInfo;
}

DWORD WINAPI FlaRuntimeStateRecoveryThread(void*)
{
    for (int attempt = 0; attempt < 200; ++attempt) {
        if (TryRecoverCriticalFlaRuntimeAddresses()) {
            Log("FLA runtime recovery: resolved attempt=%d CModelInfo=0x%08X CStreaming=0x%08X log=%s ini=%s",
                attempt + 1,
                g_relocatedCModelInfoPtrs,
                g_relocatedStreamingInfo,
                g_flaLogPath,
                g_flaIniPath);
            RefreshFlaRuntimeState(true);
            if (g_config.enableProperShadersCompat) {
                ApplyProperShadersCompat();
            }
            return 0;
        }

        Sleep(attempt < 80 ? 25 : 100);
    }

    Log("FLA runtime recovery: unresolved after retries CModelInfo=0x%08X CStreaming=0x%08X log=%s",
        g_relocatedCModelInfoPtrs,
        g_relocatedStreamingInfo,
        g_flaLogPath);
    return 0;
}

void StartFlaRuntimeStateRecovery()
{
    if ((g_relocatedCModelInfoPtrs && g_relocatedStreamingInfo) ||
        InterlockedCompareExchange(&g_flaRuntimeRecoveryStarted, 1, 0) != 0) {
        return;
    }

    HANDLE thread = CreateThread(nullptr, 0, FlaRuntimeStateRecoveryThread, nullptr, 0, nullptr);
    if (thread) {
        CloseHandle(thread);
    } else {
        InterlockedExchange(&g_flaRuntimeRecoveryStarted, 0);
        Log("FLA runtime recovery: thread creation failed gle=%lu", GetLastError());
    }
}

uintptr_t SafeModelInfoEntryAddress(uint32_t modelId)
{
    const uint32_t capacity = g_fileIdCapacity ? g_fileIdCapacity : static_cast<uint32_t>(kOriginalCModelInfoCount);
    if (modelId >= capacity) {
        return 0;
    }

    const uintptr_t table = g_relocatedCModelInfoPtrs ? g_relocatedCModelInfoPtrs : kOriginalCModelInfoPtrs;
    const uintptr_t entry = table + static_cast<uintptr_t>(modelId) * sizeof(uintptr_t);
    return IsReadableCommitted(entry, sizeof(uintptr_t)) ? entry : 0;
}

uintptr_t SafeStreamingInfoEntryAddress(uint32_t modelId)
{
    const uint32_t capacity = g_fileIdCapacity ? g_fileIdCapacity : static_cast<uint32_t>(kOriginalStreamingInfoCount);
    if (modelId >= capacity) {
        return 0;
    }

    const uintptr_t table = g_relocatedStreamingInfo ? g_relocatedStreamingInfo : kOriginalStreamingInfo;
    const uintptr_t entry = table + static_cast<uintptr_t>(modelId) * kStreamingInfoSize;
    return IsReadableCommitted(entry, kStreamingInfoSize) ? entry : 0;
}

bool IsModelRwObjectLoaded(uint32_t modelId)
{
    const uintptr_t entry = SafeModelInfoEntryAddress(modelId);
    if (!entry) {
        return false;
    }

    uint32_t modelInfo = 0;
    SafeReadU32(entry, &modelInfo);
    if (!modelInfo && modelId < kOriginalCModelInfoCount) {
        SafeReadU32(kOriginalCModelInfoPtrs + modelId * sizeof(uintptr_t), &modelInfo);
    }
    if (!modelInfo) {
        return false;
    }

    uint32_t rwObject = 0;
    return SafeReadU32(modelInfo + 0x1C, &rwObject) && rwObject != 0;
}

uint8_t GetStreamingLoadState(uint32_t modelId)
{
    const uintptr_t entry = SafeStreamingInfoEntryAddress(modelId);
    if (!entry) {
        return 0xFF;
    }

    uint32_t state = 0xFF;
    SafeReadU32(entry + 0x10, &state);
    return static_cast<uint8_t>(state & 0xFF);
}

bool IsStreamingModelDefined(uint32_t modelId)
{
    const uintptr_t modelEntry = SafeModelInfoEntryAddress(modelId);
    const uintptr_t streamingEntry = SafeStreamingInfoEntryAddress(modelId);
    if (!modelEntry || !streamingEntry) {
        return false;
    }

    uint32_t modelInfo = 0;
    SafeReadU32(modelEntry, &modelInfo);
    if (!modelInfo && modelId < kOriginalCModelInfoCount) {
        SafeReadU32(kOriginalCModelInfoPtrs + modelId * sizeof(uintptr_t), &modelInfo);
    }

    uint32_t cdSize = 0;
    SafeReadU32(streamingEntry + 0x0C, &cdSize);
    return modelInfo != 0 && cdSize != 0;
}

bool IsPedModelInfo(uint32_t modelId)
{
    const uintptr_t entry = SafeModelInfoEntryAddress(modelId);
    if (!entry) {
        return false;
    }

    uint32_t modelInfo = 0;
    SafeReadU32(entry, &modelInfo);
    if (!modelInfo && modelId < kOriginalCModelInfoCount) {
        SafeReadU32(kOriginalCModelInfoPtrs + modelId * sizeof(uintptr_t), &modelInfo);
    }
    if (!modelInfo) {
        return false;
    }

    uint32_t vtable = 0;
    return SafeReadU32(modelInfo, &vtable) && vtable == kPedModelInfoVtable;
}

void InstallFlaTrainInitHookRepair()
{
#if defined(_M_IX86)
    constexpr uintptr_t patchAddress = 0x006F7446;
    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t bridgeTarget = reinterpret_cast<uintptr_t>(Bridge_FlaTrainInit_LoadPreserveEax);

    if (currentTarget == bridgeTarget) {
        Log("train hook repair: already installed at 0x%08X", patchAddress);
        return;
    }

    if (!currentTarget) {
        Log("train hook repair: no FLA train hook detected at 0x%08X", patchAddress);
        return;
    }

    uint8_t stub[16]{};
    if (!IsReadableCommitted(currentTarget, sizeof(stub))) {
        Log("train hook repair: FLA stub unreadable target=0x%08X", currentTarget);
        return;
    }

    __try {
        std::memcpy(stub, reinterpret_cast<const void*>(currentTarget), sizeof(stub));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("train hook repair: read exception target=0x%08X", currentTarget);
        return;
    }

    if (stub[0] != 0xB9 || stub[5] != 0xE8) {
        Log("train hook repair: unexpected FLA stub target=0x%08X bytes=%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
            currentTarget, stub[0], stub[1], stub[2], stub[3], stub[4],
            stub[5], stub[6], stub[7], stub[8], stub[9]);
        return;
    }

    uintptr_t loaderThis = 0;
    int32_t callRel = 0;
    std::memcpy(&loaderThis, stub + 1, sizeof(loaderThis));
    std::memcpy(&callRel, stub + 6, sizeof(callRel));
    const uintptr_t loadFunc = currentTarget + 10 + callRel;

    if (!IsExecutableCommitted(loadFunc)) {
        Log("train hook repair: loader function not executable this=0x%08X func=0x%08X target=0x%08X",
            loaderThis, loadFunc, currentTarget);
        return;
    }

    g_flaTrainTypeCarriagesLoaderThis = loaderThis;
    g_flaTrainTypeCarriagesLoadFunc = loadFunc;

    uint8_t patch[5]{};
    patch[0] = 0xE9;
    const int32_t bridgeRel = static_cast<int32_t>(bridgeTarget - (patchAddress + 5));
    std::memcpy(patch + 1, &bridgeRel, sizeof(bridgeRel));

    if (WriteBytesWithProtect(patchAddress, patch, sizeof(patch))) {
        Log("train hook repair: installed at 0x%08X oldTarget=0x%08X this=0x%08X loadFunc=0x%08X bridge=0x%08X",
            patchAddress, currentTarget, loaderThis, loadFunc, bridgeTarget);
    }
#else
    Log("train hook repair: unsupported architecture");
#endif
}

void RestoreFlaNoCollisionErrorPatch()
{
    static bool loggedOriginal = false;
    static bool loggedPatched = false;
    static const uint8_t originalBytes[] = {
        0x8B, 0x10,                   // mov edx, [eax]
        0x89, 0x54, 0x24, 0x10        // mov [esp+10h], edx
    };

    uint8_t current[sizeof(originalBytes)]{};
    if (!IsReadableCommitted(kFlaNoCollisionErrorPatch, sizeof(current))) {
        if (!loggedOriginal) {
            loggedOriginal = true;
            Log("compat patch: no-collision error 0x534134 address unreadable");
        }
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(kFlaNoCollisionErrorPatch), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        if (!loggedOriginal) {
            loggedOriginal = true;
            Log("compat patch: no-collision error 0x534134 read exception");
        }
        return;
    }

    if (std::memcmp(current, originalBytes, sizeof(originalBytes)) == 0) {
        if (!loggedOriginal) {
            loggedOriginal = true;
            Log("compat patch: no-collision error 0x534134 already original");
        }
        return;
    }

    if (current[0] != 0xE9) {
        if (!loggedPatched) {
            loggedPatched = true;
            Log("compat patch: no-collision error 0x534134 unexpected bytes old=%02X %02X %02X %02X %02X %02X",
                current[0], current[1], current[2], current[3], current[4], current[5]);
        }
        return;
    }

    if (WriteBytesWithProtect(kFlaNoCollisionErrorPatch, originalBytes, sizeof(originalBytes))) {
        Log("compat patch: restored FLA no-collision fatal hook at 0x%08X old=%02X %02X %02X %02X %02X %02X",
            kFlaNoCollisionErrorPatch,
            current[0], current[1], current[2], current[3], current[4], current[5]);
    }
}

void RestoreFlaObjectInitCollisionPatch()
{
    static bool loggedOriginal = false;
    static bool loggedPatched = false;
    static const uint8_t originalBytes[] = {
        0x0F, 0xB6, 0x47, 0x28, 0x50
    }; // movzx eax, byte ptr [edi+28h]; push eax

    uint8_t current[sizeof(originalBytes)]{};
    if (!IsReadableCommitted(kFlaObjectInitCollisionPatch, sizeof(current))) {
        if (!loggedOriginal) {
            loggedOriginal = true;
            Log("compat patch: CObject::Init 0x59F8BE address unreadable");
        }
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(kFlaObjectInitCollisionPatch), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        if (!loggedOriginal) {
            loggedOriginal = true;
            Log("compat patch: CObject::Init 0x59F8BE read exception");
        }
        return;
    }

    if (std::memcmp(current, originalBytes, sizeof(originalBytes)) == 0) {
        if (!loggedOriginal) {
            loggedOriginal = true;
            Log("compat patch: CObject::Init 0x59F8BE already original");
        }
        return;
    }

    if (WriteBytesWithProtect(kFlaObjectInitCollisionPatch, originalBytes, sizeof(originalBytes))) {
        if (!loggedPatched) {
            loggedPatched = true;
            Log("compat patch: restored FLA CObject::Init collision hook at 0x%08X old=%02X %02X %02X %02X %02X",
                kFlaObjectInitCollisionPatch,
                current[0], current[1], current[2], current[3], current[4]);
        }
    }
}

