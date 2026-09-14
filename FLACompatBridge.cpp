#if defined(_M_IX86)
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetApiVersion=_FLACompatBridge_GetApiVersion@0")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetFileIdCapacity=_FLACompatBridge_GetFileIdCapacity@0")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetCompatFlags=_FLACompatBridge_GetCompatFlags@0")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetRuntimeSource=_FLACompatBridge_GetRuntimeSource@0")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetRelocatedAddress=_FLACompatBridge_GetRelocatedAddress@4")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetRelocatedAddressByName=_FLACompatBridge_GetRelocatedAddressByName@8")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetModelInfoEntry=_FLACompatBridge_GetModelInfoEntry@4")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetModelInfo=_FLACompatBridge_GetModelInfo@4")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetStreamingInfo=_FLACompatBridge_GetStreamingInfo@4")
#pragma comment(linker, "/EXPORT:FLACompatBridge_ReadStreamingInfo=_FLACompatBridge_ReadStreamingInfo@12")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetPoolInfo=_FLACompatBridge_GetPoolInfo@12")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetPoolInfoByName=_FLACompatBridge_GetPoolInfoByName@12")
#pragma comment(linker, "/EXPORT:FLACompatBridge_RebuildSpecialActorCatalog=_FLACompatBridge_RebuildSpecialActorCatalog@0")
#pragma comment(linker, "/EXPORT:FLACompatBridge_GetSpecialActorName=_FLACompatBridge_GetSpecialActorName@12")
#pragma comment(linker, "/EXPORT:FLACompatBridge_RequestSpecialActor=_FLACompatBridge_RequestSpecialActor@8")
#pragma comment(linker, "/EXPORT:FLACompatBridge_RequestSpecialActorByCode=_FLACompatBridge_RequestSpecialActorByCode@8")
#pragma comment(linker, "/EXPORT:FLACompatBridge_ReleaseSpecialActor=_FLACompatBridge_ReleaseSpecialActor@4")
#pragma comment(linker, "/EXPORT:FLACompatBridge_IsModelLoaded=_FLACompatBridge_IsModelLoaded@4")
#endif

#include "FLACompatBridgeInternal.h"

bool BreakRepeatedNonExecutableExceptionLoop(EXCEPTION_POINTERS* info)
{
    if (!g_config.enableExceptionLoopBreaker || !info || !info->ExceptionRecord ||
        info->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION) {
        return false;
    }

    const uintptr_t eip = reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress);
    const uintptr_t fault = info->ExceptionRecord->NumberParameters > 1
        ? static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1])
        : 0;

    if (!eip || IsExecutableCommitted(eip)) {
        return false;
    }

    static uintptr_t lastEip = 0;
    static uintptr_t lastFault = 0;
    static DWORD firstTick = 0;
    static LONG sameCount = 0;
    static LONG tripped = 0;

    const DWORD now = GetTickCount();
    if (eip != lastEip || fault != lastFault) {
        lastEip = eip;
        lastFault = fault;
        firstTick = now;
        sameCount = 1;
        return false;
    }

    const LONG count = ++sameCount;
    if (count < g_config.exceptionLoopBreakerThreshold || InterlockedExchange(&tripped, 1) != 0) {
        return false;
    }

    Log("exception loop breaker: repeated non-executable EIP eip=0x%08X fault=0x%08X count=%ld elapsedMs=%lu terminate=%d",
        eip,
        fault,
        count,
        now - firstTick,
        g_config.exceptionLoopBreakerTerminate ? 1 : 0);
    LogMemoryRegion("loop-breaker-eip", eip);
    LogMemoryRegion("loop-breaker-fault", fault);
#if defined(_M_IX86)
    if (info->ContextRecord) {
        Log("exception loop breaker context: eax=0x%08X ebx=0x%08X ecx=0x%08X edx=0x%08X esi=0x%08X edi=0x%08X esp=0x%08X",
            info->ContextRecord->Eax,
            info->ContextRecord->Ebx,
            info->ContextRecord->Ecx,
            info->ContextRecord->Edx,
            info->ContextRecord->Esi,
            info->ContextRecord->Edi,
            info->ContextRecord->Esp);
        LogStackModules(info->ContextRecord->Esp);
    }
#endif

    if (g_config.exceptionLoopBreakerTerminate) {
        TerminateProcess(GetCurrentProcess(), 0xFAEE0001u);
    }

    return true;
}

#if defined(_M_IX86)
bool SkipReplayMarkEverythingAsNew(CONTEXT* ctx, const char* reason, bool restoreSavedEsi)
{
    if (!ctx) {
        return false;
    }

    const uintptr_t savedEsiSlot = ctx->Esp;
    const uintptr_t returnSlot = ctx->Esp + (restoreSavedEsi ? sizeof(uintptr_t) : 0);
    uint32_t returnAddress = 0;
    if (!SafeReadU32(returnSlot, &returnAddress) ||
        !returnAddress ||
        !IsExecutableCommitted(returnAddress)) {
        Log("replay pool skip guard: cannot skip reason=%s esp=0x%08X returnSlot=0x%08X return=0x%08X executable=%d",
            reason ? reason : "<null>",
            ctx->Esp,
            returnSlot,
            returnAddress,
            returnAddress && IsExecutableCommitted(returnAddress) ? 1 : 0);
        return false;
    }

    uint32_t savedEsi = 0;
    if (restoreSavedEsi && !SafeReadU32(savedEsiSlot, &savedEsi)) {
        Log("replay pool skip guard: cannot restore ESI reason=%s esp=0x%08X",
            reason ? reason : "<null>",
            ctx->Esp);
        return false;
    }

    static LONG logCount = 0;
    const LONG count = InterlockedIncrement(&logCount);
    if (count <= 16) {
        Log("replay pool skip guard: skipped CReplay::MarkEverythingAsNew reason=%s eip=0x%08X return=0x%08X esp=0x%08X restoreSavedEsi=%d savedEsi=0x%08X",
            reason ? reason : "<null>",
            ctx->Eip,
            returnAddress,
            ctx->Esp,
            restoreSavedEsi ? 1 : 0,
            savedEsi);
        LogStackModules(ctx->Esp);
    }

    if (restoreSavedEsi) {
        ctx->Esi = savedEsi;
    }
    ctx->Esp = returnSlot + sizeof(uintptr_t);
    ctx->Eip = returnAddress;
    return true;
}
#endif

const char* KnownGameAddressName(uintptr_t eip)
{
    switch (eip) {
    case 0x0053147D:
        return "CControllerConfigManager::AffectPadFromKeyBoard loop advance";
    case kCPtrListSingleAddItemNullWrite:
        return "CPtrListSingleLink::AddItem null PtrNodeSingle allocation";
    case kCPtrListDoubleAddItemNullWrite:
        return "CPtrListDoubleLink::AddItem null PtrNodeDouble allocation";
    case kCQuadTreeNodeAddItemNullWrite:
        return "CQuadTreeNode::AddItem null PtrNodeSingle allocation";
    case kCPtrNodeSingleLinkPoolNewEntry:
        return "CPtrNodeSingleLinkPool::New / CPools::ms_pPtrNodeSingleLinkPool allocation";
    case kCPtrNodeDoubleLinkPoolNewEntry:
        return "CPtrNodeDoubleLinkPool::New / CPools::ms_pPtrNodeDoubleLinkPool allocation";
    case 0x00533650:
        return "CPtrListSingleLink::RemoveItem link update";
    case 0x00533769:
        return "CEntity::GetRectAdd caller area";
    case 0x0054F3B3:
        return "CPlaceable::RemoveMatrix null matrix access";
    case 0x005A1FA1:
        return "CObject create patched callsite 1";
    case 0x005A2016:
        return "CObject create patched callsite 2";
    case 0x0059FB1E:
        return "CLEO+/object-create inline patch area";
    case 0x00582870:
        return "CRadar::GetActualBlipArrayIndex entry";
    case 0x00582889:
        return "CRadar::GetActualBlipArrayIndex counter read";
    case kCBuildingPoolNewEntry:
        return "CBuildingPool::New / CPools::ms_pBuildingPool allocation";
    case kCDummyPoolNewEntry:
        return "CDummyPool::New / CPools::ms_pDummyPool allocation";
    case kCIplStoreRemoveIplObjectPoolRead:
        return "CIplStore::RemoveIpl object pool size read";
    case kCReplayMarkEverythingAsNewPedPoolRead:
        return "CReplay::MarkEverythingAsNew ped pool size read";
    case kCReplayMarkEverythingAsNewVehiclePoolRead:
        return "CReplay::MarkEverythingAsNew vehicle pool size read";
    case kCEntryInfoNodePoolNewEntry:
        return "CEntryInfoNodePool::New / CPools::ms_pEntryInfoNodePool allocation";
    case 0x0040FB80:
        return "CColModelPool::New / CPools::ms_pColModelPool allocation";
    case 0x005B31A5:
        return "CColAccel::startCache / CColStore pool size read";
    case 0x006F7524:
        return "CTrain::InitTrains train carriage/IPL init read";
    case 0x0061A500:
        return "CPool::New generic / Tasks pool allocation";
    case 0x0061A5A0:
        return "CTask::operator new / CPools::ms_pTaskPool";
    case kRpClumpForAllAtomicsNullClumpRead:
        return "RpClumpForAllAtomics null/corrupt clump read";
    case kRpAnimBlendAllocateData:
        return "RpAnimBlendAllocateData entry";
    case kRpAnimBlendAllocateDataWrite:
        return "RpAnimBlendAllocateData clump plugin write";
    case kRpAnimBlendClumpFillFrameArray:
        return "RpAnimBlendClumpFillFrameArray entry";
    case kRpAnimBlendClumpFillFrameArrayRead:
        return "RpAnimBlendClumpFillFrameArray clump plugin read";
    case kRpAnimBlendClumpInit:
        return "RpAnimBlendClumpInit entry";
    case 0x004D41C0:
        return "CAnimManager::UncompressAnimation";
    case 0x004D1490:
        return "CAnimBlendAssociation::UpdateBlend";
    case 0x004CEEC0:
        return "CAnimBlendAssociation::Init(static)";
    case 0x004D2B90:
    case 0x004D2C30:
        return "FrameUpdateCallBackSkinned";
    case 0x004D1680:
    case 0x004D1710:
        return "FrameUpdateCallBackSkinnedWithVelocityExtraction";
    default:
        return "";
    }
}

