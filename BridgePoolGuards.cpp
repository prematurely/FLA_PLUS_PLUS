#include "FLACompatBridgeInternal.h"

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_CleoPlus_ObjectAllocateBlocks_PoolGuard()
{
    __asm
    {
        mov eax, ds:[0x00B7449C]
        test eax, eax
        jz done
        push dword ptr [g_cleoPlusObjectAllocateBlocksContinue]
        retn

    done:
        ret
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_CleoPlus_VehicleAllocateBlocks_PoolGuard()
{
    __asm
    {
        mov eax, ds:[0x00B74494]
        test eax, eax
        jz done
        push dword ptr [g_cleoPlusVehicleAllocateBlocksContinue]
        retn

    done:
        ret
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_CleoPlus_PedAllocateBlocks_PoolGuard()
{
    __asm
    {
        mov eax, ds:[0x00B74490]
        test eax, eax
        jz done
        push dword ptr [g_cleoPlusPedAllocateBlocksContinue]
        retn

    done:
        ret
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_MixSets_PedAllocateBlocks_PoolGuard()
{
    __asm
    {
        mov eax, ds:[0x00B74490]
        test eax, eax
        jz done
        push dword ptr [g_mixSetsPedAllocateBlocksContinue]
        retn

    done:
        ret
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_CPoolsInitialise_ReplayHook()
{
    __asm
    {
        call dword ptr [g_cPoolsInitialiseReplayTrampoline]

        pushfd
        pushad
        call Bridge_PumpDeferredPoolAllocatesAfterCorePoolInit
        popad
        popfd
        ret
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void __stdcall Bridge_InvokePoolAllocateContinue(uintptr_t continueAddress, uintptr_t thisPtr, uintptr_t poolPtr)
{
    __asm
    {
        mov ecx, [esp + 8]
        mov eax, [esp + 12]
        call dword ptr [esp + 4]
        ret 12
    }
}
#endif

bool ReadPoolByteMap(uintptr_t pool, uintptr_t* objects, uintptr_t* byteMap, uint32_t* size, uint32_t* firstFree)
{
    uint32_t localObjects = 0;
    uint32_t localByteMap = 0;
    uint32_t localSize = 0;
    uint32_t localFirstFree = 0xFFFFFFFF;
    if (!ReadCPoolHeader(pool, &localSize, &localFirstFree) ||
        !SafeReadU32(pool + 0x00, &localObjects) ||
        !SafeReadU32(pool + 0x04, &localByteMap)) {
        return false;
    }

    if (objects) {
        *objects = localObjects;
    }
    if (byteMap) {
        *byteMap = localByteMap;
    }
    if (size) {
        *size = localSize;
    }
    if (firstFree) {
        *firstFree = localFirstFree;
    }
    return true;
}

DWORD WINAPI PreloadUrbanizePedModelsThread(void*)
{
#if defined(_M_IX86)
    using RequestModelFn = void(__cdecl*)(int, int);
    using LoadAllRequestedModelsFn = void(__cdecl*)(bool);

    constexpr uintptr_t kRequestModel = 0x004087E0;
    constexpr uintptr_t kLoadAllRequestedModels = 0x0040EA10;
    constexpr int kStreamingGameRequired = 0x2;
    constexpr int kStreamingKeepInMemory = 0x8;
    constexpr int kFlags = kStreamingGameRequired | kStreamingKeepInMemory;
    constexpr int kProblemPedModels[] = { 104, 105, 109, 110, 111 };

    auto requestModel = reinterpret_cast<RequestModelFn>(kRequestModel);
    auto loadAllRequestedModels = reinterpret_cast<LoadAllRequestedModelsFn>(kLoadAllRequestedModels);

    Log("ped preload: waiting for observed problem ped models flags=0x%X request=0x%08X loadAll=0x%08X",
        kFlags, kRequestModel, kLoadAllRequestedModels);

    Sleep(500);
    for (int attempt = 0; attempt < 60; ++attempt) {
        SyncLegacyModelInfoPointers();
        SyncLegacyStreamingInfo();

        int definedCount = 0;
        int missingRwCount = 0;
        for (int modelId : kProblemPedModels) {
            if (!IsPedModelInfo(modelId) || !IsStreamingModelDefined(modelId)) {
                continue;
            }
            ++definedCount;
            if (!IsModelRwObjectLoaded(modelId)) {
                ++missingRwCount;
            }
        }

        if (!definedCount) {
            if (attempt == 0 || attempt == 10 || attempt == 30) {
                Log("ped preload: no observed problem ped models defined yet attempt=%d", attempt);
            }
            Sleep(500);
            continue;
        }

        if (!missingRwCount) {
            Log("ped preload: all observed problem ped models already loaded attempt=%d defined=%d", attempt, definedCount);
            break;
        }

        __try {
            int requestCount = 0;
            for (int modelId : kProblemPedModels) {
                if (IsPedModelInfo(modelId) && IsStreamingModelDefined(modelId) && !IsModelRwObjectLoaded(modelId)) {
                    const uint8_t state = GetStreamingLoadState(modelId);
                    if (state == 0) {
                        Log("ped preload: request model=%d state=%u", modelId, static_cast<unsigned>(state));
                        requestModel(modelId, kFlags);
                        ++requestCount;
                    } else {
                        Log("ped preload: skip repeated request model=%d state=%u", modelId, static_cast<unsigned>(state));
                    }
                }
            }
            Log("ped preload: requested count=%d attempt=%d", requestCount, attempt);
            loadAllRequestedModels(false);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Log("ped preload: exception while requesting/loading observed problem ped models attempt=%d", attempt);
            Sleep(500);
            continue;
        }

        SyncLegacyModelInfoPointers();
        SyncLegacyStreamingInfo();
        bool allLoaded = true;
        int loadedCount = 0;
        int stillMissingCount = 0;
        for (int modelId : kProblemPedModels) {
            if (!IsPedModelInfo(modelId) || !IsStreamingModelDefined(modelId)) {
                continue;
            }
            if (IsModelRwObjectLoaded(modelId)) {
                ++loadedCount;
            } else {
                Log("ped preload: still missing model=%d loadState=%u", modelId, static_cast<unsigned>(GetStreamingLoadState(modelId)));
                ++stillMissingCount;
            }
        }
        allLoaded = stillMissingCount == 0;
        Log("ped preload: status attempt=%d defined=%d loaded=%d missing=%d",
            attempt, definedCount, loadedCount, stillMissingCount);

        if (allLoaded) {
            Log("ped preload: completed attempt=%d", attempt);
            break;
        }
        Sleep(500);
    }
#else
    Log("urbanize ped preload: unsupported architecture");
#endif
    return 0;
}

bool IsValidCPool(uintptr_t pool)
{
    if (!pool || !IsReadableCommitted(pool, 0x14)) {
        return false;
    }

    uint32_t size = 0;
    return SafeReadU32(pool + 0x08, &size) && size > 0 && size < 10000000;
}

bool ReadCPoolHeader(uintptr_t pool, uint32_t* size, uint32_t* firstFree)
{
    uint32_t localSize = 0;
    uint32_t localFirstFree = 0xFFFFFFFF;
    if (!IsValidCPool(pool) ||
        !SafeReadU32(pool + 0x08, &localSize) ||
        !SafeReadU32(pool + 0x0C, &localFirstFree)) {
        return false;
    }

    if (size) {
        *size = localSize;
    }
    if (firstFree) {
        *firstFree = localFirstFree;
    }
    return true;
}

bool IsPoolReadyForDeferredReplay(uintptr_t pool)
{
    uint32_t size = 0;
    uint32_t firstFree = 0xFFFFFFFF;
    if (!ReadCPoolHeader(pool, &size, &firstFree)) {
        return false;
    }

    // CPool starts with m_nFirstFree == -1 for an empty pool. A valid header is enough here:
    // replay is deferred until the core pool pointers exist and their object/bitmap storage is readable.
    return true;
}

bool ReadCorePoolPointers(uint32_t* pedPool, uint32_t* vehiclePool, uint32_t* objectPool, uint32_t* colModelPool)
{
    uint32_t ped = 0;
    uint32_t vehicle = 0;
    uint32_t object = 0;
    uint32_t colModel = 0;

    const bool ok =
        SafeReadU32(kOriginalPedPoolPtr, &ped) &&
        SafeReadU32(kOriginalVehiclePoolPtr, &vehicle) &&
        SafeReadU32(kOriginalObjectPoolPtr, &object) &&
        SafeReadU32(kOriginalColModelPoolPtr, &colModel);

    if (pedPool) {
        *pedPool = ped;
    }
    if (vehiclePool) {
        *vehiclePool = vehicle;
    }
    if (objectPool) {
        *objectPool = object;
    }
    if (colModelPool) {
        *colModelPool = colModel;
    }
    return ok;
}

bool AreCorePoolsReadyForDeferredReplay(uint32_t* pedOut, uint32_t* vehicleOut, uint32_t* objectOut, uint32_t* colModelOut)
{
    uint32_t ped = 0;
    uint32_t vehicle = 0;
    uint32_t object = 0;
    uint32_t colModel = 0;

    const bool readable = ReadCorePoolPointers(&ped, &vehicle, &object, &colModel);
    if (pedOut) {
        *pedOut = ped;
    }
    if (vehicleOut) {
        *vehicleOut = vehicle;
    }
    if (objectOut) {
        *objectOut = object;
    }
    if (colModelOut) {
        *colModelOut = colModel;
    }

    return readable &&
        IsPoolReadyForDeferredReplay(ped) &&
        IsPoolReadyForDeferredReplay(vehicle) &&
        IsPoolReadyForDeferredReplay(object) &&
        IsPoolReadyForDeferredReplay(colModel);
}

bool TryEnsureCPoolsInitialised(const char* reason, bool allowEarlyRecovery)
{
    if (!g_config.enableCPoolsInitialiseRecovery && !allowEarlyRecovery) {
        return false;
    }

    uint32_t pedBefore = 0;
    uint32_t vehicleBefore = 0;
    uint32_t objectBefore = 0;
    uint32_t colBefore = 0;
    if (!ReadCorePoolPointers(&pedBefore, &vehicleBefore, &objectBefore, &colBefore)) {
        Log("CPools recovery: cannot read pool pointer table reason=%s", reason ? reason : "<null>");
        return false;
    }

    if (IsValidCPool(pedBefore) &&
        IsValidCPool(vehicleBefore) &&
        IsValidCPool(objectBefore) &&
        IsValidCPool(colBefore)) {
        return true;
    }

    const bool anyCorePoolExists = pedBefore || vehicleBefore || objectBefore || colBefore;
    if (anyCorePoolExists) {
        const LONG logCount = InterlockedIncrement(&g_cPoolsInitialiseRecoveryLogs);
        if (logCount <= 16) {
            Log("CPools recovery: skipped partial pool state reason=%s ped=0x%08X vehicle=0x%08X object=0x%08X colModel=0x%08X",
                reason ? reason : "<null>",
                pedBefore,
                vehicleBefore,
                objectBefore,
                colBefore);
        }
        return false;
    }

    if (InterlockedCompareExchange(&g_cPoolsInitialiseRecoveryState, 1, 0) != 0) {
        uint32_t colAfterOtherAttempt = 0;
        SafeReadU32(kOriginalColModelPoolPtr, &colAfterOtherAttempt);
        return IsValidCPool(colAfterOtherAttempt);
    }

    const LONG logCount = InterlockedIncrement(&g_cPoolsInitialiseRecoveryLogs);
    if (logCount <= 16) {
        Log("CPools recovery: calling CPools::Initialise reason=%s early=%d entry=0x%08X executable=%d before ped=0x%08X vehicle=0x%08X object=0x%08X colModel=0x%08X",
            reason ? reason : "<null>",
            allowEarlyRecovery ? 1 : 0,
            kCPoolsInitialise,
            IsExecutableCommitted(kCPoolsInitialise) ? 1 : 0,
            pedBefore,
            vehicleBefore,
            objectBefore,
            colBefore);
        LogBytes("CPools-Initialise-entry", kCPoolsInitialise, 32);
    }

    bool called = false;
    if (IsExecutableCommitted(kCPoolsInitialise)) {
        using CPoolsInitialiseFn = void(__cdecl*)();
        __try {
            reinterpret_cast<CPoolsInitialiseFn>(kCPoolsInitialise)();
            called = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Log("CPools recovery: CPools::Initialise raised exception reason=%s code=0x%08X",
                reason ? reason : "<null>",
                GetExceptionCode());
        }
    }

    uint32_t pedAfter = 0;
    uint32_t vehicleAfter = 0;
    uint32_t objectAfter = 0;
    uint32_t colAfter = 0;
    ReadCorePoolPointers(&pedAfter, &vehicleAfter, &objectAfter, &colAfter);

    uint32_t colSize = 0;
    uint32_t colFirstFree = 0;
    if (IsValidCPool(colAfter)) {
        SafeReadU32(colAfter + 0x08, &colSize);
        SafeReadU32(colAfter + 0x0C, &colFirstFree);
    }

    const bool recovered =
        called &&
        IsValidCPool(pedAfter) &&
        IsValidCPool(vehicleAfter) &&
        IsValidCPool(objectAfter) &&
        IsValidCPool(colAfter);

    Log("CPools recovery: after call called=%d recovered=%d reason=%s ped=0x%08X vehicle=0x%08X object=0x%08X colModel=0x%08X colSize=%u colFirstFree=%u",
        called ? 1 : 0,
        recovered ? 1 : 0,
        reason ? reason : "<null>",
        pedAfter,
        vehicleAfter,
        objectAfter,
        colAfter,
        colSize,
        colFirstFree);

    InterlockedExchange(&g_cPoolsInitialiseRecoveryState, recovered ? 2 : -1);
    if (recovered) {
        Bridge_PumpDeferredPoolAllocatesAfterCorePoolInit();
    }
    return recovered;
}

LazyCPoolSpec* FindLazyCPoolSpec(uintptr_t poolPtr)
{
    for (auto& spec : g_lazyCPoolSpecs) {
        if (spec.poolPtr == poolPtr) {
            return &spec;
        }
    }
    return nullptr;
}

bool EnsureLazyCPoolReady(uintptr_t poolPtr, const char* reason)
{
    if (!g_config.enableLazyCPoolRegistry) {
        return false;
    }

    LazyCPoolSpec* spec = FindLazyCPoolSpec(poolPtr);
    if (!spec) {
        Log("lazy CPool registry: no spec for poolPtr=0x%08X reason=%s",
            poolPtr,
            reason ? reason : "<null>");
        return false;
    }

    uint32_t pool = 0;
    if (SafeReadU32(spec->poolPtr, &pool) && IsValidCPool(pool)) {
        return true;
    }

    LONG oldState = InterlockedCompareExchange(&spec->state, 1, 0);
    bool ownsCreateAttempt = oldState == 0;
    if (!ownsCreateAttempt && (oldState == -1 || oldState == 2)) {
        const LONG retryState = oldState;
        oldState = InterlockedCompareExchange(&spec->state, 1, retryState);
        ownsCreateAttempt = oldState == retryState;
    }

    if (!ownsCreateAttempt) {
        SafeReadU32(spec->poolPtr, &pool);
        return IsValidCPool(pool);
    }

    uint32_t capacity = ReadIniU32(spec->iniKey, spec->defaultCapacity);
    if (capacity == 0) {
        capacity = spec->defaultCapacity;
    }
    if (capacity > 10000000u) {
        capacity = 10000000u;
    }

    bool created = false;
    uintptr_t newPool = 0;

    __try {
        using OperatorNewFn = void* (__cdecl*)(uint32_t);
        using PoolCtorFn = void* (__thiscall*)(void*, int, const char*);
        auto operatorNewFn = reinterpret_cast<OperatorNewFn>(kGameOperatorNew);
        auto ctorFn = reinterpret_cast<PoolCtorFn>(spec->ctor);

        if (IsExecutableCommitted(kGameOperatorNew) && IsExecutableCommitted(spec->ctor)) {
            void* rawPool = operatorNewFn(0x14);
            if (rawPool) {
                newPool = reinterpret_cast<uintptr_t>(ctorFn(rawPool, static_cast<int>(capacity), reinterpret_cast<const char*>(spec->namePtr)));
                if (newPool && IsValidCPool(newPool)) {
                    uint32_t oldValue = 0;
                    SafeReadU32(spec->poolPtr, &oldValue);
                    if (!oldValue || !IsValidCPool(oldValue)) {
                        WriteBytesWithProtect(spec->poolPtr, reinterpret_cast<const uint8_t*>(&newPool), sizeof(newPool));
                    }
                    created = true;
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("lazy CPool registry: exception while creating %s reason=%s code=0x%08X",
            spec->label,
            reason ? reason : "<null>",
            GetExceptionCode());
    }

    uint32_t finalPool = 0;
    SafeReadU32(spec->poolPtr, &finalPool);
    uint32_t size = 0;
    uint32_t firstFree = 0;
    if (IsValidCPool(finalPool)) {
        SafeReadU32(finalPool + 0x08, &size);
        SafeReadU32(finalPool + 0x0C, &firstFree);
    }

    Log("lazy CPool registry: create result label=%s reason=%s created=%d poolPtr=0x%08X ptr=0x%08X new=0x%08X ctor=0x%08X name=0x%08X size=%u firstFree=%u capacity=%u iniKey=%s",
        spec->label,
        reason ? reason : "<null>",
        created ? 1 : 0,
        spec->poolPtr,
        finalPool,
        newPool,
        spec->ctor,
        spec->namePtr,
        size,
        firstFree,
        capacity,
        spec->iniKey);

    const bool ready = IsValidCPool(finalPool);
    InterlockedExchange(&spec->state, ready ? 2 : -1);
    if (ready) {
        Bridge_PumpDeferredPoolAllocatesAfterCorePoolInit();
    }

    return ready;
}

void PumpDeferredPoolAllocatesOnGameThread(uint32_t maxCount)
{
    if (!g_config.enableDeferredPoolAllocateReplay || maxCount == 0) {
        return;
    }

    const DWORD currentThreadId = GetCurrentThreadId();
    if (g_gameThreadId && currentThreadId != g_gameThreadId) {
        static LONG offThreadLogs = 0;
        if (InterlockedIncrement(&offThreadLogs) <= 4) {
            Log("auto pool guard: replay pump skipped non-game thread current=%lu game=%lu",
                currentThreadId, g_gameThreadId);
        }
        return;
    }

    if (InterlockedCompareExchange(&g_deferredPoolAllocateReplayActive, 1, 0) != 0) {
        return;
    }

    uint32_t pedPool = 0;
    uint32_t vehiclePool = 0;
    uint32_t objectPool = 0;
    uint32_t colModelPool = 0;
    if (!AreCorePoolsReadyForDeferredReplay(&pedPool, &vehiclePool, &objectPool, &colModelPool)) {
        InterlockedExchange(&g_deferredPoolAllocateReplayActive, 0);
        return;
    }

    constexpr uint32_t kMaxReplayPerPump = 8;
    if (maxCount > kMaxReplayPerPump) {
        maxCount = kMaxReplayPerPump;
    }

    DeferredPoolAllocate toRun[kMaxReplayPerPump]{};
    uint32_t runCount = 0;
    uint32_t droppedCount = 0;

    EnterCriticalSection(&g_deferredPoolAllocateLock);
    for (uint32_t i = 0; i < g_deferredPoolAllocateCount && runCount < maxCount; ++i) {
        DeferredPoolAllocate& item = g_deferredPoolAllocates[i];
        if (item.completed) {
            continue;
        }

        uintptr_t pool = 0;
        if (!SafeReadU32(item.poolPtrAddress, reinterpret_cast<uint32_t*>(&pool)) || !IsValidCPool(pool)) {
            continue;
        }

        char currentModule[MAX_PATH]{};
        const uintptr_t moduleBase = ModuleBaseFromAddress(item.continueAddress, currentModule, sizeof(currentModule));
        const bool continuationValid = moduleBase != 0 &&
            IsExecutableCommitted(item.continueAddress) &&
            item.moduleName[0] != '\0' &&
            _stricmp(currentModule, item.moduleName) == 0;
        const bool thisValid = item.thisPtr >= 0x10000 && IsWritableCommitted(item.thisPtr, sizeof(uintptr_t));
        if (!continuationValid || !thisValid) {
            item.completed = 1;
            ++droppedCount;
            Log("auto pool guard: dropped stale deferred allocate module=%s currentModule=%s continue=0x%08X this=0x%08X continuationValid=%d thisValid=%d",
                item.moduleName,
                currentModule,
                item.continueAddress,
                item.thisPtr,
                continuationValid ? 1 : 0,
                thisValid ? 1 : 0);
            continue;
        }

        item.completed = 1;
        toRun[runCount++] = item;
    }
    LeaveCriticalSection(&g_deferredPoolAllocateLock);

    for (uint32_t i = 0; i < runCount; ++i) {
        uintptr_t pool = 0;
        if (!SafeReadU32(toRun[i].poolPtrAddress, reinterpret_cast<uint32_t*>(&pool)) || !IsValidCPool(pool)) {
            continue;
        }

        __try {
            Bridge_InvokePoolAllocateContinue(toRun[i].continueAddress, toRun[i].thisPtr, pool);
            Log("auto pool guard: game-thread replay module=%s pool=0x%08X continue=0x%08X this=0x%08X",
                toRun[i].moduleName,
                pool,
                toRun[i].continueAddress,
                toRun[i].thisPtr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Log("auto pool guard: game-thread replay exception module=%s pool=0x%08X continue=0x%08X this=0x%08X code=0x%08X",
                toRun[i].moduleName,
                pool,
                toRun[i].continueAddress,
                toRun[i].thisPtr,
                GetExceptionCode());
        }
    }

    if (runCount || droppedCount) {
        EnterCriticalSection(&g_deferredPoolAllocateLock);
        uint32_t writeIndex = 0;
        for (uint32_t readIndex = 0; readIndex < g_deferredPoolAllocateCount; ++readIndex) {
            if (!g_deferredPoolAllocates[readIndex].completed) {
                if (writeIndex != readIndex) {
                    g_deferredPoolAllocates[writeIndex] = g_deferredPoolAllocates[readIndex];
                }
                ++writeIndex;
            }
        }
        for (uint32_t i = writeIndex; i < g_deferredPoolAllocateCount; ++i) {
            g_deferredPoolAllocates[i] = DeferredPoolAllocate{};
        }
        g_deferredPoolAllocateCount = writeIndex;
        LeaveCriticalSection(&g_deferredPoolAllocateLock);
    }

    InterlockedExchange(&g_deferredPoolAllocateReplayActive, 0);
}

extern "C" void __cdecl Bridge_PumpDeferredPoolAllocatesAfterCorePoolInit()
{
    PumpDeferredPoolAllocatesOnGameThread(8);
    PumpDeferredPoolAllocatesOnGameThread(8);
}

bool EnsureLazyCoreCPoolsReady(const char* reason)
{
    const uintptr_t corePools[] = {
        kPtrNodeSinglePoolPtr,
        kPtrNodeDoublePoolPtr,
        kEntryInfoNodePoolPtr,
        kOriginalPedPoolPtr,
        kOriginalVehiclePoolPtr,
        kOriginalBuildingPoolPtr,
        kOriginalObjectPoolPtr,
        kOriginalDummyPoolPtr,
        kOriginalColModelPoolPtr,
        kTasksPoolPtr,
        kEventsPoolPtr,
        kPointRoutePoolPtr,
        kPatrolRoutePoolPtr,
        kNodeRoutePoolPtr,
        kTaskAllocatorPoolPtr,
        kPedIntelligencePoolPtr,
        kPedAttractorsPoolPtr,
    };

    bool allReady = true;
    for (uintptr_t poolPtr : corePools) {
        if (!EnsureLazyCPoolReady(poolPtr, reason)) {
            allReady = false;
        }
    }

    uint32_t ped = 0;
    uint32_t vehicle = 0;
    uint32_t building = 0;
    uint32_t object = 0;
    uint32_t dummy = 0;
    uint32_t colModel = 0;
    uint32_t tasks = 0;
    uint32_t events = 0;
    ReadCorePoolPointers(&ped, &vehicle, &object, &colModel);
    SafeReadU32(kOriginalBuildingPoolPtr, &building);
    SafeReadU32(kOriginalDummyPoolPtr, &dummy);
    SafeReadU32(kTasksPoolPtr, &tasks);
    SafeReadU32(kEventsPoolPtr, &events);
    Log("lazy CPool registry: core ensure result reason=%s allReady=%d ped=0x%08X vehicle=0x%08X building=0x%08X object=0x%08X dummy=0x%08X colModel=0x%08X tasks=0x%08X events=0x%08X",
        reason ? reason : "<null>",
        allReady ? 1 : 0,
        ped,
        vehicle,
        building,
        object,
        dummy,
        colModel,
        tasks,
        events);
    return allReady;
}

bool EnsureBatchLazyCPoolsInitialised(const char* reason, bool forceRetry)
{
    if (!g_config.enableLazyCPoolRegistry || !g_config.enableBatchLazyCPoolInitialise) {
        return false;
    }

    const LONG state = InterlockedCompareExchange(&g_batchLazyCPoolInitialiseState, 0, 0);
    if (state == 2 && !forceRetry) {
        return true;
    }
    if (state == 1) {
        Sleep(0);
        return InterlockedCompareExchange(&g_batchLazyCPoolInitialiseState, 0, 0) == 2;
    }
    if (state == -1 && !forceRetry) {
        forceRetry = true;
    }

    const LONG expected = (forceRetry && state == 2) ? 2 : ((state == -1) ? -1 : 0);
    if (InterlockedCompareExchange(&g_batchLazyCPoolInitialiseState, 1, expected) != expected) {
        return InterlockedCompareExchange(&g_batchLazyCPoolInitialiseState, 0, 0) == 2;
    }

    const LONG logCount = InterlockedIncrement(&g_batchLazyCPoolInitialiseLogs);
    if (logCount <= 32) {
        Log("lazy CPool registry: batch initialise begin reason=%s retry=%d oldState=%ld",
            reason ? reason : "<null>",
            forceRetry ? 1 : 0,
            state);
    }

    InterlockedIncrement(&g_lazyCPoolBatchDepth);
    const bool allReady = EnsureLazyCoreCPoolsReady(reason ? reason : "batch lazy CPools initialise");
    InterlockedDecrement(&g_lazyCPoolBatchDepth);

    uint32_t ptrSingle = 0;
    uint32_t ptrDouble = 0;
    uint32_t entryInfo = 0;
    uint32_t ped = 0;
    uint32_t vehicle = 0;
    uint32_t building = 0;
    uint32_t object = 0;
    uint32_t dummy = 0;
    uint32_t colModel = 0;
    uint32_t tasks = 0;
    uint32_t events = 0;
    uint32_t pointRoute = 0;
    uint32_t patrolRoute = 0;
    uint32_t nodeRoute = 0;
    uint32_t taskAllocator = 0;
    uint32_t pedIntelligence = 0;
    uint32_t pedAttractors = 0;
    SafeReadU32(kPtrNodeSinglePoolPtr, &ptrSingle);
    SafeReadU32(kPtrNodeDoublePoolPtr, &ptrDouble);
    SafeReadU32(kEntryInfoNodePoolPtr, &entryInfo);
    ReadCorePoolPointers(&ped, &vehicle, &object, &colModel);
    SafeReadU32(kOriginalBuildingPoolPtr, &building);
    SafeReadU32(kOriginalDummyPoolPtr, &dummy);
    SafeReadU32(kTasksPoolPtr, &tasks);
    SafeReadU32(kEventsPoolPtr, &events);
    SafeReadU32(kPointRoutePoolPtr, &pointRoute);
    SafeReadU32(kPatrolRoutePoolPtr, &patrolRoute);
    SafeReadU32(kNodeRoutePoolPtr, &nodeRoute);
    SafeReadU32(kTaskAllocatorPoolPtr, &taskAllocator);
    SafeReadU32(kPedIntelligencePoolPtr, &pedIntelligence);
    SafeReadU32(kPedAttractorsPoolPtr, &pedAttractors);

    Log("lazy CPool registry: batch initialise end reason=%s allReady=%d ptrSingle=0x%08X ptrDouble=0x%08X entryInfo=0x%08X ped=0x%08X vehicle=0x%08X building=0x%08X object=0x%08X dummy=0x%08X colModel=0x%08X tasks=0x%08X events=0x%08X pointRoute=0x%08X patrolRoute=0x%08X nodeRoute=0x%08X taskAllocator=0x%08X pedIntelligence=0x%08X pedAttractors=0x%08X",
        reason ? reason : "<null>",
        allReady ? 1 : 0,
        ptrSingle,
        ptrDouble,
        entryInfo,
        ped,
        vehicle,
        building,
        object,
        dummy,
        colModel,
        tasks,
        events,
        pointRoute,
        patrolRoute,
        nodeRoute,
        taskAllocator,
        pedIntelligence,
        pedAttractors);

    InterlockedExchange(&g_batchLazyCPoolInitialiseState, allReady ? 2 : -1);
    return allReady;
}

void InstallCPoolsInitialiseReplayHook()
{
#if defined(_M_IX86)
    if (!g_config.enableDeferredPoolAllocateReplay) {
        return;
    }

    constexpr size_t stolenBytes = 7;
    static const uint8_t expected[stolenBytes] = {
        0x6A, 0xFF,                   // push -1
        0x68, 0x0B, 0xCC, 0x83, 0x00 // push 0x0083CC0B
    };

    const uintptr_t hookTarget = reinterpret_cast<uintptr_t>(Bridge_CPoolsInitialise_ReplayHook);
    if (DecodeRel32JumpTarget(kCPoolsInitialise) == hookTarget) {
        Log("CPools replay hook: already installed entry=0x%08X target=0x%08X trampoline=0x%08X",
            kCPoolsInitialise,
            hookTarget,
            g_cPoolsInitialiseReplayTrampoline);
        return;
    }

    uint8_t current[stolenBytes]{};
    if (!IsReadableCommitted(kCPoolsInitialise, sizeof(current))) {
        Log("CPools replay hook: entry unreadable address=0x%08X", kCPoolsInitialise);
        return;
    }
    __try {
        std::memcpy(current, reinterpret_cast<const void*>(kCPoolsInitialise), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("CPools replay hook: entry read fault address=0x%08X", kCPoolsInitialise);
        return;
    }

    if (std::memcmp(current, expected, sizeof(expected)) != 0) {
        Log("CPools replay hook: unexpected entry bytes address=0x%08X old=%02X %02X %02X %02X %02X %02X %02X",
            kCPoolsInitialise,
            current[0], current[1], current[2], current[3], current[4], current[5], current[6]);
        return;
    }

    const uintptr_t trampoline = CreateRel32Trampoline(kCPoolsInitialise, stolenBytes);
    if (!trampoline) {
        Log("CPools replay hook: trampoline creation failed address=0x%08X", kCPoolsInitialise);
        return;
    }

    uint8_t patch[stolenBytes]{ 0xE9, 0, 0, 0, 0, 0x90, 0x90 };
    const int64_t diff = static_cast<int64_t>(hookTarget) - static_cast<int64_t>(kCPoolsInitialise + 5);
    if (diff < INT32_MIN || diff > INT32_MAX) {
        VirtualFree(reinterpret_cast<void*>(trampoline), 0, MEM_RELEASE);
        Log("CPools replay hook: target out of rel32 range entry=0x%08X target=0x%08X",
            kCPoolsInitialise,
            hookTarget);
        return;
    }
    const int32_t rel = static_cast<int32_t>(diff);
    std::memcpy(patch + 1, &rel, sizeof(rel));

    g_cPoolsInitialiseReplayTrampoline = trampoline;
    if (WriteBytesWithProtect(kCPoolsInitialise, patch, sizeof(patch))) {
        Log("CPools replay hook: installed entry=0x%08X target=0x%08X trampoline=0x%08X stolen=%u",
            kCPoolsInitialise,
            hookTarget,
            trampoline,
            static_cast<unsigned>(stolenBytes));
    } else {
        g_cPoolsInitialiseReplayTrampoline = 0;
        VirtualFree(reinterpret_cast<void*>(trampoline), 0, MEM_RELEASE);
        Log("CPools replay hook: patch write failed entry=0x%08X", kCPoolsInitialise);
    }
#endif
}

extern "C" void __stdcall Bridge_DeferPoolAllocate(uintptr_t poolPtrAddress, uintptr_t continueAddress, uintptr_t thisPtr)
{
    if (!g_config.enableDeferredPoolAllocateReplay || !poolPtrAddress || !continueAddress || !thisPtr) {
        return;
    }

    char moduleName[MAX_PATH]{};
    ModuleBaseFromAddress(continueAddress, moduleName, sizeof(moduleName));

    EnterCriticalSection(&g_deferredPoolAllocateLock);
    for (uint32_t i = 0; i < g_deferredPoolAllocateCount; ++i) {
        const DeferredPoolAllocate& item = g_deferredPoolAllocates[i];
        if (item.poolPtrAddress == poolPtrAddress &&
            item.continueAddress == continueAddress &&
            item.thisPtr == thisPtr) {
            LeaveCriticalSection(&g_deferredPoolAllocateLock);
            return;
        }
    }

    if (g_deferredPoolAllocateCount < kMaxDeferredPoolAllocates) {
        DeferredPoolAllocate& item = g_deferredPoolAllocates[g_deferredPoolAllocateCount++];
        item.poolPtrAddress = poolPtrAddress;
        item.continueAddress = continueAddress;
        item.thisPtr = thisPtr;
        item.completed = 0;
        strncpy_s(item.moduleName, moduleName[0] ? moduleName : "<unknown>", _TRUNCATE);

        const LONG logCount = InterlockedIncrement(&g_deferredPoolAllocateLogs);
        if (logCount <= 64) {
            Log("auto pool guard: deferred allocate module=%s poolPtr=0x%08X continue=0x%08X this=0x%08X queued=%u",
                item.moduleName,
                item.poolPtrAddress,
                item.continueAddress,
                item.thisPtr,
                g_deferredPoolAllocateCount);
        }
    } else {
        static LONG fullLogs = 0;
        if (InterlockedIncrement(&fullLogs) <= 8) {
            Log("auto pool guard: deferred queue full poolPtr=0x%08X continue=0x%08X this=0x%08X",
                poolPtrAddress,
                continueAddress,
                thisPtr);
        }
    }
    LeaveCriticalSection(&g_deferredPoolAllocateLock);
}

uintptr_t FindAllocateBlocksByPattern(HMODULE module, const char* typeName, uintptr_t expectedPoolPtr)
{
#if defined(_M_IX86)
    if (!module) {
        return 0;
    }

    // Pattern: mov eax, [expectedPoolPtr]      (A1 xx xx xx xx)
    // followed by mov reg32, 4                 (B8/B9/BA/BB/BC/BD/BE/BF 04 00 00 00)
    // then push esi                            (56)
    // then mov esi, ecx                        (8B F1)
    // This is the prologue of plugin-sdk's ExtendedData::AllocateBlocks
    // as emitted by MSVC for the current CLEO+/MixSets/Urbanize builds.
    uint8_t pattern[12];
    pattern[0] = 0xA1;
    std::memcpy(pattern + 1, &expectedPoolPtr, sizeof(expectedPoolPtr));
    pattern[5] = 0xBA; // mov edx, 4 (any B8+reg opcode wildcarded below)
    pattern[6] = 0x04;
    pattern[7] = 0x00;
    pattern[8] = 0x00;
    pattern[9] = 0x00;
    pattern[10] = 0x56;
    pattern[11] = 0x8B;
    const char mask[] = "xxxxx?xxxxxx";

    // Scan for all matches in .text
    uintptr_t base = reinterpret_cast<uintptr_t>(module);
    MODULEINFO mi{};
    if (!GetModuleInformation(GetCurrentProcess(), module, &mi, sizeof(mi))) {
        return 0;
    }

    PIMAGE_DOS_HEADER dos = reinterpret_cast<PIMAGE_DOS_HEADER>(base);
    PIMAGE_NT_HEADERS nt = reinterpret_cast<PIMAGE_NT_HEADERS>(base + dos->e_lfanew);
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt);
    uintptr_t textBase = 0;
    size_t textSize = 0;
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section) {
        if (memcmp(section->Name, ".text\0\0\0", 8) == 0) {
            textBase = base + section->VirtualAddress;
            textSize = section->Misc.VirtualSize;
            break;
        }
    }
    if (!textBase || !textSize || !IsReadableCommitted(textBase, textSize)) {
        return 0;
    }

    uintptr_t firstMatch = 0;
    uintptr_t secondMatch = 0;
    const uint8_t* scan = reinterpret_cast<const uint8_t*>(textBase);
    const size_t patLen = sizeof(pattern);
    const size_t maskLen = sizeof(mask) - 1;

    for (size_t j = 0; j + patLen <= textSize && maskLen == patLen; ++j) {
        bool match = true;
        for (size_t k = 0; k < patLen; ++k) {
            if (mask[k] == 'x' && scan[j + k] != pattern[k]) {
                match = false;
                break;
            }
        }
        if (!match) {
            continue;
        }
        // Extra validation: byte 5 must be a MOV reg32, imm32 opcode (B8-BF).
        // This rejects any accidental match where byte 5 happens to be wildcarded.
        uint8_t movOpcode = scan[j + 5];
        if (movOpcode < 0xB8 || movOpcode > 0xBF) {
            continue;
        }

        if (!firstMatch) {
            firstMatch = textBase + j;
        } else if (!secondMatch) {
            secondMatch = textBase + j;
            break; // More than one plausible match; too ambiguous.
        }
    }

    if (firstMatch && !secondMatch) {
        Log("pool guard pattern scan: %s single match at 0x%08X", typeName ? typeName : "", firstMatch);
        return firstMatch;
    }

    if (firstMatch && secondMatch) {
        Log("pool guard pattern scan: %s ambiguous (%d matches), falling back to version hash",
            typeName ? typeName : "", 2);
    } else {
        Log("pool guard pattern scan: %s no match, falling back to version hash", typeName ? typeName : "");
    }

    // Fallback: version hash database
    uint32_t textHash = CalculateModuleTextHash(module);
    Log("pool guard version hash: %s textHash=0x%08X", typeName ? typeName : "", textHash);

    for (size_t i = 0; i < kKnownCleoPlusVersionCount; ++i) {
        if (kKnownCleoPlusVersions[i].textHash == textHash) {
            int32_t offset = 0;
            if (expectedPoolPtr == kOriginalObjectPoolPtr) {
                offset = kKnownCleoPlusVersions[i].objectOffset;
            } else if (expectedPoolPtr == kOriginalVehiclePoolPtr) {
                offset = kKnownCleoPlusVersions[i].vehicleOffset;
            } else if (expectedPoolPtr == kOriginalPedPoolPtr) {
                offset = kKnownCleoPlusVersions[i].pedOffset;
            }
            if (offset != 0) {
                uintptr_t addr = base + offset;
                Log("pool guard version hash: %s matched entry %zu offset=0x%X addr=0x%08X",
                    typeName ? typeName : "", i, offset, addr);
                return addr;
            }
        }
    }

    Log("pool guard version hash: %s no known entry for hash 0x%08X", typeName ? typeName : "", textHash);
#endif
    return 0;
}

uintptr_t CreatePoolAllocateGuardStub(uintptr_t poolPtrAddress, uintptr_t continueAddress)
{
#if defined(_M_IX86)
    uint8_t* stub = reinterpret_cast<uint8_t*>(VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!stub) {
        return 0;
    }

    size_t o = 0;
    stub[o++] = 0xA1; // mov eax, [poolPtrAddress]
    std::memcpy(stub + o, &poolPtrAddress, sizeof(poolPtrAddress));
    o += sizeof(poolPtrAddress);
    stub[o++] = 0x85; stub[o++] = 0xC0; // test eax, eax
    stub[o++] = 0x74; stub[o++] = 0x06; // jz deferred
    stub[o++] = 0x68; // push continueAddress
    std::memcpy(stub + o, &continueAddress, sizeof(continueAddress));
    o += sizeof(continueAddress);
    stub[o++] = 0xC3; // ret

    stub[o++] = 0x51; // push ecx
    stub[o++] = 0x68; // push continueAddress
    std::memcpy(stub + o, &continueAddress, sizeof(continueAddress));
    o += sizeof(continueAddress);
    stub[o++] = 0x68; // push poolPtrAddress
    std::memcpy(stub + o, &poolPtrAddress, sizeof(poolPtrAddress));
    o += sizeof(poolPtrAddress);
    stub[o++] = 0xE8; // call Bridge_DeferPoolAllocate
    const uintptr_t callSite = reinterpret_cast<uintptr_t>(stub + o - 1);
    const uintptr_t callNext = callSite + 5;
    const intptr_t rel = reinterpret_cast<uintptr_t>(Bridge_DeferPoolAllocate) - callNext;
    if (rel < INT32_MIN || rel > INT32_MAX) {
        VirtualFree(stub, 0, MEM_RELEASE);
        return 0;
    }
    const int32_t rel32 = static_cast<int32_t>(rel);
    std::memcpy(stub + o, &rel32, sizeof(rel32));
    o += sizeof(rel32);
    stub[o++] = 0xC3; // ret

    return reinterpret_cast<uintptr_t>(stub);
#else
    (void)poolPtrAddress;
    (void)continueAddress;
    return 0;
#endif
}

bool InstallOneCleoPlusPoolAllocateGuard(
    const char* name,
    uintptr_t patchAddress,
    uintptr_t expectedPoolPtr,
    uintptr_t guardTarget,
    uintptr_t* continueOut)
{
#if defined(_M_IX86)
    if (!continueOut || !IsReadableCommitted(patchAddress, 5)) {
        return false;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    if (guardTarget && currentTarget == guardTarget) {
        Log("CLEO+ pool allocate guard: %s already installed at 0x%08X",
            name ? name : "",
            patchAddress);
        return true;
    }

    uint8_t current[5]{};
    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    uintptr_t poolPtr = 0;
    if (current[0] == 0xA1) {
        std::memcpy(&poolPtr, current + 1, sizeof(poolPtr));
    }
    if (current[0] != 0xA1 || poolPtr != expectedPoolPtr) {
        Log("CLEO+ pool allocate guard: %s unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X expectedPool=0x%08X decodedPool=0x%08X",
            name ? name : "",
            patchAddress,
            current[0], current[1], current[2], current[3], current[4],
            expectedPoolPtr,
            poolPtr);
        return false;
    }

    const uintptr_t continueAddress = patchAddress + 5;
    const uintptr_t stub = CreatePoolAllocateGuardStub(expectedPoolPtr, continueAddress);
    if (!stub) {
        Log("CLEO+ pool allocate guard: %s stub allocation failed at 0x%08X poolPtr=0x%08X",
            name ? name : "",
            patchAddress,
            expectedPoolPtr);
        return false;
    }

    *continueOut = continueAddress;
    if (WriteRel32Jump(patchAddress, stub)) {
        Log("CLEO+ pool allocate guard: installed %s at 0x%08X continue=0x%08X stub=0x%08X poolPtr=0x%08X",
            name ? name : "",
            patchAddress,
            *continueOut,
            stub,
            expectedPoolPtr);
        return true;
    }

    VirtualFree(reinterpret_cast<void*>(stub), 0, MEM_RELEASE);
    *continueOut = 0;
    return false;
#else
    (void)name;
    (void)patchAddress;
    (void)expectedPoolPtr;
    (void)guardTarget;
    (void)continueOut;
    return false;
#endif
}

void InstallCleoPlusPoolAllocateGuard()
{
#if defined(_M_IX86)
    HMODULE cleoPlus = GetModuleHandleA("CLEO+.cleo");
    if (!cleoPlus) {
        Log("CLEO+ pool allocate guard: CLEO+.cleo not loaded yet");
        return;
    }

    const uintptr_t base = reinterpret_cast<uintptr_t>(cleoPlus);

    // Try pattern scan first, then fall back to known-version hash table.
    uintptr_t objAddr = FindAllocateBlocksByPattern(cleoPlus, "CLEO+ ObjectExtendedData", kOriginalObjectPoolPtr);
    uintptr_t vehAddr = FindAllocateBlocksByPattern(cleoPlus, "CLEO+ VehicleExtendedData", kOriginalVehiclePoolPtr);
    uintptr_t pedAddr = FindAllocateBlocksByPattern(cleoPlus, "CLEO+ PedExtendedData", kOriginalPedPoolPtr);

    // If pattern scan failed entirely, use the legacy hard-coded offsets as ultimate fallback.
    if (!objAddr) {
        objAddr = base + 0x30590;
        Log("CLEO+ pool allocate guard: Object pattern failed, using legacy fallback 0x%08X", objAddr);
    }
    if (!vehAddr) {
        vehAddr = base + 0x30700;
        Log("CLEO+ pool allocate guard: Vehicle pattern failed, using legacy fallback 0x%08X", vehAddr);
    }
    if (!pedAddr) {
        pedAddr = base + 0x30870;
        Log("CLEO+ pool allocate guard: Ped pattern failed, using legacy fallback 0x%08X", pedAddr);
    }

    InstallOneCleoPlusPoolAllocateGuard(
        "CLEO+ ObjectPool accessor (AllocateBlocks-like prologue)",
        objAddr,
        kOriginalObjectPoolPtr,
        0,
        &g_cleoPlusObjectAllocateBlocksContinue);
    InstallOneCleoPlusPoolAllocateGuard(
        "VehicleExtendedData::AllocateBlocks",
        vehAddr,
        kOriginalVehiclePoolPtr,
        0,
        &g_cleoPlusVehicleAllocateBlocksContinue);
    InstallOneCleoPlusPoolAllocateGuard(
        "PedExtendedData::AllocateBlocks",
        pedAddr,
        kOriginalPedPoolPtr,
        0,
        &g_cleoPlusPedAllocateBlocksContinue);
#endif
}

void InstallMixSetsPoolAllocateGuard()
{
#if defined(_M_IX86)
    if (g_mixSetsPedAllocateBlocksContinue) {
        return;
    }

    HMODULE mixSets = GetModuleHandleA("mixsets.asi");
    if (!mixSets) {
        mixSets = GetModuleHandleA("MixSets.asi");
    }
    if (!mixSets) {
        mixSets = FindLoadedModuleBySubstring("mixsets");
    }
    if (!mixSets) {
        Log("MixSets pool allocate guard: MixSets.asi not loaded yet");
        return;
    }

    uintptr_t pedAddr = FindAllocateBlocksByPattern(mixSets, "MixSets PedExtendedData", kOriginalPedPoolPtr);
    if (!pedAddr) {
        pedAddr = reinterpret_cast<uintptr_t>(mixSets) + 0x00CCB0;
        Log("MixSets pool allocate guard: pattern failed, using legacy fallback 0x%08X", pedAddr);
    }

    InstallOneCleoPlusPoolAllocateGuard(
        "MixSets PedExtendedData::AllocateBlocks",
        pedAddr,
        kOriginalPedPoolPtr,
        0,
        &g_mixSetsPedAllocateBlocksContinue);
#endif
}

DWORD WINAPI MixSetsPoolAllocateGuardInstallThread(void*)
{
    for (int attempt = 0; attempt < 300; ++attempt) {
        if (!g_config.enableMixSetsPoolAllocateGuard || g_mixSetsPedAllocateBlocksContinue) {
            return 0;
        }

        if (FindLoadedModuleBySubstring("mixsets")) {
            InstallMixSetsPoolAllocateGuard();
            return 0;
        }

        if (attempt == 0 || attempt == 50 || attempt == 150) {
            Log("MixSets pool allocate guard: waiting for module attempt=%d", attempt);
        }
        Sleep(100);
    }

    Log("MixSets pool allocate guard: module still not loaded after delayed install window");
    return 0;
}

void InstallUrbanizePoolAllocateGuard()
{
#if defined(_M_IX86)
    if (g_urbanizePedAllocateBlocksContinue) {
        return;
    }

    HMODULE urbanize = GetModuleHandleA("urbanize (junior_djjr).asi");
    if (!urbanize) {
        urbanize = FindLoadedModuleBySubstring("urbanize");
    }
    if (!urbanize) {
        Log("Urbanize pool allocate guard: urbanize module not loaded yet");
        return;
    }

    uintptr_t pedAddr = FindAllocateBlocksByPattern(urbanize, "Urbanize PedExtendedData", kOriginalPedPoolPtr);
    if (!pedAddr) {
        pedAddr = reinterpret_cast<uintptr_t>(urbanize) + 0x018750;
        Log("Urbanize pool allocate guard: pattern failed, using legacy fallback 0x%08X", pedAddr);
    }

    InstallOneCleoPlusPoolAllocateGuard(
        "Urbanize PedExtendedData::AllocateBlocks",
        pedAddr,
        kOriginalPedPoolPtr,
        0,
        &g_urbanizePedAllocateBlocksContinue);
#endif
}

DWORD WINAPI UrbanizePoolAllocateGuardInstallThread(void*)
{
    for (int attempt = 0; attempt < 300; ++attempt) {
        if (!g_config.enableUrbanizePoolAllocateGuard || g_urbanizePedAllocateBlocksContinue) {
            return 0;
        }

        if (FindLoadedModuleBySubstring("urbanize")) {
            InstallUrbanizePoolAllocateGuard();
            return 0;
        }

        if (attempt == 0 || attempt == 50 || attempt == 150) {
            Log("Urbanize pool allocate guard: waiting for module attempt=%d", attempt);
        }
        Sleep(100);
    }

    Log("Urbanize pool allocate guard: module still not loaded after delayed install window");
    return 0;
}

uintptr_t FindVehFuncsVehicleAllocateBlocks(HMODULE vehFuncs)
{
#if defined(_M_IX86)
    if (!vehFuncs) {
        return 0;
    }

    const uintptr_t base = reinterpret_cast<uintptr_t>(vehFuncs);
    MODULEINFO mi{};
    if (!GetModuleInformation(GetCurrentProcess(), vehFuncs, &mi, sizeof(mi))) {
        return 0;
    }

    PIMAGE_DOS_HEADER dos = reinterpret_cast<PIMAGE_DOS_HEADER>(base);
    PIMAGE_NT_HEADERS nt = reinterpret_cast<PIMAGE_NT_HEADERS>(base + dos->e_lfanew);
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt);
    uintptr_t textBase = 0;
    size_t textSize = 0;
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section) {
        if (std::memcmp(section->Name, ".text\0\0\0", 8) == 0) {
            textBase = base + section->VirtualAddress;
            textSize = section->Misc.VirtualSize;
            break;
        }
    }
    if (!textBase || textSize < 96 || !IsReadableCommitted(textBase, textSize)) {
        return 0;
    }

    uintptr_t firstMatch = 0;
    uintptr_t secondMatch = 0;
    for (uintptr_t address = textBase; address + 96 <= textBase + textSize; ++address) {
        if (!LooksLikePluginSdkPoolAllocateBlocks(address, kOriginalVehiclePoolPtr)) {
            continue;
        }
        if (!firstMatch) {
            firstMatch = address;
        } else {
            secondMatch = address;
            break;
        }
    }

    if (firstMatch && !secondMatch) {
        Log("VehFuncs pool allocate guard: VehicleExtendedData pattern single match at 0x%08X rva=0x%08X",
            firstMatch,
            static_cast<uint32_t>(firstMatch - base));
        return firstMatch;
    }

    if (firstMatch && secondMatch) {
        Log("VehFuncs pool allocate guard: VehicleExtendedData pattern ambiguous first=0x%08X second=0x%08X",
            firstMatch,
            secondMatch);
        return UINTPTR_MAX;
    }

    return 0;
#else
    (void)vehFuncs;
    return 0;
#endif
}

void InstallVehFuncsPoolAllocateGuard()
{
#if defined(_M_IX86)
    if (g_vehFuncsVehicleAllocateBlocksContinue) {
        return;
    }

    HMODULE vehFuncs = GetModuleHandleA("VehFuncs.asi");
    if (!vehFuncs) {
        Log("VehFuncs pool allocate guard: vehfuncs module not loaded yet");
        return;
    }

    uintptr_t vehAddr = FindVehFuncsVehicleAllocateBlocks(vehFuncs);
    if (vehAddr == UINTPTR_MAX) {
        Log("VehFuncs pool allocate guard: ambiguous VehicleExtendedData match; guard not installed");
        return;
    }
    if (!vehAddr) {
        vehAddr = reinterpret_cast<uintptr_t>(vehFuncs) + 0x029650;
        Log("VehFuncs pool allocate guard: pattern failed, using observed fallback 0x%08X", vehAddr);
    }

    InstallOneCleoPlusPoolAllocateGuard(
        "VehFuncs VehicleExtendedData::AllocateBlocks",
        vehAddr,
        kOriginalVehiclePoolPtr,
        0,
        &g_vehFuncsVehicleAllocateBlocksContinue);
#endif
}

DWORD WINAPI VehFuncsPoolAllocateGuardInstallThread(void*)
{
    int installAttempts = 0;
    for (int attempt = 0; attempt < 300; ++attempt) {
        if (!g_config.enableVehFuncsPoolAllocateGuard || g_vehFuncsVehicleAllocateBlocksContinue) {
            return 0;
        }

        if (GetModuleHandleA("VehFuncs.asi")) {
            InstallVehFuncsPoolAllocateGuard();
            if (g_vehFuncsVehicleAllocateBlocksContinue) {
                return 0;
            }
            if (++installAttempts >= 5) {
                Log("VehFuncs pool allocate guard: install failed after %d retries", installAttempts);
                return 0;
            }
        }

        if (attempt == 0 || attempt == 50 || attempt == 150) {
            Log("VehFuncs pool allocate guard: waiting for module attempt=%d", attempt);
        }
        Sleep(100);
    }

    Log("VehFuncs pool allocate guard: module still not loaded after delayed install window");
    return 0;
}

void StartVehFuncsPoolAllocateGuardInstaller()
{
    if (!g_config.enableVehFuncsPoolAllocateGuard || g_vehFuncsVehicleAllocateBlocksContinue) {
        return;
    }
    if (InterlockedCompareExchange(&g_vehFuncsPoolGuardInstallerStarted, 1, 0) != 0) {
        return;
    }

    InstallVehFuncsPoolAllocateGuard();
    if (g_vehFuncsVehicleAllocateBlocksContinue) {
        return;
    }

    HANDLE thread = CreateThread(nullptr, 0, VehFuncsPoolAllocateGuardInstallThread, nullptr, 0, nullptr);
    if (thread) {
        CloseHandle(thread);
    } else {
        InterlockedExchange(&g_vehFuncsPoolGuardInstallerStarted, 0);
        Log("VehFuncs pool allocate guard: delayed install thread creation failed gle=%lu", GetLastError());
    }
}

bool LooksLikePluginSdkPoolAllocateBlocks(uintptr_t address, uintptr_t expectedPoolPtr)
{
    uint8_t first[5]{};
    __try {
        std::memcpy(first, reinterpret_cast<const void*>(address), sizeof(first));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    uintptr_t decodedPoolPtr = 0;
    std::memcpy(&decodedPoolPtr, first + 1, sizeof(decodedPoolPtr));
    if (first[0] != 0xA1 || decodedPoolPtr != expectedPoolPtr) {
        return false;
    }

    static const uint8_t thisSetup[] = { 0x56, 0x8B, 0xF1 };       // push esi; mov esi, ecx
    static const uint8_t readPoolCount[] = { 0x8B, 0x40, 0x08 };   // mov eax, [eax+8]
    static const uint8_t writeCount[] = { 0x89, 0x46, 0x08 };      // mov [esi+8], eax
    static const uint8_t writeArray[] = { 0x89, 0x46, 0x04 };      // mov [esi+4], eax

    return HasBytesInRange(address + 5, 20, thisSetup, sizeof(thisSetup)) &&
        HasBytesInRange(address + 5, 28, readPoolCount, sizeof(readPoolCount)) &&
        HasBytesInRange(address + 5, 48, writeCount, sizeof(writeCount)) &&
        HasBytesInRange(address + 5, 96, writeArray, sizeof(writeArray));
}

uint32_t ScanOneModuleForPoolAllocateGuards(const MODULEENTRY32& me, uint32_t remainingPatchBudget)
{
    if (!remainingPatchBudget || !ModuleAllowedForAutoPoolGuard(me.szModule, me.szExePath)) {
        return 0;
    }

    struct PoolPattern {
        const char* name;
        uintptr_t poolPtr;
    };

    const PoolPattern patterns[] = {
        { "PedExtendedData::AllocateBlocks", kOriginalPedPoolPtr },
        { "VehicleExtendedData::AllocateBlocks", kOriginalVehiclePoolPtr },
        { "CLEO+ ObjectPool accessor (AllocateBlocks-like prologue)", kOriginalObjectPoolPtr },
    };

    const uintptr_t base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
    const uintptr_t end = base + static_cast<uintptr_t>(me.modBaseSize);
    uint32_t patched = 0;

    for (uintptr_t regionCursor = base; regionCursor < end && patched < remainingPatchBudget;) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(reinterpret_cast<const void*>(regionCursor), &mbi, sizeof(mbi)) || !mbi.RegionSize) {
            break;
        }

        const uintptr_t regionBase = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        const uintptr_t regionEnd = regionBase + static_cast<uintptr_t>(mbi.RegionSize);
        const uintptr_t scanStart = regionBase > base ? regionBase : base;
        const uintptr_t scanEnd = regionEnd < end ? regionEnd : end;
        regionCursor = regionEnd > regionCursor ? regionEnd : regionCursor + 0x1000;

        if (!IsExecutableRegion(mbi) || scanEnd <= scanStart + 5) {
            continue;
        }

        for (uintptr_t address = scanStart; address + 96 <= scanEnd && patched < remainingPatchBudget; ++address) {
            for (const PoolPattern& pattern : patterns) {
                if (!LooksLikePluginSdkPoolAllocateBlocks(address, pattern.poolPtr)) {
                    continue;
                }

                uintptr_t ignoredContinue = 0;
                if (InstallOneCleoPlusPoolAllocateGuard(pattern.name, address, pattern.poolPtr, 0, &ignoredContinue)) {
                    ++patched;
                    Log("auto pool guard: patched module=%s pattern=%s rva=0x%08X path=%s",
                        me.szModule,
                        pattern.name,
                        static_cast<uint32_t>(address - base),
                        me.szExePath);
                }
                break;
            }
        }
    }

    return patched;
}

void InstallAutoPoolAllocateGuards()
{
    if (!g_config.enableAutoPoolAllocateGuard || g_config.autoPoolAllocateGuardMaxPatches <= 0) {
        return;
    }

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE) {
        Log("auto pool guard: module snapshot failed gle=%lu", GetLastError());
        return;
    }

    uint32_t patched = 0;
    MODULEENTRY32 me{};
    me.dwSize = sizeof(me);

    Log("auto pool guard: scan begin allowlist='%s' denylist='%s' maxPatches=%d deferredReplay=%d",
        g_config.autoPoolAllocateGuardAllowlist,
        g_config.autoPoolAllocateGuardDenylist,
        g_config.autoPoolAllocateGuardMaxPatches,
        g_config.enableDeferredPoolAllocateReplay ? 1 : 0);

    if (Module32First(snapshot, &me)) {
        do {
            if (patched >= static_cast<uint32_t>(g_config.autoPoolAllocateGuardMaxPatches)) {
                break;
            }
            patched += ScanOneModuleForPoolAllocateGuards(
                me,
                static_cast<uint32_t>(g_config.autoPoolAllocateGuardMaxPatches) - patched);
        } while (Module32Next(snapshot, &me));
    }

    CloseHandle(snapshot);
    Log("auto pool guard: scan end patched=%u", patched);
}

