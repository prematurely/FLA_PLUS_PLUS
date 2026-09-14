#include "FLACompatBridgeInternal.h"

bool ShouldLogSanPabloSpecialActorBridge(long count, int mode, int result)
{
    const LONG state = static_cast<LONG>(
        ((static_cast<uint32_t>(mode) & 0xFFFFu) << 16) |
        (static_cast<uint32_t>(result) & 0xFFFFu));
    const LONG previous = InterlockedExchange(&g_sanPabloSpecialActorBridgeLastLogState, state);
    return count <= 32 || previous != state || (count % 1200) == 0;
}

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_Cleo_Dispatch_NullGuard()
{
    __asm
    {
        push ecx
        push ecx
        call Bridge_GetSafeCleoDispatchTarget
        pop ecx
        test eax, eax
        jz done
        push ecx
        push eax
        call Bridge_CallCleoDispatchTargetSafely

    done:
        push dword ptr [g_cleoDispatchNullGuardReturn]
        retn
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_Cleo_Thunk26720_NullGuard()
{
    __asm
    {
        mov eax, [ecx + 4]
        test eax, eax
        jz done

        mov eax, [eax + 0x18]
        test eax, eax
        jz done

        push ecx
        push edx
        push eax
        call Bridge_GetSafeCleoThunkTarget
        pop edx
        pop ecx
        test eax, eax
        jz done

        jmp eax

    done:
        xor eax, eax
        retn
    }
}
#endif

uintptr_t ResolveCleoThunk26720FinalTarget(uintptr_t dispatchObject, uintptr_t thunkTarget, bool logInvalid)
{
    HMODULE cleoPlus = GetModuleHandleA("CLEO+.cleo");
    const uintptr_t cleoBase = reinterpret_cast<uintptr_t>(cleoPlus);
    if (!cleoBase || thunkTarget != cleoBase + 0x26720) {
        return thunkTarget;
    }

    uintptr_t innerObject = 0;
    uintptr_t finalTarget = 0;
    if (IsReadableCommitted(dispatchObject + 0x04, sizeof(uintptr_t))) {
        __try {
            innerObject = *reinterpret_cast<const uintptr_t*>(dispatchObject + 0x04);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            innerObject = 0;
        }
    }

    if (innerObject && IsReadableCommitted(innerObject + 0x18, sizeof(uintptr_t))) {
        __try {
            finalTarget = *reinterpret_cast<const uintptr_t*>(innerObject + 0x18);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            finalTarget = 0;
        }
    }

    if (finalTarget && IsExecutableCommitted(finalTarget)) {
        return finalTarget;
    }

    if (logInvalid) {
        static LONG logCount = 0;
        const LONG count = InterlockedIncrement(&logCount);
        if (count <= 6) {
            Log("CLEO+ dispatch guard: skipped thunk26720 final target object=0x%08X inner=0x%08X thunk=0x%08X final=0x%08X executable=%d",
                dispatchObject,
                innerObject,
                thunkTarget,
                finalTarget,
                finalTarget ? (IsExecutableCommitted(finalTarget) ? 1 : 0) : 0);
            if (count <= 2) {
                LogMemoryRegion("cleo-thunk26720-object", dispatchObject);
                LogMemoryRegion("cleo-thunk26720-inner", innerObject);
                LogMemoryRegion("cleo-thunk26720-final", finalTarget);
            }
        }
    }

    return 0;
}

bool IsWritableVectorArray(uintptr_t address, size_t count)
{
    return IsWritableCommitted(address, count * 3 * sizeof(uintptr_t));
}

bool LooksLikeStdVectorTriplet(uintptr_t first, uintptr_t last, uintptr_t end)
{
    if (!first && !last && !end) {
        return true;
    }
    if (!first || !last || !end || first > last || last > end) {
        return false;
    }
    if ((last - first) > 4096 * sizeof(uintptr_t) || (end - first) > 8192 * sizeof(uintptr_t)) {
        return false;
    }
    return IsReadableCommitted(first, last > first ? last - first : sizeof(uintptr_t));
}

bool ClearCleoPlusScriptEventsAt(uintptr_t eventsBase, const char* reason)
{
    constexpr size_t kScriptEventLists = 17;
    if (!IsWritableVectorArray(eventsBase, kScriptEventLists)) {
        return false;
    }

    uintptr_t* vectors = reinterpret_cast<uintptr_t*>(eventsBase);
    for (size_t i = 0; i < kScriptEventLists; ++i) {
        uintptr_t first = 0;
        uintptr_t last = 0;
        uintptr_t end = 0;
        __try {
            first = vectors[i * 3 + 0];
            last = vectors[i * 3 + 1];
            end = vectors[i * 3 + 2];
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
        if (!LooksLikeStdVectorTriplet(first, last, end)) {
            return false;
        }
    }

    size_t cleared = 0;
    __try {
        for (size_t i = 0; i < kScriptEventLists; ++i) {
            uintptr_t first = vectors[i * 3 + 0];
            uintptr_t last = vectors[i * 3 + 1];
            if (last != first) {
                ++cleared;
            }
            vectors[i * 3 + 1] = first;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    Log("CLEO+ scriptEvents recovery: cleared=%u base=0x%08X reason=%s",
        static_cast<unsigned>(cleared),
        eventsBase,
        reason ? reason : "");
    return true;
}

void ClearCleoPlusScriptEvents(const char* reason)
{
    HMODULE cleoPlus = GetModuleHandleA("CLEO+.cleo");
    const uintptr_t base = reinterpret_cast<uintptr_t>(cleoPlus);
    if (!base) {
        return;
    }

    static LONG clearCount = 0;
    const LONG count = InterlockedIncrement(&clearCount);
    if (count > 16) {
        return;
    }

    // Known offsets for the current restored CLEO+ build and the local rebuilt variant.
    constexpr uintptr_t kCandidateOffsets[] = { 0x000B7FC8, 0x0008E7E8 };
    for (uintptr_t offset : kCandidateOffsets) {
        if (ClearCleoPlusScriptEventsAt(base + offset, reason)) {
            return;
        }
    }

    Log("CLEO+ scriptEvents recovery: no valid vector array found reason=%s", reason ? reason : "");
}

extern "C" uintptr_t __stdcall Bridge_GetSafeCleoThunkTarget(uintptr_t target)
{
    if (target && IsExecutableCommitted(target)) {
        return target;
    }

    static LONG logCount = 0;
    const LONG count = InterlockedIncrement(&logCount);
    if (count <= 32) {
        char moduleName[MAX_PATH]{};
        const uintptr_t moduleBase = ModuleBaseFromAddress(target, moduleName, sizeof(moduleName));
        Log("CLEO+ thunk26720 guard: skipped invalid target=0x%08X module=%s+0x%X executable=%d",
            target,
            moduleName,
            moduleBase ? target - moduleBase : 0,
            target ? (IsExecutableCommitted(target) ? 1 : 0) : 0);
        LogMemoryRegion("cleo-thunk26720-target", target);
    }

    return 0;
}

extern "C" uintptr_t __stdcall Bridge_GetSafeCleoDispatchTarget(uintptr_t dispatchObject)
{
    uintptr_t vtable = 0;
    uintptr_t target = 0;

    if (IsReadableCommitted(dispatchObject, sizeof(uintptr_t))) {
        __try {
            vtable = *reinterpret_cast<const uintptr_t*>(dispatchObject);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            vtable = 0;
        }
    }

    if (vtable && IsReadableCommitted(vtable + 0x08, sizeof(uintptr_t))) {
        __try {
            target = *reinterpret_cast<const uintptr_t*>(vtable + 0x08);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            target = 0;
        }
    }

    target = ResolveCleoThunk26720FinalTarget(dispatchObject, target, true);

    if (target && IsExecutableCommitted(target)) {
        return target;
    }

    static LONG logCount = 0;
    const LONG count = InterlockedIncrement(&logCount);
    if (count <= 6) {
        Log("CLEO+ dispatch guard: skipped invalid dispatch object=0x%08X vtable=0x%08X target=0x%08X executable=%d",
            dispatchObject,
            vtable,
            target,
            target ? (IsExecutableCommitted(target) ? 1 : 0) : 0);
        if (count <= 2) {
            LogMemoryRegion("cleo-dispatch-object", dispatchObject);
            LogMemoryRegion("cleo-dispatch-vtable", vtable);
            LogMemoryRegion("cleo-dispatch-target", target);
        }
    }

    return 0;
}

int __stdcall Bridge_CleoDispatchExceptionFilter(EXCEPTION_POINTERS* info, uintptr_t target, uintptr_t dispatchObject)
{
    static LONG logCount = 0;
    const LONG count = InterlockedIncrement(&logCount);
    if (count <= 32) {
        const uintptr_t eip = info && info->ExceptionRecord
            ? reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress)
            : 0;
        const uintptr_t fault = info && info->ExceptionRecord && info->ExceptionRecord->NumberParameters > 1
            ? static_cast<uintptr_t>(info->ExceptionRecord->ExceptionInformation[1])
            : 0;

        char moduleName[MAX_PATH]{};
        const uintptr_t moduleBase = ModuleBaseFromAddress(eip, moduleName, sizeof(moduleName));
        Log("CLEO+ dispatch guard: swallowed callback exception target=0x%08X object=0x%08X eip=0x%08X %s+0x%X fault=0x%08X",
            target,
            dispatchObject,
            eip,
            moduleName,
            moduleBase ? eip - moduleBase : 0,
            fault);
#if defined(_M_IX86)
        if (info && info->ContextRecord) {
            Log("CLEO+ dispatch guard context: eax=0x%08X ecx=0x%08X edx=0x%08X esi=0x%08X edi=0x%08X esp=0x%08X",
                info->ContextRecord->Eax,
                info->ContextRecord->Ecx,
                info->ContextRecord->Edx,
                info->ContextRecord->Esi,
                info->ContextRecord->Edi,
                info->ContextRecord->Esp);
            LogStackModules(info->ContextRecord->Esp);
        }
#endif
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

extern "C" void __stdcall Bridge_CallCleoDispatchTargetSafely(uintptr_t target, uintptr_t dispatchObject)
{
    if (!target || !IsExecutableCommitted(target)) {
        return;
    }

    if (target == kCPoolsInitialise) {
        Log("CLEO+ dispatch guard: blocked invalid dispatch target=CPools::Initialise object=0x%08X batchLazyCPools=%d lazyPoolRecovery=%d",
            dispatchObject,
            g_config.enableBatchLazyCPoolInitialise ? 1 : 0,
            g_config.enableCleoDispatchLazyPoolRecovery ? 1 : 0);
        ClearCleoPlusScriptEvents("invalid dispatch target CPools::Initialise");
        if (g_config.enableCleoDispatchLazyPoolRecovery) {
            if (g_config.enableBatchLazyCPoolInitialise) {
                EnsureBatchLazyCPoolsInitialised("blocked CLEO+ dispatch target CPools::Initialise", true);
            } else {
                EnsureLazyCoreCPoolsReady("blocked CLEO+ dispatch target CPools::Initialise");
            }
        } else {
            Log("CLEO+ dispatch guard: cleared stale CPools::Initialise dispatch without lazy pool recovery");
        }
        return;
    }

    __try {
        using DispatchFn = void(__thiscall*)(void*);
        reinterpret_cast<DispatchFn>(target)(reinterpret_cast<void*>(dispatchObject));
    }
    __except (Bridge_CleoDispatchExceptionFilter(GetExceptionInformation(), target, dispatchObject)) {
    }
}

CleoExports::OpcodeResult CallCleoPlusExtendedObjectVarSafely(
    const char* name,
    uintptr_t trampoline,
    RunningScriptLite* thread)
{
    if (!trampoline || !IsExecutableCommitted(trampoline)) {
        UpdateScriptCompareFlag(thread, false);
        return kCleoOpcodeResultContinue;
    }

    __try {
        return reinterpret_cast<CleoOpcodeTrampolineFn>(trampoline)(thread);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        static LONG logCount = 0;
        const LONG count = InterlockedIncrement(&logCount);
        if (count <= 64) {
            const uint32_t offset = GetCleoScriptOffset(thread);
            Log("CLEO+ extended object var guard: swallowed exception opcode=%s script='%.*s' offset=0x%X trampoline=0x%08X",
                name ? name : "",
                thread ? 8 : 0,
                thread ? thread->name : "",
                offset,
                trampoline);
        }
        UpdateScriptCompareFlag(thread, false);
        return kCleoOpcodeResultContinue;
    }
}

extern "C" CleoExports::OpcodeResult __stdcall Bridge_CleoPlus_InitExtendedObjectVars_Guard(RunningScriptLite* thread)
{
    return CallCleoPlusExtendedObjectVarSafely(
        "INIT_EXTENDED_OBJECT_VARS",
        g_cleoPlusInitExtendedObjectVarsTrampoline,
        thread);
}

extern "C" CleoExports::OpcodeResult __stdcall Bridge_CleoPlus_SetExtendedObjectVar_Guard(RunningScriptLite* thread)
{
    return CallCleoPlusExtendedObjectVarSafely(
        "SET_EXTENDED_OBJECT_VAR",
        g_cleoPlusSetExtendedObjectVarTrampoline,
        thread);
}

extern "C" CleoExports::OpcodeResult __stdcall Bridge_CleoPlus_GetExtendedObjectVar_Guard(RunningScriptLite* thread)
{
    return CallCleoPlusExtendedObjectVarSafely(
        "GET_EXTENDED_OBJECT_VAR",
        g_cleoPlusGetExtendedObjectVarTrampoline,
        thread);
}

extern "C" void __stdcall Bridge_LogInvalidTxdLoadDictionaryWrite(uintptr_t slot, uintptr_t dictionary, uintptr_t index)
{
    static LONG logCount = 0;
    const LONG count = InterlockedIncrement(&logCount);
    if (count > 16) {
        return;
    }

    Log("txd load guard: blocked invalid dictionary write slot=0x%08X dictionary=0x%08X index=0x%08X",
        slot,
        dictionary,
        index);
    LogMemoryRegion("txd-load-slot", slot);
    LogMemoryRegion("txd-load-dictionary", dictionary);
    LogStackModules(reinterpret_cast<uintptr_t>(_AddressOfReturnAddress()) + sizeof(uintptr_t));
}

void UpdateScriptCompareFlag(RunningScriptLite* script, bool state)
{
    using UpdateCompareFlagFn = void(__thiscall*)(RunningScriptLite*, bool);
    auto updateCompareFlag = reinterpret_cast<UpdateCompareFlagFn>(kCRunningScriptUpdateCompareFlag);

    __try {
        updateCompareFlag(script, state);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

bool RequestSanPabloSpecialActors()
{
    using RequestSpecialModelFn = void(__cdecl*)(int, const char*, int);
    auto requestSpecialModel = reinterpret_cast<RequestSpecialModelFn>(kCStreamingRequestSpecialModel);
    constexpr int kStreamingMissionKeepPriority = 0x04 | 0x08 | 0x10;

    __try {
        for (const auto& actor : kSanPabloSpecialActors) {
            if (!SafeModelInfoEntryAddress(actor.modelId) || !SafeStreamingInfoEntryAddress(actor.modelId)) {
                Log("sanpablo special actor bridge: missing model/streaming entry model=%u name=%s",
                    actor.modelId, actor.name);
                return false;
            }
            requestSpecialModel(static_cast<int>(actor.modelId), actor.name, kStreamingMissionKeepPriority);
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("sanpablo special actor bridge: RequestSpecialModel exception");
        return false;
    }
}

bool DecodeGenericSpecialActorMode(int mode, int* op, uint32_t* modelId, uint32_t* actorCode)
{
    if (!op || !modelId || !actorCode) {
        return false;
    }

    *actorCode = 0;
    if (mode < 0) {
        const uint32_t encoded = static_cast<uint32_t>(mode);
        *op = static_cast<int>((encoded >> 29) & 0x03);
        *actorCode = (encoded >> 18) & 0x07FF;
        *modelId = encoded & 0x0003FFFF;
        return *op >= 1 && *op <= 3 && *modelId > 0;
    }

    if (mode >= 10000000 && mode < 40000000) {
        *op = mode / 10000000;
        const int rest = mode % 10000000;
        *modelId = static_cast<uint32_t>(rest / 1000);
        *actorCode = static_cast<uint32_t>(rest % 1000);
        return *op >= 1 && *op <= 3 && *modelId > 0;
    }

    if (mode >= 100000 && mode < 400000) {
        *op = mode / 100000;
        *modelId = static_cast<uint32_t>(mode % 100000);
        return *op >= 1 && *op <= 3 && *modelId > 0;
    }

    if (mode >= 10000 && mode < 40000) {
        *op = mode / 10000;
        *modelId = static_cast<uint32_t>(mode % 10000);
        return *op >= 1 && *op <= 3 && *modelId > 0;
    }

    return false;
}

bool AreSanPabloSpecialActorsLoaded()
{
    for (const auto& actor : kSanPabloSpecialActors) {
        if (GetStreamingLoadState(actor.modelId) != 1 || !IsModelRwObjectLoaded(actor.modelId)) {
            return false;
        }
    }
    return true;
}

bool ReleaseSanPabloSpecialActors()
{
    using SetMissionDoesntRequireModelFn = void(__cdecl*)(int);
    auto setMissionDoesntRequireModel = reinterpret_cast<SetMissionDoesntRequireModelFn>(kCStreamingSetMissionDoesntRequireModel);

    __try {
        for (const auto& actor : kSanPabloSpecialActors) {
            setMissionDoesntRequireModel(static_cast<int>(actor.modelId));
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("sanpablo special actor bridge: SetMissionDoesntRequireModel exception");
        return false;
    }
}

bool IsNumberVariableOperandType(int operandType)
{
    return operandType == kScriptParamGlobalNumberVariable ||
        operandType == kScriptParamLocalNumberVariable ||
        operandType == kScriptParamGlobalNumberArray ||
        operandType == kScriptParamLocalNumberArray;
}

bool HandleSpecialActorBridgeMode(RunningScriptLite* script, int mode, int* outResult, int* outGenericOp, uint32_t* outGenericModelId, uint32_t* outGenericActorCode)
{
    if (!outResult || !outGenericOp || !outGenericModelId || !outGenericActorCode) {
        return false;
    }

    *outResult = 0;
    *outGenericOp = 0;
    *outGenericModelId = 0;
    *outGenericActorCode = 0;

    const bool isSanPabloScript = IsScriptNamed(script, "SANPABLO");
    const bool isGenericBridgeMode = DecodeGenericSpecialActorMode(mode, outGenericOp, outGenericModelId, outGenericActorCode);
    if (!isSanPabloScript && !isGenericBridgeMode) {
        return false;
    }

    if (isGenericBridgeMode) {
        switch (*outGenericOp) {
        case 1:
            *outResult = RequestConfiguredSpecialActor(*outGenericModelId, *outGenericActorCode) ? 1 : 0;
            break;
        case 2:
            *outResult = (GetStreamingLoadState(*outGenericModelId) == 1 && IsModelRwObjectLoaded(*outGenericModelId)) ? 1 : 0;
            break;
        case 3:
            *outResult = ReleaseConfiguredSpecialActor(*outGenericModelId) ? 1 : 0;
            break;
        default:
            break;
        }
        return true;
    }

    switch (mode) {
    case 1:
        *outResult = RequestSanPabloSpecialActors() ? 1 : 0;
        break;
    case 2:
        *outResult = AreSanPabloSpecialActorsLoaded() ? 1 : 0;
        break;
    case 3:
        *outResult = ReleaseSanPabloSpecialActors() ? 1 : 0;
        break;
    default:
        Log("sanpablo special actor bridge: unknown mode=%d", mode);
        break;
    }
    return true;
}

signed char WINAPI Bridge_CleoScriptOpcodeProcessBefore(RunningScriptLite* script, DWORD opcode)
{
    if (opcode != kCommandUnloadSpecialCharacterHexAlias ||
        !g_config.enableSanPabloSpecialActorBridge ||
        !script ||
        !script->currentIP ||
        !g_cleoExports.getOperandType ||
        !g_cleoExports.peekPointerToScriptVariable ||
        !g_cleoExports.skipOpcodeParams) {
        return kCleoOpcodeResultNone;
    }

    int operandType = -1;
    ScriptParamLite* modeVariable = nullptr;
    __try {
        operandType = g_cleoExports.getOperandType(script);
        if (!IsNumberVariableOperandType(operandType)) {
            return kCleoOpcodeResultNone;
        }
        modeVariable = g_cleoExports.peekPointerToScriptVariable(script);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("special actor bridge: CLEO opcode-before param peek exception script='%.*s'", 8, script->name);
        return kCleoOpcodeResultNone;
    }

    if (!modeVariable || !IsWritableCommitted(reinterpret_cast<uintptr_t>(modeVariable), sizeof(*modeVariable))) {
        return kCleoOpcodeResultNone;
    }

    int mode = 0;
    __try {
        mode = modeVariable->iParam;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return kCleoOpcodeResultNone;
    }

    int result = 0;
    int genericOp = 0;
    uint32_t genericModelId = 0;
    uint32_t genericActorCode = 0;
    if (!HandleSpecialActorBridgeMode(script, mode, &result, &genericOp, &genericModelId, &genericActorCode)) {
        return kCleoOpcodeResultNone;
    }

    __try {
        g_cleoExports.skipOpcodeParams(script, 1);
        modeVariable->iParam = result;
        if (g_cleoExports.setThreadCondResult) {
            g_cleoExports.setThreadCondResult(script, result != 0 ? TRUE : FALSE);
        } else {
            UpdateScriptCompareFlag(script, result != 0);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("special actor bridge: CLEO opcode-before write exception script='%.*s' mode=%d result=%d",
            8, script->name, mode, result);
        return kCleoOpcodeResultNone;
    }

    const long count = InterlockedIncrement(&g_sanPabloSpecialActorBridgeLogs);
    if (ShouldLogSanPabloSpecialActorBridge(count, mode, result)) {
        Log("special actor bridge: CLEO before script='%.*s' mode=%d genericOp=%d genericModel=%u genericActorCode=%u result=%d sanpabloLoaded=%d states=[%u,%u,%u,%u,%u] operand=%d var=0x%08X",
            8,
            script->name,
            mode,
            genericOp,
            genericModelId,
            genericActorCode,
            result,
            AreSanPabloSpecialActorsLoaded() ? 1 : 0,
            static_cast<unsigned>(GetStreamingLoadState(290)),
            static_cast<unsigned>(GetStreamingLoadState(291)),
            static_cast<unsigned>(GetStreamingLoadState(292)),
            static_cast<unsigned>(GetStreamingLoadState(293)),
            static_cast<unsigned>(GetStreamingLoadState(294)),
            operandType,
            reinterpret_cast<uintptr_t>(modeVariable));
    }

    return kCleoOpcodeResultContinue;
}

signed char CallNativeCleoOpcode0296(RunningScriptLite* script)
{
    if (g_cleoExports.callNativeOpcode) {
        __try {
            return g_cleoExports.callNativeOpcode(script, kCommandUnloadSpecialCharacterHexAlias);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Log("special actor bridge: CLEO_CallNativeOpcode(0296) exception script='%.*s'", 8, script ? script->name : "");
        }
    }
    return -1;
}

signed char __stdcall Bridge_CleoOpcode0296(RunningScriptLite* script)
{
    if (!g_config.enableSanPabloSpecialActorBridge ||
        !script ||
        !script->currentIP ||
        !g_cleoExports.getOperandType ||
        !g_cleoExports.peekPointerToScriptVariable ||
        !g_cleoExports.skipOpcodeParams ||
        !g_cleoExports.callNativeOpcode) {
        return CallNativeCleoOpcode0296(script);
    }

    int operandType = -1;
    ScriptParamLite* modeVariable = nullptr;
    __try {
        operandType = g_cleoExports.getOperandType(script);
        if (!IsNumberVariableOperandType(operandType)) {
            return CallNativeCleoOpcode0296(script);
        }
        modeVariable = g_cleoExports.peekPointerToScriptVariable(script);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("special actor bridge: CLEO 0296 param peek exception script='%.*s'", 8, script->name);
        return CallNativeCleoOpcode0296(script);
    }

    if (!modeVariable || !IsWritableCommitted(reinterpret_cast<uintptr_t>(modeVariable), sizeof(*modeVariable))) {
        return CallNativeCleoOpcode0296(script);
    }

    int mode = 0;
    __try {
        mode = modeVariable->iParam;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return CallNativeCleoOpcode0296(script);
    }

    int result = 0;
    int genericOp = 0;
    uint32_t genericModelId = 0;
    uint32_t genericActorCode = 0;
    if (!HandleSpecialActorBridgeMode(script, mode, &result, &genericOp, &genericModelId, &genericActorCode)) {
        return CallNativeCleoOpcode0296(script);
    }

    __try {
        g_cleoExports.skipOpcodeParams(script, 1);
        modeVariable->iParam = result;
        if (g_cleoExports.setThreadCondResult) {
            g_cleoExports.setThreadCondResult(script, result != 0 ? TRUE : FALSE);
        } else {
            UpdateScriptCompareFlag(script, result != 0);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("special actor bridge: CLEO 0296 write exception script='%.*s' mode=%d result=%d",
            8, script->name, mode, result);
        return -1;
    }

    const long count = InterlockedIncrement(&g_sanPabloSpecialActorBridgeLogs);
    if (ShouldLogSanPabloSpecialActorBridge(count, mode, result)) {
        Log("special actor bridge: CLEO 0296 script='%.*s' mode=%d genericOp=%d genericModel=%u genericActorCode=%u result=%d sanpabloLoaded=%d states=[%u,%u,%u,%u,%u] operand=%d var=0x%08X",
            8,
            script->name,
            mode,
            genericOp,
            genericModelId,
            genericActorCode,
            result,
            AreSanPabloSpecialActorsLoaded() ? 1 : 0,
            static_cast<unsigned>(GetStreamingLoadState(290)),
            static_cast<unsigned>(GetStreamingLoadState(291)),
            static_cast<unsigned>(GetStreamingLoadState(292)),
            static_cast<unsigned>(GetStreamingLoadState(293)),
            static_cast<unsigned>(GetStreamingLoadState(294)),
            operandType,
            reinterpret_cast<uintptr_t>(modeVariable));
    }

    return kCleoOpcodeResultContinue;
}

unsigned char __fastcall Bridge_ProcessCommands600To699(RunningScriptLite* script, void*, unsigned short commandID)
{
    if (!g_originalCommands600To699) {
        return 0;
    }

    if (InterlockedCompareExchange(&g_forwardingCommands600To699, 0, 0) != 0) {
        auto vanilla = reinterpret_cast<ScriptCommandHandlerFn>(kProcessCommands600To699);
        return vanilla(script, commandID);
    }

    if ((commandID != kCommandUnloadSpecialCharacter && commandID != kCommandUnloadSpecialCharacterHexAlias) ||
        !g_config.enableSanPabloSpecialActorBridge ||
        !script ||
        !script->currentIP) {
        InterlockedExchange(&g_forwardingCommands600To699, 1);
        const unsigned char result = g_originalCommands600To699(script, commandID);
        InterlockedExchange(&g_forwardingCommands600To699, 0);
        return result;
    }

    auto collectParameters = reinterpret_cast<ScriptCollectParametersFn>(kCRunningScriptCollectParameters);
    auto storeParameters = reinterpret_cast<ScriptStoreParametersFn>(kCRunningScriptStoreParameters);
    auto params = reinterpret_cast<ScriptParamLite*>(kScriptParams);
    uint8_t* const commandIp = script->currentIP;

    int mode = 0;
    bool collected = false;
    __try {
        collectParameters(script, 1);
        mode = params[0].iParam;
        collected = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        script->currentIP = commandIp;
    }

    int genericOp = 0;
    uint32_t genericModelId = 0;
    uint32_t genericActorCode = 0;
    const bool isSanPabloScript = IsScriptNamed(script, "SANPABLO");
    const bool isGenericBridgeMode = collected && DecodeGenericSpecialActorMode(mode, &genericOp, &genericModelId, &genericActorCode);
    if (!isSanPabloScript && !isGenericBridgeMode) {
        script->currentIP = commandIp;
        InterlockedExchange(&g_forwardingCommands600To699, 1);
        const unsigned char result = g_originalCommands600To699(script, commandID);
        InterlockedExchange(&g_forwardingCommands600To699, 0);
        return result;
    }

    int result = 0;
    if (isGenericBridgeMode) {
        switch (genericOp) {
        case 1:
            result = RequestConfiguredSpecialActor(genericModelId, genericActorCode) ? 1 : 0;
            break;
        case 2:
            result = (GetStreamingLoadState(genericModelId) == 1 && IsModelRwObjectLoaded(genericModelId)) ? 1 : 0;
            break;
        case 3:
            result = ReleaseConfiguredSpecialActor(genericModelId) ? 1 : 0;
            break;
        default:
            break;
        }
    } else if (collected) {
        switch (mode) {
        case 1:
            result = RequestSanPabloSpecialActors() ? 1 : 0;
            break;
        case 2:
            result = AreSanPabloSpecialActorsLoaded() ? 1 : 0;
            break;
        case 3:
            result = ReleaseSanPabloSpecialActors() ? 1 : 0;
            break;
        default:
            Log("sanpablo special actor bridge: unknown mode=%d", mode);
            break;
        }
    }

    __try {
        params[0].iParam = result;
        storeParameters(script, 1);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("sanpablo special actor bridge: StoreParameters exception mode=%d result=%d", mode, result);
    }

    UpdateScriptCompareFlag(script, result != 0);

    const long count = InterlockedIncrement(&g_sanPabloSpecialActorBridgeLogs);
    if (ShouldLogSanPabloSpecialActorBridge(count, mode, result)) {
        Log("special actor bridge: script='%.*s' mode=%d genericOp=%d genericModel=%u genericActorCode=%u result=%d sanpabloLoaded=%d states=[%u,%u,%u,%u,%u]",
            8,
            script->name,
            mode,
            genericOp,
            genericModelId,
            genericActorCode,
            result,
            AreSanPabloSpecialActorsLoaded() ? 1 : 0,
            static_cast<unsigned>(GetStreamingLoadState(290)),
            static_cast<unsigned>(GetStreamingLoadState(291)),
            static_cast<unsigned>(GetStreamingLoadState(292)),
            static_cast<unsigned>(GetStreamingLoadState(293)),
            static_cast<unsigned>(GetStreamingLoadState(294)));
    }

    return 0;
}

void InstallSanPabloSpecialActorBridge()
{
#if defined(_M_IX86)
    if (ResolveCleoExports() &&
        g_cleoExports.registerOpcode &&
        g_cleoExports.callNativeOpcode &&
        g_cleoExports.getOperandType &&
        g_cleoExports.peekPointerToScriptVariable &&
        g_cleoExports.skipOpcodeParams &&
        InterlockedCompareExchange(&g_sanPabloCleoOpcodeRegistered, 1, 0) == 0) {
        __try {
            if (g_cleoExports.registerOpcode(kCommandUnloadSpecialCharacterHexAlias, Bridge_CleoOpcode0296)) {
                Log("sanpablo special actor bridge: registered CLEO opcode=0x%04X handler=0x%08X nativeForward=%p",
                    kCommandUnloadSpecialCharacterHexAlias,
                    reinterpret_cast<uintptr_t>(Bridge_CleoOpcode0296),
                    g_cleoExports.callNativeOpcode);
                return;
            }
            InterlockedExchange(&g_sanPabloCleoOpcodeRegistered, 0);
            Log("sanpablo special actor bridge: CLEO_RegisterOpcode failed opcode=0x%04X; falling back to handler table",
                kCommandUnloadSpecialCharacterHexAlias,
                reinterpret_cast<uintptr_t>(Bridge_CleoOpcode0296));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            InterlockedExchange(&g_sanPabloCleoOpcodeRegistered, 0);
            Log("sanpablo special actor bridge: CLEO opcode registration exception; falling back to handler table");
        }
    }

    auto table = reinterpret_cast<uintptr_t*>(kScriptCommandHandlerTable);
    if (!IsReadableCommitted(kScriptCommandHandlerTable, kCommandHandlerTableCount * sizeof(uintptr_t))) {
        Log("sanpablo special actor bridge: command handler table unreadable at 0x%08X", kScriptCommandHandlerTable);
        return;
    }

    uintptr_t currentHandler = 0;
    const uintptr_t slot = kScriptCommandHandlerTable + kCommandHandlerTableIndex600To699 * sizeof(uintptr_t);
    __try {
        currentHandler = table[kCommandHandlerTableIndex600To699];
        if (currentHandler == reinterpret_cast<uintptr_t>(Bridge_ProcessCommands600To699)) {
            Log("sanpablo special actor bridge: already installed tableIndex=%u slot=0x%08X",
                kCommandHandlerTableIndex600To699, slot);
            return;
        }
        if (table[0] == currentHandler && table[1] == currentHandler && table[2] == currentHandler &&
            table[3] == currentHandler && table[4] == currentHandler && table[5] == currentHandler) {
            Log("sanpablo special actor bridge: shared dispatcher detected handler=0x%08X; installing with recursion guard",
                currentHandler);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("sanpablo special actor bridge: command handler table scan exception");
        return;
    }

    if (!currentHandler || !IsExecutableCommitted(currentHandler)) {
        Log("sanpablo special actor bridge: current 600-699 handler not executable tableIndex=%u handler=0x%08X vanilla=0x%08X",
            kCommandHandlerTableIndex600To699, currentHandler, kProcessCommands600To699);
        return;
    }

    g_originalCommands600To699 = reinterpret_cast<ScriptCommandHandlerFn>(currentHandler);
    g_commands600To699TableSlot = slot;
    const uintptr_t bridge = reinterpret_cast<uintptr_t>(Bridge_ProcessCommands600To699);
    if (WriteBytesWithProtect(slot, reinterpret_cast<const uint8_t*>(&bridge), sizeof(bridge))) {
        Log("sanpablo special actor bridge: installed tableIndex=%u tableSlot=0x%08X old=0x%08X vanilla=0x%08X bridge=0x%08X",
            kCommandHandlerTableIndex600To699, slot, currentHandler, kProcessCommands600To699, bridge);
    }
#else
    Log("sanpablo special actor bridge: unsupported architecture");
#endif
}

unsigned char __fastcall Bridge_ProcessCommands900To999(RunningScriptLite* script, void*, unsigned short commandID)
{
    if (!g_originalCommands900To999) {
        return 0;
    }

    if (InterlockedCompareExchange(&g_forwardingCommands900To999, 0, 0) != 0) {
        auto vanilla = reinterpret_cast<ScriptCommandHandlerFn>(kProcessCommands900To999);
        return vanilla(script, commandID);
    }

    if (commandID != kCommandGetClosestCarNodeWithHeading || !script || !script->currentIP) {
        InterlockedExchange(&g_forwardingCommands900To999, 1);
        const unsigned char result = g_originalCommands900To999(script, commandID);
        InterlockedExchange(&g_forwardingCommands900To999, 0);
        return result;
    }

    auto collectParameters = reinterpret_cast<ScriptCollectParametersFn>(kCRunningScriptCollectParameters);
    auto storeParameters = reinterpret_cast<ScriptStoreParametersFn>(kCRunningScriptStoreParameters);
    auto params = reinterpret_cast<ScriptParamLite*>(kScriptParams);

    uint8_t* const commandIp = script->currentIP;
    float inX = 0.0f;
    float inY = 0.0f;
    float inZ = 0.0f;
    bool haveInput = false;

    __try {
        collectParameters(script, 3);
        inX = params[0].fParam;
        inY = params[1].fParam;
        inZ = params[2].fParam;
        haveInput = IsValidScriptCoord3D(inX, inY, inZ);
        script->currentIP = commandIp;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        script->currentIP = commandIp;
    }

    if (haveInput) {
        RequestPathStreamingForCoord(inX, inY, inZ);
    }

    InterlockedExchange(&g_forwardingCommands900To999, 1);
    const unsigned char result = g_originalCommands900To999(script, commandID);
    InterlockedExchange(&g_forwardingCommands900To999, 0);
    uint8_t* const afterOriginalIp = script->currentIP;

    if (!haveInput) {
        return result;
    }

    const float outX = params[0].fParam;
    const float outY = params[1].fParam;
    const float outZ = params[2].fParam;
    const float outHeading = params[3].fParam;

    if (!IsNearWorldOrigin2D(outX, outY) || IsNearWorldOrigin2D(inX, inY)) {
        return result;
    }

    float fallbackZ = inZ;
    if (AbsFloat(fallbackZ) < 1.0f) {
        fallbackZ = SafeFindGroundZForCoord(inX, inY, 10.0f);
    }

    float fallbackHeading = outHeading;
    if (!IsReasonableWorldCoord(fallbackHeading)) {
        fallbackHeading = 0.0f;
    }

    __try {
        script->currentIP = commandIp;
        collectParameters(script, 3);
        params[0].fParam = inX;
        params[1].fParam = inY;
        params[2].fParam = fallbackZ;
        params[3].fParam = fallbackHeading;
        storeParameters(script, 4);
        script->currentIP = afterOriginalIp;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        script->currentIP = afterOriginalIp;
        const long count = InterlockedIncrement(&g_closestCarNode03D3FallbackLogs);
        if (count <= 16) {
            Log("03D3 fallback: failed to store replacement input=(%.2f, %.2f, %.2f) originalOut=(%.2f, %.2f, %.2f)",
                inX, inY, inZ, outX, outY, outZ);
        }
        return result;
    }

    const long count = InterlockedIncrement(&g_closestCarNode03D3FallbackLogs);
    if (count <= 64) {
        Log("03D3 fallback: replaced origin output originalOut=(%.2f, %.2f, %.2f, %.2f) input=(%.2f, %.2f, %.2f) fallback=(%.2f, %.2f, %.2f, %.2f) script='%.*s'",
            outX, outY, outZ, outHeading,
            inX, inY, inZ,
            inX, inY, fallbackZ, fallbackHeading,
            8, script->name);
    }

    return result;
}

void InstallClosestCarNode03D3Fallback()
{
#if defined(_M_IX86)
    auto table = reinterpret_cast<uintptr_t*>(kScriptCommandHandlerTable);
    if (!IsReadableCommitted(kScriptCommandHandlerTable, kCommandHandlerTableCount * sizeof(uintptr_t))) {
        Log("03D3 fallback: command handler table unreadable at 0x%08X", kScriptCommandHandlerTable);
        return;
    }

    uintptr_t currentHandler = 0;
    const uintptr_t slot = kScriptCommandHandlerTable + kCommandHandlerTableIndex900To999 * sizeof(uintptr_t);
    __try {
        currentHandler = table[kCommandHandlerTableIndex900To999];
        if (currentHandler == reinterpret_cast<uintptr_t>(Bridge_ProcessCommands900To999)) {
            Log("03D3 fallback: already installed tableIndex=%u slot=0x%08X",
                kCommandHandlerTableIndex900To999, slot);
            return;
        }
        if (table[0] == currentHandler && table[1] == currentHandler && table[2] == currentHandler &&
            table[3] == currentHandler && table[4] == currentHandler && table[5] == currentHandler) {
            Log("03D3 fallback: shared dispatcher detected handler=0x%08X; installing with recursion guard",
                currentHandler);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("03D3 fallback: command handler table scan exception");
        return;
    }

    if (!currentHandler || !IsExecutableCommitted(currentHandler)) {
        Log("03D3 fallback: current 900-999 handler not executable tableIndex=%u handler=0x%08X vanilla=0x%08X",
            kCommandHandlerTableIndex900To999, currentHandler, kProcessCommands900To999);
        return;
    }

    g_originalCommands900To999 = reinterpret_cast<ScriptCommandHandlerFn>(currentHandler);
    g_commands900To999TableSlot = slot;
    const uintptr_t bridge = reinterpret_cast<uintptr_t>(Bridge_ProcessCommands900To999);
    if (WriteBytesWithProtect(slot, reinterpret_cast<const uint8_t*>(&bridge), sizeof(bridge))) {
        Log("03D3 fallback: installed tableIndex=%u tableSlot=0x%08X old=0x%08X vanilla=0x%08X bridge=0x%08X",
            kCommandHandlerTableIndex900To999, slot, currentHandler, kProcessCommands900To999, bridge);
    }
#else
    Log("03D3 fallback: unsupported architecture");
#endif
}

bool IsTaxi77Script(const RunningScriptLite* script)
{
    if (!script) {
        return false;
    }

    char name[9]{};
    __try {
        std::memcpy(name, script->name, 8);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    return ContainsCaseInsensitive(name, "taxi77");
}

bool SetLocalFloatFromParam(RunningScriptLite* script, const uint8_t* param, float value)
{
    if (!script || !param || !IsReadableCommitted(reinterpret_cast<uintptr_t>(param), 3)) {
        return false;
    }

    uint8_t type = 0;
    uint16_t index = 0;
    __try {
        type = param[0];
        std::memcpy(&index, param + 1, sizeof(index));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    if (type != 0x03 || index >= 32) {
        return false;
    }

    auto locals = reinterpret_cast<ScriptParamLite*>(reinterpret_cast<uintptr_t>(script) + kRunningScriptLocalVarsOffset);
    if (!IsWritableCommitted(reinterpret_cast<uintptr_t>(&locals[index]), sizeof(ScriptParamLite))) {
        return false;
    }

    __try {
        locals[index].fParam = value;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool SetScriptCarCoordinatesSafe(uint32_t carHandle, float x, float y, float z)
{
    using GetVehicleFn = void*(__cdecl*)(int);
    using SetCoordsOfScriptCarFn = void(__cdecl*)(void*, float, float, float, uint8_t, uint8_t);

    auto getVehicle = reinterpret_cast<GetVehicleFn>(kCPoolsGetVehicle);
    auto setCoords = reinterpret_cast<SetCoordsOfScriptCarFn>(kCCarCtrlSetCoordsOfScriptCar);

    __try {
        void* vehicle = getVehicle(static_cast<int>(carHandle));
        if (!vehicle || !IsReadableCommitted(reinterpret_cast<uintptr_t>(vehicle), 4)) {
            return false;
        }

        setCoords(vehicle, x, y, z, 0, 0);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

unsigned char __fastcall Bridge_ProcessCommands100To199(RunningScriptLite* script, void*, int commandID)
{
    if (!g_originalProcessCommands100To199) {
        return 0;
    }

    if (commandID != kCommandSetCarCoordinates || !script || !script->currentIP || !IsTaxi77Script(script)) {
        return g_originalProcessCommands100To199(script, commandID);
    }

    auto collectParameters = reinterpret_cast<ScriptCollectParametersFn>(kCRunningScriptCollectParameters);
    auto params = reinterpret_cast<ScriptParamLite*>(kScriptParams);

    uint8_t* const commandIp = script->currentIP;
    uint32_t car = 0;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    bool collected = false;

    __try {
        collectParameters(script, 4);
        car = params[0].uParam;
        x = params[1].fParam;
        y = params[2].fParam;
        z = params[3].fParam;
        collected = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        script->currentIP = commandIp;
    }

    if (!collected) {
        return g_originalProcessCommands100To199(script, commandID);
    }

    const uintptr_t baseIp = reinterpret_cast<uintptr_t>(script->baseIP);
    const uintptr_t paramsOffset = baseIp ? reinterpret_cast<uintptr_t>(commandIp) - baseIp : 0;
    const uintptr_t opcodeOffset = paramsOffset >= 2 ? paramsOffset - 2 : paramsOffset;

    const bool originWrite = IsNearWorldOrigin2DLoose(x, y);
    const LONG count = InterlockedIncrement(&g_taxi77SetCarCoordinatesLogs);
    if (count <= 128 || originWrite) {
        Log("taxi77 00AB: %s script='%.*s' opcodeOff=0x%X paramsOff=0x%X car=%u coords=(%.2f, %.2f, %.2f)",
            originWrite ? "originish" : "pass",
            8,
            script->name,
            static_cast<uint32_t>(opcodeOffset),
            static_cast<uint32_t>(paramsOffset),
            car,
            x,
            y,
            z);
    }

    if (originWrite) {
        if (InterlockedCompareExchange(&g_hasLastTargetBlipCoords, 0, 0) == 1) {
            float targetZ = g_lastTargetBlipZ;
            if (AbsFloat(targetZ) < 1.0f) {
                targetZ = SafeFindGroundZForCoord(g_lastTargetBlipX, g_lastTargetBlipY, 10.0f);
            }
            if (AbsFloat(targetZ) < 1.0f) {
                targetZ = 10.0f;
            }
            const bool moved = SetScriptCarCoordinatesSafe(car, g_lastTargetBlipX, g_lastTargetBlipY, targetZ + 2.0f);
            Log("taxi77 00AB: repaired-origin opcodeOff=0x%X old=(%.2f, %.2f, %.2f) target=(%.2f, %.2f, %.2f) moved=%d",
                static_cast<uint32_t>(opcodeOffset),
                x,
                y,
                z,
                g_lastTargetBlipX,
                g_lastTargetBlipY,
                targetZ + 2.0f,
                moved ? 1 : 0);
            return 0;
        }

        Log("taxi77 00AB: blocked-origin opcodeOff=0x%X no usable target coords", static_cast<uint32_t>(opcodeOffset));
        return 0;
    }

    script->currentIP = commandIp;
    return g_originalProcessCommands100To199(script, commandID);
}

void InstallTaxi77SetCarCoordinatesGuard()
{
#if defined(_M_IX86)
    constexpr size_t stolenBytes = 8;
    static const uint8_t expectedBytes[stolenBytes] = {
        0x64, 0xA1, 0x00, 0x00, 0x00, 0x00, // mov eax, fs:[0]
        0x6A, 0xFF                          // push -1
    };

    const uintptr_t bridge = reinterpret_cast<uintptr_t>(Bridge_ProcessCommands100To199);
    const uintptr_t currentTarget = DecodeRel32JumpTarget(kProcessCommands100To199);
    if (currentTarget == bridge) {
        Log("taxi77 00AB guard: already installed at 0x%08X", kProcessCommands100To199);
        return;
    }
    if (currentTarget) {
        Log("taxi77 00AB guard: skipped, ProcessCommands100To199 already hooked target=0x%08X", currentTarget);
        return;
    }

    uint8_t current[stolenBytes]{};
    if (!IsReadableCommitted(kProcessCommands100To199, sizeof(current))) {
        Log("taxi77 00AB guard: function unreadable at 0x%08X", kProcessCommands100To199);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(kProcessCommands100To199), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("taxi77 00AB guard: read exception at 0x%08X", kProcessCommands100To199);
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("taxi77 00AB guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X %02X %02X %02X",
            kProcessCommands100To199,
            current[0], current[1], current[2], current[3],
            current[4], current[5], current[6], current[7]);
        return;
    }

    g_processCommands100To199Trampoline = CreateRel32Trampoline(kProcessCommands100To199, stolenBytes);
    if (!g_processCommands100To199Trampoline) {
        return;
    }

    g_originalProcessCommands100To199 = reinterpret_cast<ProcessCommandsGroupFn>(g_processCommands100To199Trampoline);
    if (WriteRel32Jump(kProcessCommands100To199, bridge)) {
        Log("taxi77 00AB guard: installed at 0x%08X trampoline=0x%08X bridge=0x%08X",
            kProcessCommands100To199,
            g_processCommands100To199Trampoline,
            bridge);
    }
#else
    Log("taxi77 00AB guard: unsupported architecture");
#endif
}

DWORD ElapsedMs(uint32_t now, uint32_t then)
{
    return now - then;
}

bool ResolveCleoExports()
{
    HMODULE module = GetModuleHandleA("CLEO.asi");
    if (!module) {
        module = GetModuleHandleA("cleo.asi");
    }
    if (!module) {
        return false;
    }

    if (g_cleoExports.module == module &&
        g_cleoExports.getScriptByName &&
        g_cleoExports.isValidScriptPtr &&
        g_cleoExports.isScriptRunning &&
        g_cleoExports.getScriptBaseRelativeOffset &&
        g_cleoExports.threadJumpAtLabelPtr) {
        return true;
    }

    CleoExports exports{};
    exports.module = module;
    exports.getScriptByName = ResolveProcByNameOrOrdinal<CleoExports::GetScriptByNameFn>(
        module, "_CLEO_GetScriptByName@16", 47);
    exports.isValidScriptPtr = ResolveProcByNameOrOrdinal<CleoExports::IsValidScriptPtrFn>(
        module, "_CLEO_IsValidScriptPtr@4", 69);
    exports.isScriptRunning = ResolveProcByNameOrOrdinal<CleoExports::IsScriptRunningFn>(
        module, "_CLEO_IsScriptRunning@4", 53);
    exports.getScriptBaseRelativeOffset = ResolveProcByNameOrOrdinal<CleoExports::GetScriptBaseRelativeOffsetFn>(
        module, "_CLEO_GetScriptBaseRelativeOffset@8", 59);
    exports.threadJumpAtLabelPtr = ResolveProcByNameOrOrdinal<CleoExports::ThreadJumpAtLabelPtrFn>(
        module, "_CLEO_ThreadJumpAtLabelPtr@8", 14);
    exports.registerOpcode = ResolveProcByNameOrOrdinal<CleoExports::RegisterOpcodeFn>(
        module, "_CLEO_RegisterOpcode@8", 8);
    exports.callNativeOpcode = ResolveProcByNameOrOrdinal<CleoExports::CallNativeOpcodeFn>(
        module, "_CLEO_CallNativeOpcode@8", 61);
    exports.registerCallback = ResolveProcByNameOrOrdinal<CleoExports::RegisterCallbackFn>(
        module, "_CLEO_RegisterCallback@8", 27);
    exports.getOperandType = ResolveProcByNameOrOrdinal<CleoExports::GetOperandTypeFn>(
        module, "_CLEO_GetOperandType@4", 4);
    exports.peekPointerToScriptVariable = ResolveProcByNameOrOrdinal<CleoExports::PeekPointerToScriptVariableFn>(
        module, "_CLEO_PeekPointerToScriptVariable@4", 46);
    exports.skipOpcodeParams = ResolveProcByNameOrOrdinal<CleoExports::SkipOpcodeParamsFn>(
        module, "_CLEO_SkipOpcodeParams@8", 13);
    exports.setThreadCondResult = ResolveProcByNameOrOrdinal<CleoExports::SetThreadCondResultFn>(
        module, "_CLEO_SetThreadCondResult@8", 12);

    if (!exports.getScriptByName || !exports.isValidScriptPtr || !exports.isScriptRunning ||
        !exports.getScriptBaseRelativeOffset || !exports.threadJumpAtLabelPtr) {
        static LONG missingLogCount = 0;
        if (InterlockedIncrement(&missingLogCount) <= 8) {
            Log("CLEO exports incomplete module=0x%08X get=%p valid=%p running=%p offset=%p jump=%p",
                reinterpret_cast<uintptr_t>(module),
                exports.getScriptByName,
                exports.isValidScriptPtr,
                exports.isScriptRunning,
                exports.getScriptBaseRelativeOffset,
                exports.threadJumpAtLabelPtr);
        }
        return false;
    }

    g_cleoExports = exports;
    Log("CLEO exports resolved module=0x%08X registerOpcode=%p callNative=%p callback=%p operand=%p peekVar=%p skip=%p setCond=%p",
        reinterpret_cast<uintptr_t>(module),
        exports.registerOpcode,
        exports.callNativeOpcode,
        exports.registerCallback,
        exports.getOperandType,
        exports.peekPointerToScriptVariable,
        exports.skipOpcodeParams,
        exports.setThreadCondResult);
    return true;
}

bool IsCleoScriptUsable(RunningScriptLite* script)
{
    if (!script || !g_cleoExports.isValidScriptPtr || !g_cleoExports.isScriptRunning) {
        return false;
    }

    __try {
        return g_cleoExports.isValidScriptPtr(script) && g_cleoExports.isScriptRunning(script);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

RunningScriptLite* FindCleoScriptByName(const char* name)
{
    if (!g_cleoExports.getScriptByName) {
        return nullptr;
    }

    for (DWORD index = 0; index < 16; ++index) {
        RunningScriptLite* script = nullptr;
        __try {
            script = g_cleoExports.getScriptByName(name, FALSE, TRUE, index);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
        if (!script) {
            return nullptr;
        }
        if (IsCleoScriptUsable(script)) {
            return script;
        }
    }
    return nullptr;
}

RunningScriptLite* FindTaxi77MainScript()
{
    RunningScriptLite* script = FindCleoScriptByName("taxi77");
    if (script) {
        return script;
    }
    return FindCleoScriptByName("Taxi77");
}

RunningScriptLite* FindTaxi77MeterScript()
{
    RunningScriptLite* script = FindCleoScriptByName("taxi77m");
    if (script) {
        return script;
    }
    return FindCleoScriptByName("Taxi77M");
}

uint32_t GetCleoScriptOffset(RunningScriptLite* script)
{
    if (!script || !script->currentIP) {
        return 0xFFFFFFFF;
    }

    if (g_cleoExports.getScriptBaseRelativeOffset) {
        __try {
            return g_cleoExports.getScriptBaseRelativeOffset(script, script->currentIP);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    __try {
        if (script->baseIP && script->currentIP >= script->baseIP) {
            return static_cast<uint32_t>(script->currentIP - script->baseIP);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return 0xFFFFFFFF;
}

bool ReadTaxi77Local(RunningScriptLite* script, uint32_t index, ScriptParamLite* out)
{
    if (!script || !out || index >= 32) {
        return false;
    }

    auto locals = reinterpret_cast<ScriptParamLite*>(reinterpret_cast<uintptr_t>(script) + kRunningScriptLocalVarsOffset);
    if (!IsReadableCommitted(reinterpret_cast<uintptr_t>(&locals[index]), sizeof(ScriptParamLite))) {
        return false;
    }

    __try {
        *out = locals[index];
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool WriteTaxi77LocalDword(RunningScriptLite* script, uint32_t index, uint32_t value)
{
    if (!script || index >= 32) {
        return false;
    }

    auto locals = reinterpret_cast<ScriptParamLite*>(reinterpret_cast<uintptr_t>(script) + kRunningScriptLocalVarsOffset);
    if (!IsWritableCommitted(reinterpret_cast<uintptr_t>(&locals[index]), sizeof(ScriptParamLite))) {
        return false;
    }

    __try {
        locals[index].uParam = value;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool IsTaxi77ScriptProcessing(RunningScriptLite* script)
{
    if (!script || !IsReadableCommitted(reinterpret_cast<uintptr_t>(script) + 0xDF, 1)) {
        return true;
    }

    __try {
        const uint8_t flags = *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(script) + 0xDF);
        return (flags & 0x02) != 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return true;
    }
}

bool ResetTaxi77MainScriptToStart(RunningScriptLite* script, uint32_t offset, uint32_t local0)
{
    if (!script || !g_cleoExports.threadJumpAtLabelPtr || g_config.taxi77StartLabelOffset <= 0) {
        return false;
    }

    constexpr uint32_t transientMask =
        (1u << 0) | // teleport flag
        (1u << 1) | // friends clear-to-go flag
        (1u << 4) |
        (1u << 7) | // set blip message flag
        (1u << 8) | // water message flag
        (1u << 9) | // cruising flag
        (1u << 10); // FPV flag

    const uint32_t cleanedLocal0 = local0 & ~transientMask;
    const bool localWritten = WriteTaxi77LocalDword(script, 0, cleanedLocal0);

    __try {
        auto timers = reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(script) + 0xBC);
        if (IsWritableCommitted(reinterpret_cast<uintptr_t>(&timers[0]), sizeof(uint32_t) * 2)) {
            timers[0] = 0;
            timers[1] = 0;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }

    __try {
        g_cleoExports.threadJumpAtLabelPtr(script, -g_config.taxi77StartLabelOffset);
        Log("taxi77 watchdog: recovered main script ptr=0x%08X oldOffset=0x%X startOffset=0x%X local0=0x%08X->0x%08X localWritten=%d recoveryCount=%ld",
            reinterpret_cast<uintptr_t>(script),
            offset,
            g_config.taxi77StartLabelOffset,
            local0,
            cleanedLocal0,
            localWritten ? 1 : 0,
            InterlockedIncrement(&g_taxi77WatchdogRecoveries));
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("taxi77 watchdog: recovery exception ptr=0x%08X oldOffset=0x%X startOffset=0x%X",
            reinterpret_cast<uintptr_t>(script),
            offset,
            g_config.taxi77StartLabelOffset);
        return false;
    }
}

DWORD WINAPI Taxi77StateWatchdogThread(void*)
{
    Sleep(3000);
    Log("taxi77 watchdog: thread started pollMs=%d stuckSeconds=%d recovery=%d",
        g_config.taxi77WatchdogPollMs,
        g_config.taxi77MainStuckSeconds,
        g_config.enableTaxi77StateWatchdogRecovery ? 1 : 0);

    for (;;) {
        Sleep(static_cast<DWORD>(g_config.taxi77WatchdogPollMs));

        if (!ResolveCleoExports()) {
            continue;
        }

        RunningScriptLite* mainScript = FindTaxi77MainScript();
        RunningScriptLite* meterScript = FindTaxi77MeterScript();
        const uint32_t now = GetTickCount();

        if (!mainScript) {
            if (g_taxi77WatchdogMainPtr != 0) {
                Log("taxi77 watchdog: main script disappeared oldPtr=0x%08X", g_taxi77WatchdogMainPtr);
            }
            g_taxi77WatchdogMainPtr = 0;
            g_taxi77WatchdogActiveSince = 0;
            g_taxi77WatchdogLastOffset = 0xFFFFFFFF;
            continue;
        }

        const uintptr_t mainPtr = reinterpret_cast<uintptr_t>(mainScript);
        if (g_taxi77WatchdogMainPtr != mainPtr) {
            Log("taxi77 watchdog: main script ptr=0x%08X", mainPtr);
            g_taxi77WatchdogMainPtr = mainPtr;
            g_taxi77WatchdogActiveSince = 0;
            g_taxi77WatchdogLastOffset = 0xFFFFFFFF;
        }

        const uint32_t offset = GetCleoScriptOffset(mainScript);
        ScriptParamLite local0{};
        ScriptParamLite car{};
        ScriptParamLite driver{};
        ReadTaxi77Local(mainScript, 0, &local0);
        ReadTaxi77Local(mainScript, 10, &car);
        ReadTaxi77Local(mainScript, 11, &driver);

        const bool hasMeter = meterScript != nullptr;
        const bool activeRange =
            offset != 0xFFFFFFFF &&
            offset >= static_cast<uint32_t>(g_config.taxi77ActiveMinOffset) &&
            offset < static_cast<uint32_t>(g_config.taxi77ActiveMaxOffset);
        const bool recoverableActive = activeRange && !hasMeter;

        if (offset != g_taxi77WatchdogLastOffset ||
            g_taxi77WatchdogLastLog == 0 ||
            ElapsedMs(now, g_taxi77WatchdogLastLog) >= 10000) {
            Log("taxi77 watchdog: state ptr=0x%08X offset=0x%X active=%d meter=%d local0=0x%08X car=%u driver=%u activeForMs=%u",
                mainPtr,
                offset,
                activeRange ? 1 : 0,
                hasMeter ? 1 : 0,
                local0.uParam,
                car.uParam,
                driver.uParam,
                g_taxi77WatchdogActiveSince ? ElapsedMs(now, g_taxi77WatchdogActiveSince) : 0);
            g_taxi77WatchdogLastLog = now;
            g_taxi77WatchdogLastOffset = offset;
        }

        if (!recoverableActive) {
            g_taxi77WatchdogActiveSince = 0;
            continue;
        }

        if (g_taxi77WatchdogActiveSince == 0) {
            g_taxi77WatchdogActiveSince = now;
            continue;
        }

        const DWORD activeMs = ElapsedMs(now, g_taxi77WatchdogActiveSince);
        if (activeMs < static_cast<DWORD>(g_config.taxi77MainStuckSeconds) * 1000) {
            continue;
        }

        if (!g_config.enableTaxi77StateWatchdogRecovery) {
            Log("taxi77 watchdog: would recover main script ptr=0x%08X offset=0x%X activeMs=%u local0=0x%08X car=%u driver=%u",
                mainPtr,
                offset,
                activeMs,
                local0.uParam,
                car.uParam,
                driver.uParam);
            g_taxi77WatchdogActiveSince = now;
            continue;
        }

        if (IsTaxi77ScriptProcessing(mainScript)) {
            Log("taxi77 watchdog: recovery delayed because script is processing ptr=0x%08X offset=0x%X activeMs=%u",
                mainPtr,
                offset,
                activeMs);
            continue;
        }

        if (ResetTaxi77MainScriptToStart(mainScript, offset, local0.uParam)) {
            g_taxi77WatchdogActiveSince = 0;
            g_taxi77WatchdogLastOffset = 0xFFFFFFFF;
            g_taxi77WatchdogLastLog = 0;
        } else {
            g_taxi77WatchdogActiveSince = now;
        }
    }
}

void InstallOneCObjectCreateBridgeStub(const char* label, uintptr_t patchAddress, void* bridgeTarget)
{
#if defined(_M_IX86)
    const uintptr_t target = reinterpret_cast<uintptr_t>(bridgeTarget);
    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);

    Log("bridge stub: %s patch=0x%08X currentTarget=0x%08X targetExecutable=%d bridgeTarget=0x%08X",
        label,
        patchAddress,
        currentTarget,
        currentTarget ? (IsExecutableCommitted(currentTarget) ? 1 : 0) : -1,
        target);

    if (currentTarget == target) {
        return;
    }

    if (!currentTarget || !IsExecutableCommitted(currentTarget)) {
        if (WriteRel32Jump(patchAddress, target)) {
            Log("bridge stub: redirected stale FLA %s hook to bridge target", label);
        }
    }
#else
    (void)label;
    (void)patchAddress;
    (void)bridgeTarget;
#endif
}

void InstallCObjectCreateBridgeStubs()
{
#if defined(_M_IX86)
    InstallOneCObjectCreateBridgeStub("CObject::Create 0x5A1FA1", kCObjectCreatePatch1, Bridge_CObject_Create_5A1FA1);
    InstallOneCObjectCreateBridgeStub("CObject::Create 0x5A2016", kCObjectCreatePatch2, Bridge_CObject_Create_5A2016);
#endif
}

void RestoreCleoObjectCreateInlinePatch()
{
    static bool loggedOriginal = false;
    static bool loggedPatched = false;
    static const uint8_t originalBytes[] = {
        0x89, 0x9E, 0x74, 0x01, 0x00, 0x00
    }; // mov [esi+174h], ebx

    uint8_t current[sizeof(originalBytes)]{};
    if (!IsReadableCommitted(kCleoObjectCreateInlinePatch, sizeof(current))) {
        if (!loggedOriginal) {
            loggedOriginal = true;
            Log("CLEO+ compat: object-create inline 0x59FB1E address unreadable");
        }
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(kCleoObjectCreateInlinePatch), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        if (!loggedOriginal) {
            loggedOriginal = true;
            Log("CLEO+ compat: object-create inline 0x59FB1E read exception");
        }
        return;
    }

    if (std::memcmp(current, originalBytes, sizeof(originalBytes)) == 0) {
        if (!loggedOriginal) {
            loggedOriginal = true;
            Log("CLEO+ compat: object-create inline 0x59FB1E already original");
        }
        return;
    }

    if (WriteBytesWithProtect(kCleoObjectCreateInlinePatch, originalBytes, sizeof(originalBytes))) {
        if (!loggedPatched) {
            loggedPatched = true;
            Log("CLEO+ compat: restored object-create inline hook at 0x%08X old=%02X %02X %02X %02X %02X %02X",
                kCleoObjectCreateInlinePatch,
                current[0], current[1], current[2], current[3], current[4], current[5]);
        }
    }
}

void InstallCleoDispatchNullGuard()
{
#if defined(_M_IX86)
    HMODULE cleoPlus = GetModuleHandleA("CLEO+.cleo");
    if (!cleoPlus) {
        Log("CLEO+ dispatch guard: CLEO+.cleo not loaded yet");
        return;
    }

    const uintptr_t base = reinterpret_cast<uintptr_t>(cleoPlus);
    const uintptr_t patchAddress = base + 0x18C62;
    g_cleoDispatchNullGuardReturn = base + 0x18C67;

    static const uint8_t expectedBytes[] = { 0x8B, 0x01, 0xFF, 0x50, 0x08 };
    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("CLEO+ dispatch guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("CLEO+ dispatch guard: read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_Cleo_Dispatch_NullGuard);
    if (currentTarget == guardTarget) {
        Log("CLEO+ dispatch guard: already installed at 0x%08X", patchAddress);
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("CLEO+ dispatch guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X",
            patchAddress, current[0], current[1], current[2], current[3], current[4]);
        return;
    }

    if (WriteRel32Jump(patchAddress, guardTarget)) {
        Log("CLEO+ dispatch guard: installed at CLEO+.cleo+0x18C62 return=0x%08X target=0x%08X",
            g_cleoDispatchNullGuardReturn, guardTarget);
    }
#endif
}

void InstallCleoThunk26720NullGuard()
{
#if defined(_M_IX86)
    HMODULE cleoPlus = GetModuleHandleA("CLEO+.cleo");
    if (!cleoPlus) {
        Log("CLEO+ thunk26720 guard: CLEO+.cleo not loaded yet");
        return;
    }

    const uintptr_t base = reinterpret_cast<uintptr_t>(cleoPlus);
    const uintptr_t patchAddress = base + 0x26720;

    static const uint8_t expectedBytes[] = {
        0x8B, 0x41, 0x04,       // mov eax, [ecx+4]
        0x8B, 0x40, 0x18,       // mov eax, [eax+18h]
        0xFF, 0xE0              // jmp eax
    };

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("CLEO+ thunk26720 guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("CLEO+ thunk26720 guard: read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_Cleo_Thunk26720_NullGuard);
    if (currentTarget == guardTarget) {
        Log("CLEO+ thunk26720 guard: already installed at CLEO+.cleo+0x26720");
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("CLEO+ thunk26720 guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X %02X %02X %02X",
            patchAddress,
            current[0], current[1], current[2], current[3],
            current[4], current[5], current[6], current[7]);
        return;
    }

    g_cleoThunk26720Original = patchAddress;
    if (WriteRel32Jump(patchAddress, guardTarget)) {
        Log("CLEO+ thunk26720 guard: installed at CLEO+.cleo+0x26720 target=0x%08X",
            guardTarget);
    }
#endif
}

bool InstallOneCleoPlusOpcodeFunctionGuard(
    const char* name,
    uintptr_t patchAddress,
    uintptr_t guardTarget,
    uintptr_t* trampolineOut)
{
#if defined(_M_IX86)
    if (!trampolineOut || !IsExecutableCommitted(patchAddress)) {
        return false;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    if (currentTarget == guardTarget) {
        Log("CLEO+ extended object var guard: %s already installed at 0x%08X",
            name ? name : "",
            patchAddress);
        return *trampolineOut != 0;
    }

    uint8_t current[5]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        return false;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    if (current[0] == 0xE9) {
        Log("CLEO+ extended object var guard: %s already hooked by another module at 0x%08X target=0x%08X",
            name ? name : "",
            patchAddress,
            currentTarget);
        return false;
    }

    uintptr_t trampoline = CreateRel32Trampoline(patchAddress, sizeof(current));
    if (!trampoline) {
        Log("CLEO+ extended object var guard: %s trampoline failed at 0x%08X bytes=%02X %02X %02X %02X %02X",
            name ? name : "",
            patchAddress,
            current[0], current[1], current[2], current[3], current[4]);
        return false;
    }

    if (!WriteRel32Jump(patchAddress, guardTarget)) {
        VirtualFree(reinterpret_cast<void*>(trampoline), 0, MEM_RELEASE);
        return false;
    }

    *trampolineOut = trampoline;
    Log("CLEO+ extended object var guard: installed %s at 0x%08X trampoline=0x%08X target=0x%08X old=%02X %02X %02X %02X %02X",
        name ? name : "",
        patchAddress,
        trampoline,
        guardTarget,
        current[0], current[1], current[2], current[3], current[4]);
    return true;
#else
    (void)name;
    (void)patchAddress;
    (void)guardTarget;
    (void)trampolineOut;
    return false;
#endif
}

void InstallCleoPlusExtendedObjectVarGuard()
{
#if defined(_M_IX86)
    HMODULE cleoPlus = GetModuleHandleA("CLEO+.cleo");
    if (!cleoPlus) {
        Log("CLEO+ extended object var guard: CLEO+.cleo not loaded yet");
        return;
    }

    const uintptr_t base = reinterpret_cast<uintptr_t>(cleoPlus);
    struct OffsetSet {
        uintptr_t init;
        uintptr_t set;
        uintptr_t get;
        const char* label;
    };
    constexpr OffsetSet kOffsetSets[] = {
        { 0x306C0, 0x30800, 0x308E0, "restored-current" },
        { 0x177F0, 0x17940, 0x17A30, "local-rebuilt" },
    };

    for (const OffsetSet& offsets : kOffsetSets) {
        if (!InstallOneCleoPlusOpcodeFunctionGuard(
                "INIT_EXTENDED_OBJECT_VARS",
                base + offsets.init,
                reinterpret_cast<uintptr_t>(Bridge_CleoPlus_InitExtendedObjectVars_Guard),
                &g_cleoPlusInitExtendedObjectVarsTrampoline)) {
            continue;
        }
        if (!InstallOneCleoPlusOpcodeFunctionGuard(
                "SET_EXTENDED_OBJECT_VAR",
                base + offsets.set,
                reinterpret_cast<uintptr_t>(Bridge_CleoPlus_SetExtendedObjectVar_Guard),
                &g_cleoPlusSetExtendedObjectVarTrampoline)) {
            Log("CLEO+ extended object var guard: incomplete offset set %s after INIT; not trying other sets",
                offsets.label);
            return;
        }
        if (!InstallOneCleoPlusOpcodeFunctionGuard(
                "GET_EXTENDED_OBJECT_VAR",
                base + offsets.get,
                reinterpret_cast<uintptr_t>(Bridge_CleoPlus_GetExtendedObjectVar_Guard),
                &g_cleoPlusGetExtendedObjectVarTrampoline)) {
            Log("CLEO+ extended object var guard: incomplete offset set %s after INIT/SET; not trying other sets",
                offsets.label);
            return;
        }

        Log("CLEO+ extended object var guard: offset set selected %s base=0x%08X",
            offsets.label,
            base);
        return;
    }

    Log("CLEO+ extended object var guard: no supported CLEO+ offset set matched base=0x%08X", base);
#endif
}