void LogCrashClassification(EXCEPTION_POINTERS* info)
{
    if (!g_config.enableCrashClassification || !info || !info->ExceptionRecord) {
        return;
    }

    const DWORD exceptionCode = info->ExceptionRecord->ExceptionCode;
    const uintptr_t eip = reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress);
    const bool hasAccessType =
        (exceptionCode == EXCEPTION_ACCESS_VIOLATION || exceptionCode == EXCEPTION_IN_PAGE_ERROR) &&
        info->ExceptionRecord->NumberParameters > 0;
    const uintptr_t accessType = hasAccessType
        ? static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[0])
        : 0;
    const uintptr_t fault = info->ExceptionRecord->NumberParameters > 1
        ? static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1])
        : 0;
    const char* accessName = !hasAccessType ? "n/a" :
        accessType == 0 ? "read" :
        accessType == 1 ? "write" :
        accessType == 8 ? "execute" : "other";

    char moduleName[MAX_PATH]{};
    const uintptr_t moduleBase = ModuleBaseFromAddress(eip, moduleName, sizeof(moduleName));
    const char* known = KnownGameAddressName(eip);
    const bool lowFault = fault < 0x10000;
    DWORD eipState = 0;
    DWORD eipProtect = 0;
    const bool staleExec = IsFreeOrNonExecutableRegion(eip, &eipState, &eipProtect) && moduleBase == 0;

#if defined(_M_IX86)
    CONTEXT* ctx = info->ContextRecord;
    const uintptr_t esp = ctx ? ctx->Esp : 0;
    const bool stackUrbanize = esp && StackMentionsModule(esp, "urbanize");
    const bool stackFla = esp && StackMentionsModule(esp, "fastman92");
    const bool stackCleo = esp && StackMentionsModule(esp, "cleo");
    const bool stackScriptOpcode = esp && StackMentionsGtaScriptOpcodePath(esp);
#else
    const uintptr_t esp = 0;
    const bool stackUrbanize = false;
    const bool stackFla = false;
    const bool stackCleo = false;
    const bool stackScriptOpcode = false;
#endif

    const char* category = "UNKNOWN";
    if (staleExec && stackScriptOpcode) {
        category = "SCRIPT_OR_PLUGIN_STALE_EXEC_POINTER";
    } else if (staleExec) {
        category = "STALE_EXEC_POINTER_OR_FREED_CODE";
    } else if (eip == kCPtrListSingleAddItemNullWrite ||
        eip == kCPtrListDoubleAddItemNullWrite ||
        eip == kCQuadTreeNodeAddItemNullWrite) {
        category = "PTR_NODE_POOL_EXHAUSTED";
    } else if (eip == kCBuildingPoolNewEntry) {
        category = "BUILDING_POOL_ALLOCATION";
    } else if (eip == kCDummyPoolNewEntry) {
        category = "DUMMY_POOL_ALLOCATION";
    } else if (eip == kCIplStoreRemoveIplObjectPoolRead) {
        category = "IPL_REMOVE_OBJECT_POOL_READ";
    } else if (eip == kCReplayMarkEverythingAsNewPedPoolRead ||
        eip == kCReplayMarkEverythingAsNewVehiclePoolRead) {
        category = "REPLAY_POOL_POINTER_READ";
    } else if (eip == kCEntryInfoNodePoolNewEntry) {
        category = "ENTRY_INFO_NODE_POOL_ALLOCATION";
    } else if (eip == kCColModelPoolNewEntry) {
        category = "COL_MODEL_POOL_ALLOCATION";
    } else if (eip == kCPtrNodeSingleLinkPoolNewEntry || eip == kCPtrNodeDoubleLinkPoolNewEntry) {
        category = "PTR_NODE_POOL_ALLOCATION";
    } else if (eip >= 0x00582870 && eip <= 0x00582899) {
        category = "RADAR_BLIP_HANDLE_OR_TRACE_LIMIT";
    } else if (eip == kCColAccelStartCachePoolRead) {
        category = "COL_ACCEL_OR_RELOCATED_COL_POOL";
    } else if (eip == kGenericPoolNewEntry) {
        category = "TASK_POOL_ALLOCATION";
    } else if (eip >= 0x006F7400 && eip <= 0x006F7800) {
        category = "TRAIN_INIT_OR_FLA_CARRIAGE_LOADER";
    } else if (eip == kRpClumpForAllAtomicsNullClumpRead) {
        category = "RW_CLUMP_ATOMICS_NULL_OR_BAD_CLUMP";
    } else if (eip == 0x0054F3B3) {
        category = "PLACEABLE_MATRIX_LIFETIME";
    } else if (eip >= 0x00531140 && eip <= 0x0053149E) {
        category = "CONTROLLER_KEYBOARD_STATE_OR_RUNTIME_PATCH";
    } else if (eip == kRpAnimBlendAllocateDataWrite ||
        eip == kRpAnimBlendClumpFillFrameArray ||
        eip == kRpAnimBlendClumpFillFrameArrayRead ||
        eip == kRpAnimBlendClumpInit ||
        eip == 0x004D41C0 ||
        eip == 0x004CEEC0 ||
        (eip >= 0x004D1680 && eip <= 0x004D1A4A)) {
        category = "ANIMATION_POINTER";
    } else if (ContainsCaseInsensitive(moduleName, "CLEO")) {
        category = "CLEO_PLUGIN_OR_SCRIPT_BRIDGE";
    } else if (ContainsCaseInsensitive(moduleName, "fastman92")) {
        category = "FLA_INTERNAL_OR_FLA_HOOK";
    } else if (lowFault) {
        category = "NULL_OR_LOW_POINTER";
    }

    Log("crash-class: category=%s code=0x%08X access=%s(%u) eip=0x%08X module=%s+0x%X known='%s' fault=0x%08X lowFault=%d staleExec=%d eipState=%s eipProtect=%s stackUrbanize=%d stackFLA=%d stackCLEO=%d stackScriptOpcode=%d",
        category,
        exceptionCode,
        accessName,
        static_cast<unsigned>(accessType),
        eip,
        moduleName,
        moduleBase ? eip - moduleBase : 0,
        known,
        fault,
        lowFault ? 1 : 0,
        staleExec ? 1 : 0,
        StateName(eipState),
        ProtectName(eipProtect),
        stackUrbanize ? 1 : 0,
        stackFla ? 1 : 0,
        stackCleo ? 1 : 0,
        stackScriptOpcode ? 1 : 0);

#if defined(_M_IX86)
    if (ctx) {
        Log("crash-context: eax=0x%08X ebx=0x%08X ecx=0x%08X edx=0x%08X esi=0x%08X edi=0x%08X ebp=0x%08X esp=0x%08X eip=0x%08X eflags=0x%08X",
            ctx->Eax,
            ctx->Ebx,
            ctx->Ecx,
            ctx->Edx,
            ctx->Esi,
            ctx->Edi,
            ctx->Ebp,
            ctx->Esp,
            ctx->Eip,
            ctx->EFlags);
    }

    if (exceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        ContainsCaseInsensitive(moduleName, "gta_sa")) {
        LogBytes("crash-eip", eip, 16);
        if (esp) {
            LogDwords("crash-stack", esp, 16);
        }
    }
#endif
}

#if defined(_M_IX86)
bool SkipFailedPtrNodeListAdd(
    CONTEXT* ctx,
    const char* label,
    uintptr_t poolPtrAddress,
    uintptr_t fault,
    uintptr_t returnAddressOffset,
    uintptr_t itemOffset,
    uintptr_t finalEspAdvance)
{
    if (!ctx || ctx->Eax != 0 ||
        finalEspAdvance < sizeof(uintptr_t) ||
        !IsReadableCommitted(ctx->Esp, finalEspAdvance)) {
        return false;
    }

    uintptr_t savedEsi = 0;
    uintptr_t returnAddress = 0;
    uintptr_t item = 0;
    __try {
        savedEsi = *reinterpret_cast<const uintptr_t*>(ctx->Esp);
        returnAddress = *reinterpret_cast<const uintptr_t*>(ctx->Esp + returnAddressOffset);
        item = *reinterpret_cast<const uintptr_t*>(ctx->Esp + itemOffset);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    if (!IsExecutableCommitted(returnAddress)) {
        return false;
    }

    uint32_t pool = 0;
    uint32_t poolSize = 0;
    uint32_t firstFree = 0;
    SafeReadU32(poolPtrAddress, &pool);
    if (IsValidCPool(pool)) {
        SafeReadU32(pool + 0x08, &poolSize);
        SafeReadU32(pool + 0x0C, &firstFree);
    }

    const LONG count = InterlockedIncrement(&g_ptrNodeExhaustionGuardLogs);
    if (count <= 32) {
        Log("ptrnode exhaustion guard: skipped %s add after allocation failure eip=0x%08X fault=0x%08X poolPtr[0x%08X]=0x%08X size=%u firstFree=%u list=0x%08X item=0x%08X return=0x%08X savedEsi=0x%08X esp=0x%08X",
            label ? label : "<null>",
            ctx->Eip,
            fault,
            poolPtrAddress,
            pool,
            poolSize,
            firstFree,
            ctx->Esi,
            item,
            returnAddress,
            savedEsi,
            ctx->Esp);
        LogStackModules(ctx->Esp);
    }

    ctx->Esi = static_cast<DWORD>(savedEsi);
    ctx->Eip = static_cast<DWORD>(returnAddress);
    ctx->Esp += finalEspAdvance;
    return true;
}
#endif

LONG CALLBACK BridgeVectoredExceptionHandler(EXCEPTION_POINTERS* info)
{
    if (!info || !info->ExceptionRecord) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

#if defined(_M_IX86)
    if (g_config.enablePtrNodeExhaustionGuard &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t eip = reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress);
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);
        if (eip == kCPtrListSingleAddItemNullWrite && fault == 0x04) {
            if (SkipFailedPtrNodeListAdd(ctx, "CPtrListSingleLink::AddItem", kPtrNodeSinglePoolPtr, fault, 0x04, 0x08, 0x0C)) {
                return EXCEPTION_CONTINUE_EXECUTION;
            }
        } else if (eip == kCPtrListDoubleAddItemNullWrite && fault == 0x08) {
            if (SkipFailedPtrNodeListAdd(ctx, "CPtrListDoubleLink::AddItem", kPtrNodeDoublePoolPtr, fault, 0x04, 0x08, 0x0C)) {
                return EXCEPTION_CONTINUE_EXECUTION;
            }
        } else if (eip == kCQuadTreeNodeAddItemNullWrite && fault == 0x04) {
            if (SkipFailedPtrNodeListAdd(ctx, "CQuadTreeNode::AddItem", kPtrNodeSinglePoolPtr, fault, 0x14, 0x18, 0x20)) {
                return EXCEPTION_CONTINUE_EXECUTION;
            }
        }
    }

    if (g_config.enablePlaceableRemoveMatrixGuard &&
        g_config.enablePlaceableRemoveMatrixSkipGuard &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        (info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(static_cast<uintptr_t>(g_config.matrixGuardRemoveMatrixEntry)) ||
            info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(static_cast<uintptr_t>(g_config.matrixGuardRemoveMatrixNullLoad))) &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t eip = reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress);
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);
        const bool nullThisAtEntry = eip == g_config.matrixGuardRemoveMatrixEntry && ctx->Ecx == 0 && fault == 0x14;
        const bool nullMatrixAfterLoad = eip == g_config.matrixGuardRemoveMatrixNullLoad && ctx->Eax == 0 && fault < 0x10000;

        if ((nullThisAtEntry || nullMatrixAfterLoad) && MatrixGuardCodeLooksCompatible(eip)) {
            const uintptr_t returnAddress = ResolvePlaceableRemoveMatrixReturnAddress(ctx->Esp);

            if (returnAddress) {
                uintptr_t recoveredLink = 0;
                uintptr_t recoveredOwner = 0;
                const char* recoveredSource = "none";
                const bool recoveredMatrix = RecoverOldestMatrixLinkToFreeList(ctx->Ecx, &recoveredLink, &recoveredOwner, &recoveredSource);

                static LONG recoverCount = 0;
                const LONG count = InterlockedIncrement(&recoverCount);
                if (count <= 32) {
                    int32_t modelId = -1;
                    if (IsReadableCommitted(ctx->Esi + 0x22, sizeof(uint16_t))) {
                        modelId = ReadExtendedIdFrom16BitField(reinterpret_cast<const void*>(ctx->Esi + 0x22));
                    }

                    Log("placeable remove-matrix guard: skipped invalid RemoveMatrix eip=0x%08X fault=0x%08X this=0x%08X matrix=0x%08X possibleEntity=0x%08X model=%u recovered=%d source=%s link=0x%08X owner=0x%08X return=0x%08X esp=0x%08X",
                        eip,
                        fault,
                        ctx->Ecx,
                        ctx->Eax,
                        ctx->Esi,
                        modelId >= 0 ? static_cast<uint32_t>(modelId) : 0xFFFFFFFFu,
                        recoveredMatrix ? 1 : 0,
                        recoveredSource,
                        recoveredLink,
                        recoveredOwner,
                        returnAddress,
                        ctx->Esp);
                    LogStackModules(ctx->Esp);
                }

                ctx->Eip = static_cast<DWORD>(returnAddress);
                ctx->Esp += 0x10;
                return EXCEPTION_CONTINUE_EXECUTION;
            }
        }
    }

    if (g_config.enablePlaceableRemoveMatrixGuard &&
        g_config.enablePlaceableStaticMatrixAllocGuard &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(static_cast<uintptr_t>(g_config.matrixGuardAllocateStaticOwnerWrite)) &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);
        if (ctx->Eax == 0 && fault == 0x48 && ctx->Ecx == MatrixListBase() && ctx->Esi != 0 &&
            MatrixGuardCodeLooksCompatible(g_config.matrixGuardAllocateStaticOwnerWrite)) {
            uintptr_t recoveredLink = 0;
            uintptr_t recoveredOwner = 0;
            uintptr_t allocatedLink = 0;
            const char* recoveredSource = "none";

            bool allocated = AllocateStaticMatrixLinkFromFreeList(ctx->Esi, &allocatedLink);
            bool recovered = false;
            if (!allocated) {
                recovered = RecoverOldestMatrixLinkToFreeList(ctx->Esi, &recoveredLink, &recoveredOwner, &recoveredSource);
                if (recovered) {
                    allocated = AllocateStaticMatrixLinkFromFreeList(ctx->Esi, &allocatedLink);
                }
            }

            static LONG allocateRecoverCount = 0;
            const LONG count = InterlockedIncrement(&allocateRecoverCount);
            if (count <= 32) {
                int32_t modelId = -1;
                if (IsReadableCommitted(ctx->Esi + 0x22, sizeof(uint16_t))) {
                    modelId = ReadExtendedIdFrom16BitField(reinterpret_cast<const void*>(ctx->Esi + 0x22));
                }

                Log("placeable static-matrix alloc guard: eip=0x%08X fault=0x%08X entity=0x%08X model=%u allocated=%d allocLink=0x%08X recovered=%d source=%s recoveredLink=0x%08X recoveredOwner=0x%08X matrixBase=0x%08X esp=0x%08X",
                    g_config.matrixGuardAllocateStaticOwnerWrite,
                    fault,
                    ctx->Esi,
                    modelId >= 0 ? static_cast<uint32_t>(modelId) : 0xFFFFFFFFu,
                    allocated ? 1 : 0,
                    allocatedLink,
                    recovered ? 1 : 0,
                    recoveredSource,
                    recoveredLink,
                    recoveredOwner,
                    MatrixListBase(),
                    ctx->Esp);
                LogStackModules(ctx->Esp);
            }

            if (allocated) {
                ctx->Eax = static_cast<DWORD>(allocatedLink);
                ctx->Eip = g_config.matrixGuardAllocateStaticResume;
                return EXCEPTION_CONTINUE_EXECUTION;
            }
        }
    }

    if (g_config.enableReplayPoolReadSkipGuard &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t eip = reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress);
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);

        if (eip == kCReplayMarkEverythingAsNewPedPoolRead && ctx->Edx == 0 && fault == 0x08) {
            if (SkipReplayMarkEverythingAsNew(ctx, "Ped pool is null", false)) {
                return EXCEPTION_CONTINUE_EXECUTION;
            }
            return EXCEPTION_CONTINUE_SEARCH;
        }

        if (eip == kCReplayMarkEverythingAsNewVehiclePoolRead && ctx->Esi == 0 && fault == 0x08) {
            if (SkipReplayMarkEverythingAsNew(ctx, "Vehicle pool is null", true)) {
                return EXCEPTION_CONTINUE_EXECUTION;
            }
            return EXCEPTION_CONTINUE_SEARCH;
        }
    }

    if (g_config.enableLazyCPoolRegistry &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t eip = reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress);
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);

        if ((eip == kCPtrNodeSingleLinkPoolNewEntry || eip == kCPtrNodeDoubleLinkPoolNewEntry) &&
            ctx->Ecx == 0 && fault == 0x08) {
            TryEnsureCPoolsInitialised("CPtrNode pool allocation early core pool recovery", g_config.enableEarlyCPoolsInitialiseRecovery);
            const uintptr_t poolPtrAddress = eip == kCPtrNodeSingleLinkPoolNewEntry
                ? kPtrNodeSinglePoolPtr
                : kPtrNodeDoublePoolPtr;
            const char* reason = eip == kCPtrNodeSingleLinkPoolNewEntry
                ? "CPtrNodeSingleLinkPool::New"
                : "CPtrNodeDoubleLinkPool::New";

            EnsureBatchLazyCPoolsInitialised(reason);
            if (EnsureLazyCPoolReady(poolPtrAddress, reason)) {
                uint32_t pool = 0;
                SafeReadU32(poolPtrAddress, &pool);
                if (IsValidCPool(pool)) {
                    static LONG logCount = 0;
                    const LONG count = InterlockedIncrement(&logCount);
                    if (count <= 16) {
                        Log("ptrnode pool guard: supplied lazy pool eip=0x%08X poolPtr=0x%08X pool=0x%08X return=0x%08X",
                            eip,
                            poolPtrAddress,
                            pool,
                            IsReadableCommitted(ctx->Esp, sizeof(uintptr_t)) ? *reinterpret_cast<uintptr_t*>(ctx->Esp) : 0);
                        LogStackModules(ctx->Esp);
                    }
                    ctx->Ecx = pool;
                    return EXCEPTION_CONTINUE_EXECUTION;
                }
            }
        }
    }

    if (g_config.enableLazyCPoolRegistry &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(kCBuildingPoolNewEntry) &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);
        if (ctx->Ecx == 0 && fault == 0x08) {
            EnsureBatchLazyCPoolsInitialised("CBuildingPool::New");
            if (EnsureLazyCPoolReady(kOriginalBuildingPoolPtr, "CBuildingPool::New")) {
                uint32_t pool = 0;
                SafeReadU32(kOriginalBuildingPoolPtr, &pool);
                if (IsValidCPool(pool)) {
                    static LONG logCount = 0;
                    const LONG count = InterlockedIncrement(&logCount);
                    if (count <= 16) {
                        Log("building pool guard: supplied lazy pool eip=0x%08X pool=0x%08X return=0x%08X",
                            kCBuildingPoolNewEntry,
                            pool,
                            IsReadableCommitted(ctx->Esp, sizeof(uintptr_t)) ? *reinterpret_cast<uintptr_t*>(ctx->Esp) : 0);
                        LogStackModules(ctx->Esp);
                    }
                    ctx->Ecx = pool;
                    return EXCEPTION_CONTINUE_EXECUTION;
                }
            }
        }
    }

    if (g_config.enableLazyCPoolRegistry &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(kCDummyPoolNewEntry) &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);
        if (ctx->Ecx == 0 && fault == 0x08) {
            EnsureBatchLazyCPoolsInitialised("CDummyPool::New");
            if (EnsureLazyCPoolReady(kOriginalDummyPoolPtr, "CDummyPool::New")) {
                uint32_t pool = 0;
                SafeReadU32(kOriginalDummyPoolPtr, &pool);
                if (IsValidCPool(pool)) {
                    static LONG logCount = 0;
                    const LONG count = InterlockedIncrement(&logCount);
                    if (count <= 16) {
                        uint32_t poolSize = 0;
                        uint32_t firstFree = 0;
                        SafeReadU32(pool + 0x08, &poolSize);
                        SafeReadU32(pool + 0x0C, &firstFree);
                        Log("dummy pool guard: supplied lazy pool eip=0x%08X pool=0x%08X size=%u firstFree=%u return=0x%08X",
                            kCDummyPoolNewEntry,
                            pool,
                            poolSize,
                            firstFree,
                            IsReadableCommitted(ctx->Esp, sizeof(uintptr_t)) ? *reinterpret_cast<uintptr_t*>(ctx->Esp) : 0);
                        LogStackModules(ctx->Esp);
                    }
                    ctx->Ecx = pool;
                    return EXCEPTION_CONTINUE_EXECUTION;
                }
            }
        }
    }

    if (g_config.enableLazyCPoolRegistry &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(kCIplStoreRemoveIplObjectPoolRead) &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);
        if (ctx->Eax == 0 && fault == 0x08) {
            EnsureBatchLazyCPoolsInitialised("CIplStore::RemoveIpl");
            if (EnsureLazyCPoolReady(kOriginalObjectPoolPtr, "CIplStore::RemoveIpl")) {
                uint32_t pool = 0;
                SafeReadU32(kOriginalObjectPoolPtr, &pool);
                if (IsValidCPool(pool)) {
                    static LONG logCount = 0;
                    const LONG count = InterlockedIncrement(&logCount);
                    if (count <= 16) {
                        uint32_t poolSize = 0;
                        uint32_t firstFree = 0;
                        SafeReadU32(pool + 0x08, &poolSize);
                        SafeReadU32(pool + 0x0C, &firstFree);
                        Log("object pool guard: supplied lazy pool eip=0x%08X pool=0x%08X size=%u firstFree=%u return=0x%08X",
                            kCIplStoreRemoveIplObjectPoolRead,
                            pool,
                            poolSize,
                            firstFree,
                            IsReadableCommitted(ctx->Esp, sizeof(uintptr_t)) ? *reinterpret_cast<uintptr_t*>(ctx->Esp) : 0);
                        LogStackModules(ctx->Esp);
                    }
                    ctx->Eax = pool;
                    return EXCEPTION_CONTINUE_EXECUTION;
                }
            }
        }
    }

    if (!g_config.enableReplayPoolReadSkipGuard &&
        g_config.enableLazyCPoolRegistry &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t eip = reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress);
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);

        if (eip == kCReplayMarkEverythingAsNewPedPoolRead && ctx->Edx == 0 && fault == 0x08) {
            EnsureBatchLazyCPoolsInitialised("CReplay::MarkEverythingAsNew");
            EnsureLazyCPoolReady(kOriginalVehiclePoolPtr, "CReplay::MarkEverythingAsNew prerequisite");
            if (EnsureLazyCPoolReady(kOriginalPedPoolPtr, "CReplay::MarkEverythingAsNew")) {
                uint32_t pool = 0;
                SafeReadU32(kOriginalPedPoolPtr, &pool);
                if (IsValidCPool(pool)) {
                    static LONG logCount = 0;
                    const LONG count = InterlockedIncrement(&logCount);
                    if (count <= 16) {
                        uint32_t poolSize = 0;
                        uint32_t firstFree = 0;
                        SafeReadU32(pool + 0x08, &poolSize);
                        SafeReadU32(pool + 0x0C, &firstFree);
                        Log("replay pool guard: supplied lazy Ped pool eip=0x%08X pool=0x%08X size=%u firstFree=%u return=0x%08X",
                            eip,
                            pool,
                            poolSize,
                            firstFree,
                            IsReadableCommitted(ctx->Esp, sizeof(uintptr_t)) ? *reinterpret_cast<uintptr_t*>(ctx->Esp) : 0);
                        LogStackModules(ctx->Esp);
                    }
                    ctx->Edx = pool;
                    return EXCEPTION_CONTINUE_EXECUTION;
                }
            }
        }

        if (eip == kCReplayMarkEverythingAsNewVehiclePoolRead && ctx->Esi == 0 && fault == 0x08) {
            EnsureBatchLazyCPoolsInitialised("CReplay::MarkEverythingAsNew");
            if (EnsureLazyCPoolReady(kOriginalVehiclePoolPtr, "CReplay::MarkEverythingAsNew")) {
                uint32_t pool = 0;
                SafeReadU32(kOriginalVehiclePoolPtr, &pool);
                if (IsValidCPool(pool)) {
                    static LONG logCount = 0;
                    const LONG count = InterlockedIncrement(&logCount);
                    if (count <= 16) {
                        uint32_t poolSize = 0;
                        uint32_t firstFree = 0;
                        SafeReadU32(pool + 0x08, &poolSize);
                        SafeReadU32(pool + 0x0C, &firstFree);
                        Log("replay pool guard: supplied lazy Vehicle pool eip=0x%08X pool=0x%08X size=%u firstFree=%u return=0x%08X",
                            eip,
                            pool,
                            poolSize,
                            firstFree,
                            IsReadableCommitted(ctx->Esp, sizeof(uintptr_t)) ? *reinterpret_cast<uintptr_t*>(ctx->Esp) : 0);
                        LogStackModules(ctx->Esp);
                    }
                    ctx->Esi = pool;
                    return EXCEPTION_CONTINUE_EXECUTION;
                }
            }
        }
    }

    if (g_config.enableLazyCPoolRegistry &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(kCEntryInfoNodePoolNewEntry) &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);
        if (ctx->Ecx == 0 && fault == 0x08) {
            EnsureBatchLazyCPoolsInitialised("CEntryInfoNodePool::New");
            if (EnsureLazyCPoolReady(kEntryInfoNodePoolPtr, "CEntryInfoNodePool::New")) {
                uint32_t pool = 0;
                SafeReadU32(kEntryInfoNodePoolPtr, &pool);
                if (IsValidCPool(pool)) {
                    static LONG logCount = 0;
                    const LONG count = InterlockedIncrement(&logCount);
                    if (count <= 16) {
                        uint32_t poolSize = 0;
                        uint32_t firstFree = 0;
                        SafeReadU32(pool + 0x08, &poolSize);
                        SafeReadU32(pool + 0x0C, &firstFree);
                        Log("entry info node pool guard: supplied lazy pool eip=0x%08X pool=0x%08X size=%u firstFree=%u return=0x%08X",
                            kCEntryInfoNodePoolNewEntry,
                            pool,
                            poolSize,
                            firstFree,
                            IsReadableCommitted(ctx->Esp, sizeof(uintptr_t)) ? *reinterpret_cast<uintptr_t*>(ctx->Esp) : 0);
                        LogStackModules(ctx->Esp);
                    }
                    ctx->Ecx = pool;
                    return EXCEPTION_CONTINUE_EXECUTION;
                }
            }
        }
    }

    if (g_config.enableColModelPoolNewGuard &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(kCColModelPoolNewEntry) &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);
        if (ctx->Ecx == 0 && fault == 0x08) {
            uint32_t colModelPool = 0;
            SafeReadU32(kOriginalColModelPoolPtr, &colModelPool);
            if (!colModelPool) {
                EnsureLazyCPoolReady(kOriginalColModelPoolPtr, "CColModelPool::New");
                SafeReadU32(kOriginalColModelPoolPtr, &colModelPool);
            }
            const bool validPool =
                colModelPool != 0 &&
                IsReadableCommitted(colModelPool, 0x14) &&
                IsReadableCommitted(colModelPool + 0x08, sizeof(uint32_t));

            const LONG logCount = InterlockedIncrement(&g_colModelPoolNewGuardLogs);
            if (logCount <= 16) {
                uint32_t poolSize = 0;
                uint32_t firstFree = 0;
                if (validPool) {
                    SafeReadU32(colModelPool + 0x08, &poolSize);
                    SafeReadU32(colModelPool + 0x0C, &firstFree);
                }
                Log("col model pool guard: null this at CColModelPool::New fault=0x%08X poolPtr[0x%08X]=0x%08X valid=%d size=%u firstFree=%u return=0x%08X",
                    fault,
                    kOriginalColModelPoolPtr,
                    colModelPool,
                    validPool ? 1 : 0,
                    poolSize,
                    firstFree,
                    IsReadableCommitted(ctx->Esp, sizeof(uintptr_t)) ? *reinterpret_cast<uintptr_t*>(ctx->Esp) : 0);
                LogBytes("col-model-pool-new-eip", kCColModelPoolNewEntry, 64);
                LogStackModules(ctx->Esp);
            }

            if (validPool) {
                ctx->Ecx = colModelPool;
                return EXCEPTION_CONTINUE_EXECUTION;
            }
        }
    }

    if (info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(kCPedIntelligencePoolNewEntry) &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);
        if (ctx->Ecx == 0 && fault == 0x08) {
            if (EnsureLazyCPoolReady(kPedIntelligencePoolPtr, "CPedIntelligencePool::New")) {
                uint32_t pool = 0;
                SafeReadU32(kPedIntelligencePoolPtr, &pool);
                if (IsValidCPool(pool)) {
                    ctx->Ecx = pool;
                    return EXCEPTION_CONTINUE_EXECUTION;
                }
            }
        }
    }

    if (g_config.enableLazyCPoolRegistry &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t eip = reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress);
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);

        uintptr_t routePoolPtr = 0;
        const char* routeReason = nullptr;
        if (eip == kCEventPoolNewEntry) {
            routePoolPtr = kEventsPoolPtr;
            routeReason = "CEventPool::New";
        } else if (eip == kCPointRoutePoolNewEntry) {
            routePoolPtr = kPointRoutePoolPtr;
            routeReason = "CPointRoutePool::New";
        } else if (eip == kCNodeRoutePoolNewEntry) {
            routePoolPtr = kNodeRoutePoolPtr;
            routeReason = "CNodeRoutePool::New";
        } else if (eip == kCTaskAllocatorPoolNewEntry) {
            routePoolPtr = kTaskAllocatorPoolPtr;
            routeReason = "CTaskAllocatorPool::New";
        } else if (eip == kCPedAttractorPoolNewEntry) {
            routePoolPtr = kPedAttractorsPoolPtr;
            routeReason = "CPedAttractorPool::New";
        }

        if (routePoolPtr && ctx->Ecx == 0 && fault == 0x08) {
            EnsureBatchLazyCPoolsInitialised(routeReason);
            if (EnsureLazyCPoolReady(routePoolPtr, routeReason)) {
                uint32_t pool = 0;
                SafeReadU32(routePoolPtr, &pool);
                if (IsValidCPool(pool)) {
                    static LONG logCount = 0;
                    const LONG count = InterlockedIncrement(&logCount);
                    if (count <= 16) {
                        uint32_t poolSize = 0;
                        uint32_t firstFree = 0;
                        SafeReadU32(pool + 0x08, &poolSize);
                        SafeReadU32(pool + 0x0C, &firstFree);
                        Log("late CPool guard: supplied lazy pool reason=%s eip=0x%08X poolPtr=0x%08X pool=0x%08X size=%u firstFree=%u return=0x%08X",
                            routeReason,
                            eip,
                            routePoolPtr,
                            pool,
                            poolSize,
                            firstFree,
                            IsReadableCommitted(ctx->Esp, sizeof(uintptr_t)) ? *reinterpret_cast<uintptr_t*>(ctx->Esp) : 0);
                        LogStackModules(ctx->Esp);
                    }
                    ctx->Ecx = pool;
                    return EXCEPTION_CONTINUE_EXECUTION;
                }
            }
        }
    }

    if (g_config.enableLazyCPoolRegistry &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(kGenericPoolNewEntry) &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);
        if (ctx->Ecx == 0 && fault == 0x08) {
            uintptr_t returnAddress = 0;
            if (IsReadableCommitted(ctx->Esp, sizeof(returnAddress))) {
                __try {
                    returnAddress = *reinterpret_cast<const uintptr_t*>(ctx->Esp);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    returnAddress = 0;
                }
            }

            if (EnsureLazyCPoolReady(kTasksPoolPtr, "CTask pool generic CPool::New")) {
                uint32_t pool = 0;
                SafeReadU32(kTasksPoolPtr, &pool);
                if (IsValidCPool(pool)) {
                    static LONG logCount = 0;
                    const LONG count = InterlockedIncrement(&logCount);
                    if (count <= 16) {
                        uint32_t poolSize = 0;
                        uint32_t firstFree = 0;
                        SafeReadU32(pool + 0x08, &poolSize);
                        SafeReadU32(pool + 0x0C, &firstFree);
                        Log("task pool guard: supplied lazy Tasks pool for generic CPool::New pool=0x%08X size=%u firstFree=%u return=0x%08X esp=0x%08X",
                            pool,
                            poolSize,
                            firstFree,
                            returnAddress,
                            ctx->Esp);
                        LogStackModules(ctx->Esp);
                    }
                    ctx->Ecx = pool;
                    return EXCEPTION_CONTINUE_EXECUTION;
                }
            }
        }
    }

    if (g_config.enableColAccelStartCachePoolGuard &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(kCColAccelStartCachePoolRead) &&
        info->ExceptionRecord->NumberParameters > 1) {

        CONTEXT* ctx = info->ContextRecord;
        const uintptr_t fault = static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);
        if (fault == ctx->Eax + 0x08) {
            if (ctx->Eax == 0 && TryEnsureCPoolsInitialised("CColAccel::startCache early core pool recovery", g_config.enableEarlyCPoolsInitialiseRecovery)) {
                uint32_t colModelPool = 0;
                SafeReadU32(kOriginalColModelPoolPtr, &colModelPool);
                if (IsValidCPool(colModelPool)) {
                    const LONG logCount = InterlockedIncrement(&g_colAccelStartCachePoolGuardLogs);
                    if (logCount <= 16) {
                        uint32_t ped = 0;
                        uint32_t vehicle = 0;
                        uint32_t object = 0;
                        SafeReadU32(kOriginalPedPoolPtr, &ped);
                        SafeReadU32(kOriginalVehiclePoolPtr, &vehicle);
                        SafeReadU32(kOriginalObjectPoolPtr, &object);
                        Log("col accel guard: early CPools recovery produced core pools eip=0x%08X ped=0x%08X vehicle=0x%08X object=0x%08X colModel=0x%08X",
                            kCColAccelStartCachePoolRead,
                            ped,
                            vehicle,
                            object,
                            colModelPool);
                    }
                    ctx->Eax = colModelPool;
                    return EXCEPTION_CONTINUE_EXECUTION;
                }
            }

            if (ctx->Eax == 0 && EnsureLazyCPoolReady(kOriginalColModelPoolPtr, "CColAccel::startCache")) {
                uint32_t colModelPool = 0;
                SafeReadU32(kOriginalColModelPoolPtr, &colModelPool);
                if (IsValidCPool(colModelPool)) {
                    const LONG logCount = InterlockedIncrement(&g_colAccelStartCachePoolGuardLogs);
                    if (logCount <= 16) {
                        Log("col accel guard: CPools recovery produced real ColModel pool eip=0x%08X pool=0x%08X",
                            kCColAccelStartCachePoolRead,
                            colModelPool);
                    }
                    ctx->Eax = colModelPool;
                    return EXCEPTION_CONTINUE_EXECUTION;
                }
            }

            const uint32_t fallbackSize = static_cast<uint32_t>(
                ReadFlaInt("Collision size", static_cast<int>(ReadIniU32("FILE_TYPE_COL", 500)), 1, 1000000));
            uint32_t originalColPool = 0;
            SafeReadU32(kCColStorePoolPtr, &originalColPool);

            g_colAccelFakePool[0] = 0;
            g_colAccelFakePool[1] = 0;
            g_colAccelFakePool[2] = fallbackSize;
            g_colAccelFakePool[3] = 0;

            const LONG logCount = InterlockedIncrement(&g_colAccelStartCachePoolGuardLogs);
            if (logCount <= 16) {
                Log("col accel guard: supplied fallback CColStore pool for startCache eip=0x%08X oldEax=0x%08X fault=0x%08X originalPoolPtr[0x%08X]=0x%08X fallbackSize=%u fakePool=0x%08X",
                    kCColAccelStartCachePoolRead,
                    ctx->Eax,
                    fault,
                    kCColStorePoolPtr,
                    originalColPool,
                    fallbackSize,
                    reinterpret_cast<uintptr_t>(g_colAccelFakePool));
                LogBytes("col-accel-eip", kCColAccelStartCachePoolRead - 16, 64);
                LogStackModules(ctx->Esp);
            }

            ctx->Eax = reinterpret_cast<DWORD>(g_colAccelFakePool);
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }

    if (g_config.enableGetBoundRectColModelGuard &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(0x00534134) &&
        info->ExceptionRecord->NumberParameters > 1 &&
        static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]) == 0x0) {

        CONTEXT* ctx = info->ContextRecord;
        int32_t modelId = -1;
        if (IsReadableCommitted(ctx->Esi + 0x22, sizeof(uint16_t))) {
            modelId = ReadExtendedIdFrom16BitField(reinterpret_cast<const void*>(ctx->Esi + 0x22));
        }

        static LONG recoverCount = 0;
        const LONG count = InterlockedIncrement(&recoverCount);
        if (count <= 16) {
            uintptr_t returnAddress = 0;
            if (IsReadableCommitted(ctx->Esp, sizeof(returnAddress))) {
                __try {
                    returnAddress = *reinterpret_cast<const uintptr_t*>(ctx->Esp);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    returnAddress = 0;
                }
            }

            Log("bound rect VEH: replaced null CColModel at 0x00534134 model=%u entity=0x%08X fakeCol=0x%08X return=0x%08X esp=0x%08X",
                modelId >= 0 ? static_cast<uint32_t>(modelId) : 0xFFFFFFFFu,
                ctx->Esi,
                reinterpret_cast<uintptr_t>(&g_fakeColModelForBounds),
                returnAddress,
                ctx->Esp);
            LogStackModules(ctx->Esp);
            if (modelId < kOriginalCModelInfoCount) {
                DumpModelContext(modelId);
            }
        }

        if (IsWritableCommitted(ctx->Ecx + 0x14, sizeof(uintptr_t))) {
            const uint32_t fakeCol = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&g_fakeColModelForBounds));
            __try {
                *reinterpret_cast<uint32_t*>(ctx->Ecx + 0x14) = fakeCol;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
            }
        }

        ctx->Eax = reinterpret_cast<DWORD>(&g_fakeColModelForBounds);
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    if (g_config.enableGetBoundRectColModelGuard &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(0x0053414E) &&
        info->ExceptionRecord->NumberParameters > 1 &&
        static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]) == 0x0C) {

        CONTEXT* ctx = info->ContextRecord;
        if (IsWritableCommitted(ctx->Esp + 0x10, sizeof(float) * 3)) {
            __try {
                *reinterpret_cast<float*>(ctx->Esp + 0x10) = g_fakeColModelForBounds.bboxMinX;
                *reinterpret_cast<float*>(ctx->Esp + 0x14) = g_fakeColModelForBounds.bboxMinY;
                *reinterpret_cast<float*>(ctx->Esp + 0x18) = g_fakeColModelForBounds.bboxMinZ;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
            }
        }

        static LONG recoverMaxCount = 0;
        const LONG count = InterlockedIncrement(&recoverMaxCount);
        if (count <= 16) {
            Log("bound rect VEH: recovered bbox max read at 0x0053414E entity=0x%08X fakeCol=0x%08X esp=0x%08X",
                ctx->Esi,
                reinterpret_cast<uintptr_t>(&g_fakeColModelForBounds),
                ctx->Esp);
            LogStackModules(ctx->Esp);
        }

        ctx->Ecx = reinterpret_cast<DWORD>(&g_fakeColModelForBounds) + 0x0C;
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    if (g_config.enableShouldModelBeStreamedGuard &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(0x00554F62) &&
        info->ExceptionRecord->NumberParameters > 1 &&
        info->ExceptionRecord->ExceptionInformation[0] == 0) {

        CONTEXT* ctx = info->ContextRecord;
        Bridge_LogInvalidShouldModelBeStreamedColModel(ctx->Esi, ctx->Edi, ctx->Eax, ctx->Esp);
        ctx->Eip = 0x00554F76;
        return EXCEPTION_CONTINUE_EXECUTION;
    }

#endif

    if (g_config.enableRpAnimBlendClumpInitGuard &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(kRpAnimBlendAllocateDataWrite) &&
        info->ExceptionRecord->NumberParameters > 1 &&
        info->ExceptionRecord->ExceptionInformation[0] == 1) {

        CONTEXT* ctx = info->ContextRecord;
        static LONG recoverCount = 0;
        const LONG count = InterlockedIncrement(&recoverCount);
        if (count <= 16) {
            Log("rp anim allocate VEH: skipped invalid plugin write eip=0x%08X fault=0x%08X eax=0x%08X edx=0x%08X ecx=0x%08X esp=0x%08X",
                ctx->Eip,
                static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]),
                ctx->Eax,
                ctx->Edx,
                ctx->Ecx,
                ctx->Esp);
            LogStackModules(ctx->Esp);
        }

        ctx->Eax = 0;
        ctx->Eip = 0x004D5F72;
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    if ((g_config.enableAnimFrameUpdateGuard || g_config.enableAnimEmptyUpdateGuard) &&
        info->ContextRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        info->ExceptionRecord->ExceptionAddress == reinterpret_cast<void*>(0x004D1710) &&
        info->ExceptionRecord->NumberParameters > 1 &&
        static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]) == 0x10 &&
        info->ContextRecord->Eax == 0) {

        CONTEXT* ctx = info->ContextRecord;
        static LONG recoverCount = 0;
        const LONG count = InterlockedIncrement(&recoverCount);
        if (count <= 4) {
            Log("anim frame VEH: skipped null blend node at 0x004D1710 esp=0x%08X edi=0x%08X esi=0x%08X ecx=0x%08X",
                ctx->Esp,
                ctx->Edi,
                ctx->Esi,
                ctx->Ecx);
        }
        ctx->Eip = 0x004D1A45;
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    const DWORD psCompatCode = info->ExceptionRecord->ExceptionCode;
    if (g_config.enableProperShadersCompat &&
        (psCompatCode == EXCEPTION_ACCESS_VIOLATION ||
            psCompatCode == EXCEPTION_ILLEGAL_INSTRUCTION ||
            psCompatCode == EXCEPTION_PRIV_INSTRUCTION)) {
        const uintptr_t psEip = static_cast<uintptr_t>(info->ContextRecord->Eip);
        const uintptr_t faultAddr = info->ExceptionRecord->NumberParameters > 1 ?
            info->ExceptionRecord->ExceptionInformation[1] : 0;

        bool isPsModule = false;
        if (faultAddr == 0) {
            char modName[MAX_PATH]{};
            ModuleBaseFromAddress(psEip, modName, sizeof(modName));
            isPsModule = (strstr(modName, "propershaders") != nullptr || strstr(modName, "ProperShaders") != nullptr);
        }

        bool isEipNonExec = false;
        if (!isPsModule && psEip > 0x00900000) {
            MEMORY_BASIC_INFORMATION eipMbi{};
            if (VirtualQuery(reinterpret_cast<void*>(psEip), &eipMbi, sizeof(eipMbi))) {
                isEipNonExec = (eipMbi.State == MEM_COMMIT) &&
                    (eipMbi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY)) &&
                    !(eipMbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY));
            }
        }

        if (psEip < 0x10000) {
            static LONG s_lowAddressSkipCount = 0;
            if (InterlockedIncrement(&s_lowAddressSkipCount) <= 6) {
                Log("proper shaders compat: not recovering low-address exception eip=0x%08X fault=0x%08X code=0x%08X",
                    psEip,
                    faultAddr,
                    psCompatCode);
            }
            return EXCEPTION_CONTINUE_SEARCH;
        }

        bool hasPsStackFrame = isPsModule;
        if (!hasPsStackFrame && info->ContextRecord && IsReadableCommitted(info->ContextRecord->Esp, sizeof(uintptr_t))) {
            for (int depth = 0; depth < 16; ++depth) {
                uint32_t candidate = 0;
                if (!SafeReadU32(info->ContextRecord->Esp + depth * 4, &candidate)) {
                    break;
                }
                char mn[MAX_PATH]{};
                ModuleBaseFromAddress(candidate, mn, sizeof(mn));
                if (strstr(mn, "propershaders") != nullptr || strstr(mn, "ProperShaders") != nullptr) {
                    hasPsStackFrame = true;
                    break;
                }
            }
        }

        const bool isJumpToBadAddress = hasPsStackFrame &&
            ((psEip == faultAddr) || (faultAddr == 0 && psEip > 0x60000000) || isPsModule || isEipNonExec);
        if (isJumpToBadAddress) {
            CONTEXT* psCtx = info->ContextRecord;

            static LONG s_dumpCount = 0;
            if (InterlockedIncrement(&s_dumpCount) <= 3) {
                Log("ps watchpoint: unsafe bad jump left to CrashInfo eip=0x%08X fault=0x%08X code=0x%08X esp=0x%08X eax=0x%08X ecx=0x%08X edx=0x%08X esi=0x%08X edi=0x%08X",
                    psEip,
                    faultAddr,
                    psCompatCode,
                    psCtx->Esp,
                    psCtx->Eax,
                    psCtx->Ecx,
                    psCtx->Edx,
                    psCtx->Esi,
                    psCtx->Edi);
                for (int slot = 0; slot < 16; ++slot) {
                    uintptr_t stackVal = 0;
                    if (!SafeReadU32(psCtx->Esp + slot * 4, reinterpret_cast<uint32_t*>(&stackVal))) break;
                    char modName[MAX_PATH]{};
                    uintptr_t modBase = ModuleBaseFromAddress(stackVal, modName, sizeof(modName));
                    if (modBase) {
                        Log("ps watchpoint: stack[+0x%X]=0x%08X -> %s+0x%X",
                            slot * 4, stackVal, modName, stackVal - modBase);
                    }
                }
            }
        }
    }

    if (BreakRepeatedNonExecutableExceptionLoop(info)) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    LogCrashClassification(info);

    if (!g_config.enableExceptionDiagnostics || g_config.maxExceptionLogs <= 0) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    const DWORD code = info->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_ACCESS_VIOLATION && code != EXCEPTION_ILLEGAL_INSTRUCTION &&
        code != EXCEPTION_PRIV_INSTRUCTION && code != EXCEPTION_IN_PAGE_ERROR) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    const LONG count = InterlockedIncrement(&g_exceptionLogCount);
    if (count > g_config.maxExceptionLogs) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    const uintptr_t eip = reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress);
    char moduleName[MAX_PATH]{};
    const uintptr_t moduleBase = ModuleBaseFromAddress(eip, moduleName, sizeof(moduleName));

    Log("exception: code=0x%08lX address=0x%08X module=%s+0x%X info0=0x%X info1=0x%X",
        code,
        eip,
        moduleName,
        moduleBase ? eip - moduleBase : 0,
        info->ExceptionRecord->NumberParameters > 0 ? static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[0]) : 0,
        info->ExceptionRecord->NumberParameters > 1 ? static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]) : 0);

    LogMemoryRegion("exception-address", eip);
    if (eip >= 16) {
        LogBytes("exception-eip", eip - 16, 64);
    }
    if (info->ExceptionRecord->NumberParameters > 1) {
        LogMemoryRegion("fault-address", static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]));
    }

#if defined(_M_IX86)
    CONTEXT* ctx = info->ContextRecord;
    if (ctx) {
        Log("context: eax=0x%08X ebx=0x%08X ecx=0x%08X edx=0x%08X esi=0x%08X edi=0x%08X ebp=0x%08X esp=0x%08X",
            ctx->Eax, ctx->Ebx, ctx->Ecx, ctx->Edx, ctx->Esi, ctx->Edi, ctx->Ebp, ctx->Esp);
        LogMemoryRegion("eax", ctx->Eax);
        LogMemoryRegion("ecx", ctx->Ecx);
        LogMemoryRegion("edx", ctx->Edx);
        LogMemoryRegion("esi", ctx->Esi);
        LogDwords("eax-dwords", ctx->Eax, 24);
        LogDwords("ecx-dwords", ctx->Ecx, 16);
        LogDwords("edx-dwords", ctx->Edx, 16);
        LogDwords("esi-dwords", ctx->Esi, 24);
        if (ctx->Edi < 20000) {
            DumpModelContext(ctx->Edi);
        }
        LogStackModules(ctx->Esp);
    }
#endif

    return EXCEPTION_CONTINUE_SEARCH;
}

DWORD WINAPI BridgeThread(void*)
{
    InitializeBridgeFilePaths();
    LoadBridgeConfig();
    char configProbe[32]{};
    const bool configReadable = ReadSmallTextValue(g_configPath, "EnableBridge", configProbe, sizeof(configProbe));
    const DWORD detectedGameThreadId = FindProcessMainThreadId();
    if (detectedGameThreadId) {
        g_gameThreadId = detectedGameThreadId;
    }
    Log("thread init: bridge=%lu game=%lu detected=%lu",
        GetCurrentThreadId(), g_gameThreadId, detectedGameThreadId);
    Log("config source: path=%s readable=%d probe='%s' openRetries=%ld",
        g_configPath,
        configReadable ? 1 : 0,
        configReadable ? configProbe : "",
        g_bridgeConfigOpenRetries);
    if (g_config.enableVectoredExceptionHandler && !g_vectoredExceptionHandlerHandle) {
        g_vectoredExceptionHandlerHandle = AddVectoredExceptionHandler(1, BridgeVectoredExceptionHandler);
        if (!g_vectoredExceptionHandlerHandle) {
            Log("VEH init: AddVectoredExceptionHandler failed gle=%lu", GetLastError());
        }
    }
    InstallCPoolsInitialiseReplayHook();
    StartVehFuncsPoolAllocateGuardInstaller();
    GuardOpenLimitAdjusterModuleLoad("bridge-init");
    GuardOpenLimitAdjusterSaLimits("bridge-init");
    if (g_config.enableProperShadersCompat) {
        HANDLE earlyThread = CreateThread(nullptr, 0, EarlyProperShadersCompatThread, nullptr, 0, nullptr);
        if (earlyThread) {
            CloseHandle(earlyThread);
        } else {
            Log("early proper shaders compat: thread creation failed gle=%lu", GetLastError());
        }
    }

    Sleep(1500);
    InstallWidescreenFixSpriteNameGuard();
    if (g_config.enableProperShadersCompat) {
        InstallProperShadersVtableGuard();
        InstallRwTexDictionaryFindNamedTextureGuard();
        InstallTxdLoadDictionaryWriteGuard();
    }
    Log("%s v%s loaded (runtime/API name: FLACompatBridge, API=%u)",
        kDisplayName, kProductVersion, kApiVersion);
    LogBridgeConfig();
    Log("loader: DINPUT8=%p DINPUT8Hooked=%p vorbisFile=%p vorbisHooked=%p",
        GetModuleHandleA("DINPUT8.dll"),
        GetModuleHandleA("DINPUT8Hooked.dll"),
        GetModuleHandleA("vorbisFile.dll"),
        GetModuleHandleA("vorbisHooked.dll"));
    AuditOpenLimitAdjusterSaOverlaps();

    LogIniValue("Apply ID limit patch");
    LogIniValue("FILE_TYPE_DFF");
    LogIniValue("FILE_TYPE_TXD");
    LogIniValue("FILE_TYPE_COL");
    LogIniValue("FILE_TYPE_IPL");
    LogIniValue("FILE_TYPE_IFP");
    LogIniValue("FILE_TYPE_RRR");
    LogIniValue("FILE_TYPE_SCM");
    LogIniValue("PtrNode Singles");
    LogIniValue("PtrNode Doubles");
    LogIniValue("EntryInfoNodes");
    LogIniValue("Count of killable model IDs");
    LogFlaPathNodeDiagnostics();
    InstallStreamingBusyThresholdPatch();
    InstallPopulationUpdateBudgetPatch();

    LogFLAExports();
    RefreshFlaRuntimeState();
    StartFlaRuntimeStateRecovery();
    RefreshRadarTraceRuntimeState();
    if (g_config.enableProperShadersCompat) {
        ApplyProperShadersCompat();
    }
    if (g_config.enableFlaTrainInitHookRepair) {
        InstallFlaTrainInitHookRepair();
    }
    LogRelocatedAddressDiagnostics();
    LogPoolPointerDiagnostics();
    if (g_config.enablePopulationPoolDiagnostics) {
        HANDLE populationDiagThread = CreateThread(nullptr, 0, PopulationPoolDiagnosticsThread, nullptr, 0, nullptr);
        if (populationDiagThread) {
            CloseHandle(populationDiagThread);
        } else {
            Log("population diag: thread creation failed gle=%lu", GetLastError());
        }
    }
    if (g_config.enableGangOnlyPopulationGuard) {
        HANDLE populationGuardThread = CreateThread(nullptr, 0, GangOnlyPopulationGuardThread, nullptr, 0, nullptr);
        if (populationGuardThread) {
            CloseHandle(populationGuardThread);
        } else {
            Log("population guard: thread creation failed gle=%lu", GetLastError());
        }
    }
    if (g_config.enablePedStreamingZoneRepair) {
        HANDLE pedZoneRepairThread = CreateThread(nullptr, 0, PedStreamingZoneRepairThread, nullptr, 0, nullptr);
        if (pedZoneRepairThread) {
            CloseHandle(pedZoneRepairThread);
        } else {
            Log("ped zone repair: thread creation failed gle=%lu", GetLastError());
        }
    }
    RuntimeRewriteHardcodedAddressConstants();
    if (g_config.enableProperShadersCompat) {
        ApplyProperShadersCompat();
    }
    if (g_config.enableRuntimeRewriteRescan) {
        HANDLE rewriteThread = CreateThread(nullptr, 0, RuntimeRewriteRescanThread, nullptr, 0, nullptr);
        if (rewriteThread) {
            CloseHandle(rewriteThread);
        } else {
            Log("runtime rewrite rescan: thread creation failed gle=%lu", GetLastError());
        }
    }
    if (g_config.enableModuleSnapshot) {
        LogModuleSnapshot();
    }
    if (g_config.enableFlaNoCollisionErrorRestore) {
        RestoreFlaNoCollisionErrorPatch();
    }
    if (g_config.enableGetBoundRectColModelGuard) {
        InstallGetBoundRectColModelGuard();
    }
    if (g_config.enableShouldModelBeStreamedGuard) {
        InstallShouldModelBeStreamedGuard();
    }
    if (g_config.enableBridgeCheatStringLoader) {
        LoadBridgeCheatStrings();
    }
    if (g_config.enableFlaObjectInitCollisionRestore) {
        RestoreFlaObjectInitCollisionPatch();
    }
    if (g_config.enablePickupModelLoadGuard) {
        InstallPickupModelLoadGuard();
    }
    if (g_config.enableRadarBlipHandleGuard) {
        InstallRadarBlipHandleGuard();
    }
    if (g_config.enableClosestCarNode03D3Fallback) {
        InstallClosestCarNode03D3Fallback();
    }
    if (g_config.enableTaxi77SetCarCoordinatesGuard) {
        InstallTaxi77SetCarCoordinatesGuard();
    }
    if (g_config.enableSanPabloSpecialActorBridge) {
        InstallSanPabloSpecialActorBridge();
    }
    if (g_config.enableTaxi77StateWatchdog) {
        HANDLE taxi77WatchdogThread = CreateThread(nullptr, 0, Taxi77StateWatchdogThread, nullptr, 0, nullptr);
        if (taxi77WatchdogThread) {
            CloseHandle(taxi77WatchdogThread);
        } else {
            Log("taxi77 watchdog: thread creation failed gle=%lu", GetLastError());
        }
    }
    if (g_config.enableCObjectCreateBridge) {
        InstallCObjectCreateBridgeStubs();
    }
    if (g_config.enableCleoObjectCreateInlineRestore) {
        RestoreCleoObjectCreateInlinePatch();
    }
    if (g_config.enableCleoDispatchGuard) {
        InstallCleoDispatchNullGuard();
    }
    if (g_config.enableCleoThunk26720Guard) {
        InstallCleoThunk26720NullGuard();
    }
    if (g_config.enableCleoPlusPoolAllocateGuard) {
        InstallCleoPlusPoolAllocateGuard();
    }
    if (g_config.enableMixSetsPoolAllocateGuard) {
        InstallMixSetsPoolAllocateGuard();
        HANDLE mixSetsPoolGuardThread = CreateThread(nullptr, 0, MixSetsPoolAllocateGuardInstallThread, nullptr, 0, nullptr);
        if (mixSetsPoolGuardThread) {
            CloseHandle(mixSetsPoolGuardThread);
        } else {
            Log("MixSets pool allocate guard: delayed install thread creation failed gle=%lu", GetLastError());
        }
    }
    if (g_config.enableUrbanizePoolAllocateGuard) {
        InstallUrbanizePoolAllocateGuard();
        HANDLE urbanizePoolGuardThread = CreateThread(nullptr, 0, UrbanizePoolAllocateGuardInstallThread, nullptr, 0, nullptr);
        if (urbanizePoolGuardThread) {
            CloseHandle(urbanizePoolGuardThread);
        } else {
            Log("Urbanize pool allocate guard: delayed install thread creation failed gle=%lu", GetLastError());
        }
    }
    if (g_config.enableVehFuncsPoolAllocateGuard) {
        StartVehFuncsPoolAllocateGuardInstaller();
    }
    if (g_config.enableAutoPoolAllocateGuard) {
        InstallAutoPoolAllocateGuards();
    }
    if (g_config.enableCleoPlusExtendedObjectVarGuard) {
        InstallCleoPlusExtendedObjectVarGuard();
    }
    if (g_config.enableAnimUncompressGuard) {
        InstallAnimUncompressNullGuard();
    }
    if (g_config.enableAnimStaticAssocGuard) {
        InstallAnimStaticAssocInitGuard();
        InstallAnimUpdateBlendGuard();
        InstallAnimBlendGroupGuard();
    }
    if (g_config.enableAnimFrameUpdateGuard) {
        InstallAnimFrameUpdateSkinnedGuard();
        InstallAnimFrameUpdateSkinnedVelocityGuard();
    }
    if (g_config.enableAnimEmptyUpdateGuard) {
        InstallAnimEmptyUpdateGuard();
    }
    if (g_config.enableAnimLifecycleDiagnostics) {
        InstallAnimLifecycleDiagnostics();
    }
    if (g_config.enableRpAnimBlendClumpInitGuard) {
        InstallRpAnimBlendClumpInitGuard();
    }
    if (g_config.enableRwClumpForAllAtomicsGuard) {
        InstallRwClumpForAllAtomicsGuard();
    }
    if (g_config.enableGetBoundCentreInlineGuard) {
        InstallGetBoundCentreNullGuard();
    }

    if (g_config.enableLegacyModelInfoShadow || g_config.enableLegacyStreamingInfoShadow) {
        HANDLE shadowThread = CreateThread(nullptr, 0, LegacyShadowThread, nullptr, 0, nullptr);
        if (shadowThread) {
            CloseHandle(shadowThread);
        }
    }
    if (g_config.enableUrbanizeProblemPedPreload) {
        HANDLE preloadThread = CreateThread(nullptr, 0, PreloadUrbanizePedModelsThread, nullptr, 0, nullptr);
        if (preloadThread) {
            CloseHandle(preloadThread);
        }
    }
    if (g_config.enableSanPabloSpecialActorBridge &&
        (g_config.specialActorAutoScanImgArchives || g_config.specialActorAutoScanModloader)) {
        HANDLE catalogThread = CreateThread(nullptr, 0, SpecialActorRuntimeCatalogThread, nullptr, 0, nullptr);
        if (catalogThread) {
            CloseHandle(catalogThread);
        } else {
            Log("special actor catalog: thread creation failed gle=%lu", GetLastError());
        }
    }
    return 0;
}

extern "C" __declspec(dllexport) uint32_t __stdcall FLACompatBridge_GetApiVersion()
{
    return kApiVersion;
}

extern "C" __declspec(dllexport) uint32_t __stdcall FLACompatBridge_GetFileIdCapacity()
{
    if (!g_fileIdCapacity) {
        RefreshFlaRuntimeState();
    }
    return g_fileIdCapacity;
}

extern "C" __declspec(dllexport) uint32_t __stdcall FLACompatBridge_GetCompatFlags()
{
    if (!g_fileIdCapacity && !g_relocatedCModelInfoPtrs && !g_relocatedStreamingInfo) {
        RefreshFlaRuntimeState();
    }
    return g_flaCompatFlags;
}

extern "C" __declspec(dllexport) uint32_t __stdcall FLACompatBridge_GetRuntimeSource()
{
    if (!g_fileIdCapacity && !g_relocatedCModelInfoPtrs && !g_relocatedStreamingInfo) {
        RefreshFlaRuntimeState();
    }
    return g_runtimeStateSource;
}

extern "C" __declspec(dllexport) uintptr_t __stdcall FLACompatBridge_GetRelocatedAddress(uint32_t id)
{
    if (!g_fileIdCapacity && !g_relocatedCModelInfoPtrs && !g_relocatedStreamingInfo) {
        RefreshFlaRuntimeState();
    }
    if (!g_radarTraceBase) {
        RefreshRadarTraceRuntimeState();
    }

    switch (id) {
    case 1:
        return g_relocatedCModelInfoPtrs;
    case 2:
        return g_relocatedStreamingInfo;
    case 3:
        return g_relocatedAnimBlocks;
    case 4:
        return g_relocatedVehicleRecordingStreamingArray;
    case 5:
        return g_relocatedStreamedScripts;
    case 6:
        return g_relocatedHandlingManager;
    case 7:
        return g_radarTraceBase;
    case 8:
        return g_relocatedStreamingInfoExtension;
    case 9:
        return g_relocatedRegisteredKills;
    default:
        return 0;
    }
}

extern "C" __declspec(dllexport) int __stdcall FLACompatBridge_GetRelocatedAddressByName(const char* name, uintptr_t* out)
{
    if (!name || !out || !IsWritableCommitted(reinterpret_cast<uintptr_t>(out), sizeof(*out))) {
        return 0;
    }

    if (!g_fileIdCapacity && !g_relocatedCModelInfoPtrs && !g_relocatedStreamingInfo) {
        RefreshFlaRuntimeState();
    }
    if (!g_radarTraceBase) {
        RefreshRadarTraceRuntimeState();
    }

    uintptr_t value = 0;
    if (_stricmp(name, "CModelInfo::ms_modelInfoPtrs") == 0 || _stricmp(name, "CModelInfo") == 0) {
        value = g_relocatedCModelInfoPtrs;
    } else if (_stricmp(name, "CStreaming::ms_aInfoForModel") == 0 || _stricmp(name, "CStreaming") == 0) {
        value = g_relocatedStreamingInfo;
    } else if (_stricmp(name, "CStreaming::ms_aInfoForModelExtension") == 0 || _stricmp(name, "CStreamingExtension") == 0) {
        value = g_relocatedStreamingInfoExtension;
    } else if (_stricmp(name, "CAnimManager::ms_aAnimBlocks") == 0 || _stricmp(name, "AnimBlocks") == 0) {
        value = g_relocatedAnimBlocks;
    } else if (_stricmp(name, "CVehicleRecording::StreamingArray") == 0 || _stricmp(name, "VehicleRecording") == 0) {
        value = g_relocatedVehicleRecordingStreamingArray;
    } else if (_stricmp(name, "CTheScripts::StreamedScripts") == 0 || _stricmp(name, "StreamedScripts") == 0) {
        value = g_relocatedStreamedScripts;
    } else if (_stricmp(name, "mod_HandlingManager") == 0 || _stricmp(name, "HandlingManager") == 0) {
        value = g_relocatedHandlingManager;
    } else if (_stricmp(name, "CDarkel::RegisteredKills") == 0 || _stricmp(name, "RegisteredKills") == 0) {
        value = g_relocatedRegisteredKills;
    } else if (_stricmp(name, "CRadar::ms_RadarTrace") == 0 || _stricmp(name, "RadarTrace") == 0) {
        value = g_radarTraceBase;
    }

    *out = value;
    return value ? 1 : 0;
}

extern "C" __declspec(dllexport) uintptr_t __stdcall FLACompatBridge_GetModelInfoEntry(uint32_t modelId)
{
    if (!g_fileIdCapacity) {
        RefreshFlaRuntimeState();
    }
    return SafeModelInfoEntryAddress(modelId);
}

extern "C" __declspec(dllexport) uintptr_t __stdcall FLACompatBridge_GetModelInfo(uint32_t modelId)
{
    if (!g_fileIdCapacity) {
        RefreshFlaRuntimeState();
    }

    const uintptr_t entry = SafeModelInfoEntryAddress(modelId);
    uint32_t modelInfo = 0;
    return entry && SafeReadU32(entry, &modelInfo) ? modelInfo : 0;
}

extern "C" __declspec(dllexport) uintptr_t __stdcall FLACompatBridge_GetStreamingInfo(uint32_t modelId)
{
    if (!g_fileIdCapacity) {
        RefreshFlaRuntimeState();
    }
    return SafeStreamingInfoEntryAddress(modelId);
}

extern "C" __declspec(dllexport) int __stdcall FLACompatBridge_ReadStreamingInfo(uint32_t modelId, void* out, uint32_t outSize)
{
    if (!out || outSize < kStreamingInfoSize ||
        !IsWritableCommitted(reinterpret_cast<uintptr_t>(out), kStreamingInfoSize)) {
        return 0;
    }

    if (!g_fileIdCapacity) {
        RefreshFlaRuntimeState();
    }

    const uintptr_t entry = SafeStreamingInfoEntryAddress(modelId);
    if (!entry) {
        return 0;
    }

    __try {
        std::memcpy(out, reinterpret_cast<const void*>(entry), kStreamingInfoSize);
        return 1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

extern "C" __declspec(dllexport) int __stdcall FLACompatBridge_GetPoolInfo(uint32_t poolId, uintptr_t* poolPtr, uint32_t* capacity)
{
    if (!poolPtr || !capacity ||
        !IsWritableCommitted(reinterpret_cast<uintptr_t>(poolPtr), sizeof(*poolPtr)) ||
        !IsWritableCommitted(reinterpret_cast<uintptr_t>(capacity), sizeof(*capacity))) {
        return 0;
    }

    if (!g_fileIdCapacity) {
        RefreshFlaRuntimeState();
    }

    uintptr_t ptrAddress = 0;
    uint32_t cap = 0;
    switch (poolId) {
    case 1:
        ptrAddress = kOriginalPedPoolPtr;
        cap = g_pedPoolCapacity;
        break;
    case 2:
        ptrAddress = kOriginalVehiclePoolPtr;
        cap = g_vehiclePoolCapacity;
        break;
    case 3:
        ptrAddress = kOriginalObjectPoolPtr;
        cap = g_objectPoolCapacity;
        break;
    case 4:
        ptrAddress = kOriginalBuildingPoolPtr;
        cap = g_buildingPoolCapacity;
        break;
    case 5:
        ptrAddress = kOriginalDummyPoolPtr;
        cap = g_dummyPoolCapacity;
        break;
    case 6:
        ptrAddress = kOriginalColModelPoolPtr;
        cap = g_colModelPoolCapacity;
        break;
    case 7:
        ptrAddress = kCColStorePoolPtr;
        cap = g_collisionStoreCapacity;
        break;
    default:
        return 0;
    }

    uint32_t ptr = 0;
    if (!SafeReadU32(ptrAddress, &ptr) || !ptr) {
        return 0;
    }

    *poolPtr = ptr;
    *capacity = cap;
    return 1;
}

extern "C" __declspec(dllexport) int __stdcall FLACompatBridge_GetPoolInfoByName(const char* name, uintptr_t* poolPtr, uint32_t* capacity)
{
    if (!name || !poolPtr || !capacity ||
        !IsWritableCommitted(reinterpret_cast<uintptr_t>(poolPtr), sizeof(*poolPtr)) ||
        !IsWritableCommitted(reinterpret_cast<uintptr_t>(capacity), sizeof(*capacity))) {
        return 0;
    }

    char localName[96]{};
    __try {
        strncpy_s(localName, name, _TRUNCATE);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }

    uint32_t poolId = 0;
    if (_stricmp(localName, "Ped") == 0 || _stricmp(localName, "Peds") == 0 || _stricmp(localName, "CPools::ms_pPedPool") == 0) {
        poolId = 1;
    } else if (_stricmp(localName, "Vehicle") == 0 || _stricmp(localName, "Vehicles") == 0 || _stricmp(localName, "CPools::ms_pVehiclePool") == 0) {
        poolId = 2;
    } else if (_stricmp(localName, "Object") == 0 || _stricmp(localName, "Objects") == 0 || _stricmp(localName, "CPools::ms_pObjectPool") == 0) {
        poolId = 3;
    } else if (_stricmp(localName, "Building") == 0 || _stricmp(localName, "Buildings") == 0 || _stricmp(localName, "CPools::ms_pBuildingPool") == 0) {
        poolId = 4;
    } else if (_stricmp(localName, "Dummy") == 0 || _stricmp(localName, "Dummies") == 0 || _stricmp(localName, "CPools::ms_pDummyPool") == 0) {
        poolId = 5;
    } else if (_stricmp(localName, "ColModel") == 0 || _stricmp(localName, "ColModels") == 0 || _stricmp(localName, "CPools::ms_pColModelPool") == 0) {
        poolId = 6;
    } else if (_stricmp(localName, "ColStore") == 0 || _stricmp(localName, "Collision") == 0 || _stricmp(localName, "CColStore::ms_pColPool") == 0) {
        poolId = 7;
    }

    return poolId ? FLACompatBridge_GetPoolInfo(poolId, poolPtr, capacity) : 0;
}

extern "C" __declspec(dllexport) uint32_t __stdcall FLACompatBridge_RebuildSpecialActorCatalog()
{
    LoadBridgeConfig();
    BuildSpecialActorRuntimeCatalog();
    return g_runtimeSpecialActorNameCount;
}

extern "C" __declspec(dllexport) int __stdcall FLACompatBridge_GetSpecialActorName(uint32_t actorCode, char* out, uint32_t outSize)
{
    if (!out || outSize == 0 || !IsWritableCommitted(reinterpret_cast<uintptr_t>(out), outSize)) {
        return 0;
    }

    if (!g_runtimeSpecialActorNameCount) {
        BuildSpecialActorRuntimeCatalog();
    }

    out[0] = '\0';
    return ReadConfiguredSpecialActorName(0, actorCode, out, outSize) ? 1 : 0;
}

extern "C" __declspec(dllexport) int __stdcall FLACompatBridge_RequestSpecialActor(uint32_t modelId, const char* name)
{
    if (!name || !IsReadableCommitted(reinterpret_cast<uintptr_t>(name), 1)) {
        return 0;
    }

    char raw[64]{};
    __try {
        strncpy_s(raw, name, _TRUNCATE);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }

    char normalized[64]{};
    if (!NormalizeSpecialActorName(raw, normalized, sizeof(normalized))) {
        return 0;
    }

    return RequestSpecialActorName(modelId, normalized) ? 1 : 0;
}

extern "C" __declspec(dllexport) int __stdcall FLACompatBridge_RequestSpecialActorByCode(uint32_t modelId, uint32_t actorCode)
{
    if (!g_runtimeSpecialActorNameCount) {
        BuildSpecialActorRuntimeCatalog();
    }

    return RequestConfiguredSpecialActor(modelId, actorCode) ? 1 : 0;
}

extern "C" __declspec(dllexport) int __stdcall FLACompatBridge_ReleaseSpecialActor(uint32_t modelId)
{
    return ReleaseConfiguredSpecialActor(modelId) ? 1 : 0;
}

extern "C" __declspec(dllexport) int __stdcall FLACompatBridge_IsModelLoaded(uint32_t modelId)
{
    if (!g_fileIdCapacity) {
        RefreshFlaRuntimeState();
    }
    return GetStreamingLoadState(modelId) == 1 ? 1 : 0;
}

extern "C" __declspec(dllexport) void FLACompatBridge_KeepExport()
{
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        g_gameThreadId = GetCurrentThreadId();
        InitializeCriticalSection(&g_deferredPoolAllocateLock);
        HANDLE thread = CreateThread(nullptr, 0, BridgeThread, nullptr, 0, nullptr);
        if (thread) {
            CloseHandle(thread);
        }
    } else if (reason == DLL_PROCESS_DETACH && reserved == nullptr) {
        if (g_vectoredExceptionHandlerHandle) {
            RemoveVectoredExceptionHandler(g_vectoredExceptionHandlerHandle);
            g_vectoredExceptionHandlerHandle = nullptr;
        }
    }
    return TRUE;
}

