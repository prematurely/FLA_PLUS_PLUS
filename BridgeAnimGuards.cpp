#include "FLACompatBridgeInternal.h"

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_AnimUncompress_NullGuard()
{
    __asm
    {
        push edi
        mov edi, [esp + 8]

        push edi
        call Bridge_IsValidAnimHierarchy
        test al, al
        jz invalidHierarchy

        push dword ptr [g_animUncompressContinue]
        retn

    invalidHierarchy:
        mov eax, [esp + 4]
        mov ecx, [esp + 8]
        lea edx, [esp + 12]
        push edx
        push eax
        push ecx
        call Bridge_LogInvalidAnimHierarchy

        pop edi
        retn
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_AnimStaticAssocInit_Guard()
{
    __asm
    {
        push esi
        mov esi, ecx
        push edi
        mov edi, [esp + 0x0C]

        push edi
        call Bridge_IsValidStaticAssociation
        test al, al
        jz invalidAssociation

        push dword ptr [g_animStaticAssocInitContinue]
        retn

    invalidAssociation:
        push edi
        push esi
        call Bridge_RepairInvalidStaticAssociation

        pop edi
        pop esi
        ret 4
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_AnimUpdateBlend_Guard()
{
    __asm
    {
        push ecx
        push ecx
        call Bridge_PrepareAnimAssociationUpdate
        test al, al
        jz skipAssociation
        pop ecx

        fld dword ptr [esp + 4]
        push esi
        mov esi, ecx
        push dword ptr [g_animUpdateBlendContinue]
        retn

    skipAssociation:
        pop ecx
        xor al, al
        ret 4
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_AnimBlendGroup_Guard()
{
    __asm
    {
        push dword ptr [esp + 0x0C] // animId
        push dword ptr [esp + 0x0C] // groupId, adjusted after first push
        call Bridge_ShouldBlockGroupBlendAnimation
        test al, al
        jnz blockBlend

        sub esp, 0x14
        mov ecx, [esp + 0x18]
        push dword ptr [g_animBlendGroupContinue]
        retn

    blockBlend:
        xor eax, eax
        ret
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_AnimFrameUpdateSkinnedVelocity_Guard()
{
    __asm
    {
        push eax
        push eax
        call Bridge_PrepareAnimFrameUpdateData
        test al, al
        pop eax
        jz skipFrameUpdate

        sub esp, 0x68
        push ebp
        push esi
        push dword ptr [g_animFrameUpdateSkinnedVelocityContinue]
        retn

    skipFrameUpdate:
        retn
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_AnimFrameUpdateSkinned_Guard()
{
    __asm
    {
        push eax
        mov eax, [esp + 0x0C]
        push eax
        call Bridge_PrepareAnimFrameUpdateData
        test al, al
        pop eax
        jz skipFrameUpdate

        sub esp, 0x3C
        push ebp
        mov ebp, [esp + 0x44]
        push dword ptr [g_animFrameUpdateSkinnedContinue]
        retn

    skipFrameUpdate:
        retn
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_AnimClumpFinalizeNodes_Guard()
{
    __asm
    {
        mov dword ptr [esp + ebx * 4 + 0x1C], 0
        test ebx, ebx
        jz emptyUpdate

        push dword ptr [g_animClumpFinalizeNodesContinue]
        retn

    emptyUpdate:
        pushfd
        pushad
        lea eax, [esp + 0x3C]
        mov ecx, [esp + 0x74]
        push ecx
        push eax
        call Bridge_LogEmptyAnimUpdate
        popad
        popfd
        push dword ptr [g_animClumpFinalizeNodesEmpty]
        retn
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_AnimDestroyAssociations_Diagnostic()
{
    __asm
    {
        pushfd
        pushad
        mov eax, [esp + 0x24]
        push eax
        push ecx
        call Bridge_LogAnimGroupDestroy
        popad
        popfd

        push esi
        mov esi, ecx
        mov ecx, [esi + 4]
        push dword ptr [g_animDestroyAssociationsContinue]
        retn
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_AnimRemoveBlock_Diagnostic()
{
    __asm
    {
        pushfd
        pushad
        mov eax, [esp + 0x28]
        mov edx, [esp + 0x24]
        push edx
        push eax
        call Bridge_LogAnimBlockRemove
        popad
        popfd

        mov eax, dword ptr ds:[0x00B4EA28]
        push dword ptr [g_animRemoveBlockContinue]
        retn
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_RpAnimBlendClumpInit_Guard()
{
    __asm
    {
        mov eax, esp
        push eax                    // original stack
        push dword ptr [eax]        // original return address
        push dword ptr [eax + 0x04] // clump
        call Bridge_ShouldSkipRpAnimBlendClumpInit
        test al, al
        jnz skipInit

        push esi
        mov esi, [esp + 0x08]
        push dword ptr [g_rpAnimBlendClumpInitContinue]
        retn

    skipInit:
        ret
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_RpClumpForAllAtomics_Guard()
{
    __asm
    {
        mov eax, esp
        push eax                    // original stack
        push dword ptr [eax]        // original return address
        push dword ptr [eax + 0x0C] // data
        push dword ptr [eax + 0x08] // callback
        push dword ptr [eax + 0x04] // clump
        call Bridge_IsSafeRpClumpForAllAtomicsCall
        test al, al
        jz invalidCall

        mov eax, [esp + 0x04]
        push ebx
        push dword ptr [g_rwClumpForAllAtomicsContinue]
        retn

    invalidCall:
        xor eax, eax
        ret
    }
}
#endif

extern "C" bool __stdcall Bridge_IsValidAnimHierarchy(uintptr_t hierarchy)
{
    if (hierarchy < 0x10000) {
        return false;
    }

    if (!IsReadableCommitted(hierarchy, 0x20)) {
        return false;
    }

    uint8_t compressedFlags = 0;
    __try {
        compressedFlags = *reinterpret_cast<const uint8_t*>(hierarchy + 0x0B);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    (void)compressedFlags;
    g_lastValidAnimHierarchy = hierarchy;
    return true;
}

extern "C" void __stdcall Bridge_LogInvalidAnimHierarchy(uintptr_t hierarchy, uintptr_t returnAddress, uintptr_t stack)
{
    static LONG logCount = 0;
    const LONG count = InterlockedIncrement(&logCount);
    if (count > 4) {
        return;
    }

    char moduleName[MAX_PATH]{};
    const uintptr_t moduleBase = ModuleBaseFromAddress(returnAddress, moduleName, sizeof(moduleName));
    Log("anim guard: skipped invalid CAnimBlendHierarchy=0x%08X return=0x%08X %s+0x%X",
        hierarchy,
        returnAddress,
        moduleName,
        moduleBase ? returnAddress - moduleBase : 0);
    if (count <= 1) {
        LogMemoryRegion("anim-hierarchy", hierarchy);
        LogStackModules(stack);
    }
}

extern "C" bool __stdcall Bridge_IsValidStaticAssociation(uintptr_t staticAssociation)
{
    if (!IsReadableCommitted(staticAssociation, 0x14)) {
        return false;
    }

    uint32_t blendSeqs = 0;
    uint32_t blendHier = 0;
    uint16_t numBlendNodes = 0;
    __try {
        numBlendNodes = *reinterpret_cast<const uint16_t*>(staticAssociation + 0x04);
        blendSeqs = *reinterpret_cast<const uint32_t*>(staticAssociation + 0x0C);
        blendHier = *reinterpret_cast<const uint32_t*>(staticAssociation + 0x10);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    if (!Bridge_IsValidAnimHierarchy(blendHier)) {
        return false;
    }

    if (numBlendNodes > 512) {
        return false;
    }

    if (numBlendNodes != 0 && !IsReadableCommitted(blendSeqs, static_cast<size_t>(numBlendNodes) * sizeof(uintptr_t))) {
        return false;
    }

    return true;
}

void MarkNeutralizedAnimAssociation(uintptr_t association)
{
    if (!association) {
        return;
    }

    for (size_t i = 0; i < kMaxNeutralizedAnimAssociations; ++i) {
        if (g_neutralizedAnimAssociations[i] == association) {
            return;
        }
    }

    const LONG slot = InterlockedIncrement(&g_neutralizedAnimAssociationCursor);
    g_neutralizedAnimAssociations[static_cast<size_t>(slot) % kMaxNeutralizedAnimAssociations] = association;
}

bool ConsumeNeutralizedAnimAssociation(uintptr_t association)
{
    if (!association) {
        return false;
    }

    for (size_t i = 0; i < kMaxNeutralizedAnimAssociations; ++i) {
        if (g_neutralizedAnimAssociations[i] == association) {
            g_neutralizedAnimAssociations[i] = 0;
            return true;
        }
    }

    return false;
}

bool IsNeutralizedAnimAssociation(uintptr_t association)
{
    if (!association) {
        return false;
    }

    for (size_t i = 0; i < kMaxNeutralizedAnimAssociations; ++i) {
        if (g_neutralizedAnimAssociations[i] == association) {
            return true;
        }
    }

    return false;
}

bool LooksLikeNeutralizedAnimAssociation(uintptr_t association)
{
    if (!IsReadableCommitted(association, 0x30)) {
        return false;
    }

    uint16_t numBlendNodes = 0xFFFF;
    uintptr_t blendNodes = 0;
    float blendAmount = 1.0f;
    float blendDelta = 1.0f;
    float speed = 1.0f;
    float timeStep = 1.0f;
    uint16_t flags = 0xFFFF;
    __try {
        numBlendNodes = *reinterpret_cast<const uint16_t*>(association + 0x0C);
        blendNodes = *reinterpret_cast<const uintptr_t*>(association + 0x10);
        blendAmount = *reinterpret_cast<const float*>(association + 0x18);
        blendDelta = *reinterpret_cast<const float*>(association + 0x1C);
        speed = *reinterpret_cast<const float*>(association + 0x24);
        timeStep = *reinterpret_cast<const float*>(association + 0x28);
        flags = *reinterpret_cast<const uint16_t*>(association + 0x2E);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    constexpr uint16_t kAnimationReferenceBlock = 0x4000;
    constexpr uint16_t kAnimationBlendAutoRemove = 0x0004;
    return numBlendNodes == 0 &&
        blendNodes == 0 &&
        blendAmount == 0.0f &&
        blendDelta <= 0.0f &&
        speed == 0.0f &&
        timeStep == 0.0f &&
        ((flags & kAnimationReferenceBlock) == kAnimationReferenceBlock ||
            (flags & kAnimationBlendAutoRemove) == kAnimationBlendAutoRemove);
}

bool ReadAnimGroupSnapshot(
    uintptr_t group,
    uintptr_t* associationsOut,
    uint32_t* countOut,
    uint32_t* idOffsetOut,
    uintptr_t* animBlockOut,
    uint32_t* groupIndexOut)
{
    constexpr uintptr_t kAnimAssocGroupsPtr = 0x00B4EA34;
    constexpr uintptr_t kNumAnimAssocDefinitions = 0x00B4EA28;
    constexpr size_t kAssocGroupSize = 0x14;

    if (!IsReadableCommitted(group, kAssocGroupSize)) {
        return false;
    }

    uintptr_t associations = 0;
    uint32_t count = 0;
    uint32_t idOffset = 0;
    uintptr_t animBlock = 0;
    uintptr_t groups = 0;
    uint32_t groupCount = 0;
    __try {
        associations = *reinterpret_cast<const uintptr_t*>(group + 0x04);
        count = *reinterpret_cast<const uint32_t*>(group + 0x08);
        idOffset = *reinterpret_cast<const uint32_t*>(group + 0x0C);
        animBlock = *reinterpret_cast<const uintptr_t*>(group + 0x10);
        groups = *reinterpret_cast<const uintptr_t*>(kAnimAssocGroupsPtr);
        groupCount = *reinterpret_cast<const uint32_t*>(kNumAnimAssocDefinitions);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    uint32_t groupIndex = UINT32_MAX;
    if (groups && group >= groups) {
        const uintptr_t delta = group - groups;
        if (delta % kAssocGroupSize == 0 && delta / kAssocGroupSize < groupCount) {
            groupIndex = static_cast<uint32_t>(delta / kAssocGroupSize);
        }
    }

    if (associationsOut) {
        *associationsOut = associations;
    }
    if (countOut) {
        *countOut = count;
    }
    if (idOffsetOut) {
        *idOffsetOut = idOffset;
    }
    if (animBlockOut) {
        *animBlockOut = animBlock;
    }
    if (groupIndexOut) {
        *groupIndexOut = groupIndex;
    }
    return true;
}

void LogInvalidStaticAssociationOwnership(const char* reason, uintptr_t staticAssociation)
{
    static LONG logCount = 0;
    const LONG count = InterlockedIncrement(&logCount);
    if (count > 32) {
        return;
    }

    constexpr uintptr_t kAnimAssocGroupsPtr = 0x00B4EA34;
    constexpr uintptr_t kNumAnimAssocDefinitions = 0x00B4EA28;
    constexpr size_t kAssocGroupSize = 0x14;
    constexpr size_t kStaticAssocSize = 0x14;

    uintptr_t groups = 0;
    uint32_t groupCount = 0;
    __try {
        groups = *reinterpret_cast<const uintptr_t*>(kAnimAssocGroupsPtr);
        groupCount = *reinterpret_cast<const uint32_t*>(kNumAnimAssocDefinitions);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        groups = 0;
        groupCount = 0;
    }

    for (uint32_t i = 0; groups && i < groupCount; ++i) {
        const uintptr_t group = groups + static_cast<uintptr_t>(i) * kAssocGroupSize;
        uintptr_t associations = 0;
        uint32_t associationCount = 0;
        uint32_t idOffset = 0;
        uintptr_t animBlock = 0;
        if (!ReadAnimGroupSnapshot(group, &associations, &associationCount, &idOffset, &animBlock, nullptr) ||
            !associations || associationCount > 100000) {
            continue;
        }

        const uintptr_t bytes = static_cast<uintptr_t>(associationCount) * kStaticAssocSize;
        if (staticAssociation >= associations && staticAssociation - associations < bytes) {
            Log("anim lifecycle: invalid static owner=%s source=0x%08X currentGroup=%u slot=%u id=%u groupPtr=0x%08X array=0x%08X count=%u block=0x%08X tid=%lu",
                reason,
                staticAssociation,
                i,
                static_cast<uint32_t>((staticAssociation - associations) / kStaticAssocSize),
                idOffset + static_cast<uint32_t>((staticAssociation - associations) / kStaticAssocSize),
                group,
                associations,
                associationCount,
                animBlock,
                GetCurrentThreadId());
            return;
        }
    }

    LONG newestSequence = 0;
    AnimLifecycleRecord newest{};
    for (size_t i = 0; i < kAnimLifecycleHistorySize; ++i) {
        const LONG before = g_animLifecycleHistory[i].sequence;
        if (before <= 0) {
            continue;
        }

        AnimLifecycleRecord snapshot{};
        snapshot.group = g_animLifecycleHistory[i].group;
        snapshot.associations = g_animLifecycleHistory[i].associations;
        snapshot.caller = g_animLifecycleHistory[i].caller;
        snapshot.animBlock = g_animLifecycleHistory[i].animBlock;
        snapshot.associationCount = g_animLifecycleHistory[i].associationCount;
        snapshot.idOffset = g_animLifecycleHistory[i].idOffset;
        snapshot.groupIndex = g_animLifecycleHistory[i].groupIndex;
        snapshot.threadId = g_animLifecycleHistory[i].threadId;
        snapshot.tick = g_animLifecycleHistory[i].tick;
        const LONG after = g_animLifecycleHistory[i].sequence;
        if (before != after || snapshot.associationCount > 100000 || !snapshot.associations) {
            continue;
        }

        const uintptr_t bytes = static_cast<uintptr_t>(snapshot.associationCount) * kStaticAssocSize;
        if (staticAssociation >= snapshot.associations && staticAssociation - snapshot.associations < bytes && after > newestSequence) {
            newestSequence = after;
            newest = snapshot;
        }
    }

    if (newestSequence > 0) {
        char moduleName[MAX_PATH]{};
        const uintptr_t moduleBase = ModuleBaseFromAddress(newest.caller, moduleName, sizeof(moduleName));
        Log("anim lifecycle: invalid static owner=%s source=0x%08X FREED group=%u slot=%u id=%u groupPtr=0x%08X array=0x%08X count=%u block=0x%08X freedTid=%u currentTid=%lu ageMs=%lu caller=0x%08X %s+0x%X",
            reason,
            staticAssociation,
            newest.groupIndex,
            static_cast<uint32_t>((staticAssociation - newest.associations) / kStaticAssocSize),
            newest.idOffset + static_cast<uint32_t>((staticAssociation - newest.associations) / kStaticAssocSize),
            newest.group,
            newest.associations,
            newest.associationCount,
            newest.animBlock,
            newest.threadId,
            GetCurrentThreadId(),
            GetTickCount() - newest.tick,
            newest.caller,
            moduleName,
            moduleBase ? newest.caller - moduleBase : 0);
        return;
    }

    Log("anim lifecycle: invalid static owner=%s source=0x%08X no current/freed group match tid=%lu",
        reason,
        staticAssociation,
        GetCurrentThreadId());
}

extern "C" void __stdcall Bridge_LogAnimGroupDestroy(uintptr_t group, uintptr_t returnAddress)
{
    static LONG logCount = 0;
    uintptr_t associations = 0;
    uint32_t associationCount = 0;
    uint32_t idOffset = 0;
    uintptr_t animBlock = 0;
    uint32_t groupIndex = UINT32_MAX;
    if (!ReadAnimGroupSnapshot(group, &associations, &associationCount, &idOffset, &animBlock, &groupIndex)) {
        return;
    }

    if (associations && associationCount && associationCount <= 100000) {
        const LONG sequence = InterlockedIncrement(&g_animLifecycleCursor);
        AnimLifecycleRecord& record = g_animLifecycleHistory[static_cast<size_t>(sequence - 1) % kAnimLifecycleHistorySize];
        InterlockedExchange(&record.sequence, 0);
        record.group = group;
        record.associations = associations;
        record.caller = returnAddress;
        record.animBlock = animBlock;
        record.associationCount = associationCount;
        record.idOffset = idOffset;
        record.groupIndex = groupIndex;
        record.threadId = GetCurrentThreadId();
        record.tick = GetTickCount();
        MemoryBarrier();
        InterlockedExchange(&record.sequence, sequence);
    }

    const LONG count = InterlockedIncrement(&logCount);
    if (count <= 64) {
        char moduleName[MAX_PATH]{};
        const uintptr_t moduleBase = ModuleBaseFromAddress(returnAddress, moduleName, sizeof(moduleName));
        Log("anim lifecycle: DestroyAssociations group=%u groupPtr=0x%08X array=0x%08X count=%u idOffset=%u block=0x%08X tid=%lu caller=0x%08X %s+0x%X",
            groupIndex,
            group,
            associations,
            associationCount,
            idOffset,
            animBlock,
            GetCurrentThreadId(),
            returnAddress,
            moduleName,
            moduleBase ? returnAddress - moduleBase : 0);
    }
}

extern "C" void __stdcall Bridge_LogAnimBlockRemove(uint32_t blockIndex, uintptr_t returnAddress)
{
    static LONG logCount = 0;
    constexpr uintptr_t kAnimBlocks = 0x00B5D4A0;
    constexpr size_t kAnimBlockSize = 0x20;

    char blockName[17]{};
    uint8_t loaded = 0;
    int16_t refCount = 0;
    uint32_t firstAnim = 0;
    uint32_t animCount = 0;
    const uintptr_t block = kAnimBlocks + static_cast<uintptr_t>(blockIndex) * kAnimBlockSize;
    if (blockIndex < 10000 && IsReadableCommitted(block, kAnimBlockSize)) {
        __try {
            std::memcpy(blockName, reinterpret_cast<const void*>(block), 16);
            loaded = *reinterpret_cast<const uint8_t*>(block + 0x10);
            refCount = *reinterpret_cast<const int16_t*>(block + 0x12);
            firstAnim = *reinterpret_cast<const uint32_t*>(block + 0x14);
            animCount = *reinterpret_cast<const uint32_t*>(block + 0x18);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            blockName[0] = '\0';
        }
    }

    const LONG count = InterlockedIncrement(&logCount);
    if (count <= 64) {
        char moduleName[MAX_PATH]{};
        const uintptr_t moduleBase = ModuleBaseFromAddress(returnAddress, moduleName, sizeof(moduleName));
        Log("anim lifecycle: RemoveAnimBlock index=%u block=0x%08X name=%s loaded=%u refs=%d firstAnim=%u animCount=%u tid=%lu caller=0x%08X %s+0x%X",
            blockIndex,
            block,
            blockName,
            loaded,
            refCount,
            firstAnim,
            animCount,
            GetCurrentThreadId(),
            returnAddress,
            moduleName,
            moduleBase ? returnAddress - moduleBase : 0);
    }
}

extern "C" void __stdcall Bridge_LogEmptyAnimUpdate(uintptr_t updateData, uintptr_t clump)
{
    static LONG logCount = 0;
    const LONG count = InterlockedIncrement(&logCount);
    if (count > 32) {
        return;
    }

    uintptr_t clumpData = 0;
    uintptr_t firstAssociation = 0;
    __try {
        clumpData = *reinterpret_cast<const uintptr_t*>(0x00B4EA0C);
        if (clumpData) {
            firstAssociation = *reinterpret_cast<const uintptr_t*>(clumpData);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        clumpData = 0;
        firstAssociation = 0;
    }

    Log("anim lifecycle: skipped empty AnimBlendUpdateData updateData=0x%08X clump=0x%08X clumpData=0x%08X firstAssoc=0x%08X tid=%lu",
        updateData,
        clump,
        clumpData,
        firstAssociation,
        GetCurrentThreadId());
}

extern "C" bool __stdcall Bridge_ShouldBlockGroupBlendAnimation(uint32_t groupId, uint32_t animId)
{
    static LONG logCount = 0;

    constexpr uintptr_t kAnimAssocGroupsPtr = 0x00B4EA34;
    constexpr uintptr_t kNumAnimAssocDefinitions = 0x00B4EA28;
    constexpr size_t kAssocGroupSize = 0x14;
    constexpr size_t kStaticAssocSize = 0x14;

    if (!IsReadableCommitted(kAnimAssocGroupsPtr, sizeof(uintptr_t)) ||
        !IsReadableCommitted(kNumAnimAssocDefinitions, sizeof(uint32_t))) {
        return false;
    }

    uintptr_t groups = 0;
    uint32_t groupCount = 0;
    __try {
        groups = *reinterpret_cast<const uintptr_t*>(kAnimAssocGroupsPtr);
        groupCount = *reinterpret_cast<const uint32_t*>(kNumAnimAssocDefinitions);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    if (!groups || groupId >= groupCount || !IsReadableCommitted(groups + groupId * kAssocGroupSize, kAssocGroupSize)) {
        return false;
    }

    uintptr_t group = groups + groupId * kAssocGroupSize;
    uintptr_t associations = 0;
    uint32_t animCount = 0;
    uint32_t idOffset = 0;
    __try {
        associations = *reinterpret_cast<const uintptr_t*>(group + 0x04);
        animCount = *reinterpret_cast<const uint32_t*>(group + 0x08);
        idOffset = *reinterpret_cast<const uint32_t*>(group + 0x0C);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    if (!associations || animId < idOffset || animId - idOffset >= animCount) {
        return false;
    }

    const uintptr_t staticAssociation = associations + static_cast<uintptr_t>(animId - idOffset) * kStaticAssocSize;
    if (Bridge_IsValidStaticAssociation(staticAssociation)) {
        return false;
    }

    LogInvalidStaticAssociationOwnership("blend-entry", staticAssociation);

    uint16_t numBlendNodes = 0;
    uint16_t flags = 0;
    uint32_t blendSeqs = 0;
    uint32_t blendHier = 0;
    if (IsReadableCommitted(staticAssociation, kStaticAssocSize)) {
        __try {
            numBlendNodes = *reinterpret_cast<const uint16_t*>(staticAssociation + 0x04);
            flags = *reinterpret_cast<const uint16_t*>(staticAssociation + 0x0A);
            blendSeqs = *reinterpret_cast<const uint32_t*>(staticAssociation + 0x0C);
            blendHier = *reinterpret_cast<const uint32_t*>(staticAssociation + 0x10);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    const LONG count = InterlockedIncrement(&logCount);
    if (count <= 16) {
        Log("anim blend guard: blocked invalid target group=%u animId=%u offset=%u count=%u static=0x%08X nodes=%u flags=0x%04X seqs=0x%08X hier=0x%08X",
            groupId,
            animId,
            idOffset,
            animCount,
            staticAssociation,
            numBlendNodes,
            flags,
            blendSeqs,
            blendHier);
    }

    return true;
}

extern "C" void __stdcall Bridge_RepairInvalidStaticAssociation(uintptr_t runtimeAssociation, uintptr_t staticAssociation)
{
    static LONG logCount = 0;
    const LONG count = InterlockedIncrement(&logCount);

    uint16_t numBlendNodes = 0;
    uint16_t animId = 0xFFFF;
    uint16_t animGroupId = 0xFFFF;
    uint16_t flags = 0;
    uint32_t blendSeqs = 0;
    uint32_t blendHier = 0;

    if (IsReadableCommitted(staticAssociation, 0x14)) {
        __try {
            numBlendNodes = *reinterpret_cast<const uint16_t*>(staticAssociation + 0x04);
            animId = *reinterpret_cast<const uint16_t*>(staticAssociation + 0x06);
            animGroupId = *reinterpret_cast<const uint16_t*>(staticAssociation + 0x08);
            flags = *reinterpret_cast<const uint16_t*>(staticAssociation + 0x0A);
            blendSeqs = *reinterpret_cast<const uint32_t*>(staticAssociation + 0x0C);
            blendHier = *reinterpret_cast<const uint32_t*>(staticAssociation + 0x10);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    LogInvalidStaticAssociationOwnership("static-init", staticAssociation);

    const uintptr_t fallbackHier = Bridge_IsValidAnimHierarchy(g_lastValidAnimHierarchy) ? g_lastValidAnimHierarchy : 0;
    constexpr uint16_t kAnimationBlendAutoRemove = 0x0004;
    constexpr uint16_t kAnimationReferenceBlock = 0x4000;
    constexpr float kZeroBlend = 0.0f;
    constexpr float kFadeOutNow = -1.0f;
    const uint16_t neutralizedFlags = fallbackHier ? kAnimationBlendAutoRemove : static_cast<uint16_t>(kAnimationBlendAutoRemove | kAnimationReferenceBlock);

    if (IsReadableCommitted(runtimeAssociation, 0x3C)) {
        __try {
            *reinterpret_cast<uint16_t*>(runtimeAssociation + 0x0C) = 0;                 // m_NumBlendNodes
            *reinterpret_cast<uint16_t*>(runtimeAssociation + 0x0E) = animGroupId;       // m_AnimGroupId
            *reinterpret_cast<uint32_t*>(runtimeAssociation + 0x10) = 0;                 // m_BlendNodes
            *reinterpret_cast<uint32_t*>(runtimeAssociation + 0x14) = static_cast<uint32_t>(fallbackHier);
            *reinterpret_cast<float*>(runtimeAssociation + 0x18) = kZeroBlend;           // m_BlendAmount
            *reinterpret_cast<float*>(runtimeAssociation + 0x1C) = kFadeOutNow;          // m_BlendDelta
            *reinterpret_cast<float*>(runtimeAssociation + 0x20) = 0.0f;                 // m_CurrentTime
            *reinterpret_cast<float*>(runtimeAssociation + 0x24) = 0.0f;                 // m_Speed
            *reinterpret_cast<float*>(runtimeAssociation + 0x28) = 0.0f;                 // m_TimeStep
            *reinterpret_cast<uint16_t*>(runtimeAssociation + 0x2C) = animId;            // m_AnimId
            *reinterpret_cast<uint16_t*>(runtimeAssociation + 0x2E) = neutralizedFlags;  // m_Flags
            *reinterpret_cast<uint32_t*>(runtimeAssociation + 0x30) = 0;                 // m_nCallbackType
            *reinterpret_cast<uintptr_t*>(runtimeAssociation + 0x34) = 0;                // m_pCallbackFunc
            *reinterpret_cast<uintptr_t*>(runtimeAssociation + 0x38) = 0;                // m_pCallbackData
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }
    MarkNeutralizedAnimAssociation(runtimeAssociation);

    if (count <= 4) {
        Log("static assoc guard: neutralized invalid source=0x%08X runtime=0x%08X srcNodes=%u animId=%u group=%u flags=0x%04X neutralFlags=0x%04X seqs=0x%08X hier=0x%08X fallbackHier=0x%08X",
            staticAssociation,
            runtimeAssociation,
            numBlendNodes,
            animId,
            animGroupId,
            flags,
            neutralizedFlags,
            blendSeqs,
            blendHier,
            fallbackHier);
        if (count <= 1) {
            LogMemoryRegion("static-assoc-source", staticAssociation);
            LogMemoryRegion("static-assoc-seqs", blendSeqs);
            LogMemoryRegion("static-assoc-hier", blendHier);
            LogMemoryRegion("static-assoc-fallback-hier", fallbackHier);
        }
    }
}

extern "C" bool __stdcall Bridge_PrepareAnimAssociationUpdate(uintptr_t association)
{
    static LONG logCount = 0;

    if (!IsNeutralizedAnimAssociation(association)) {
        return true;
    }

    if (!IsReadableCommitted(association, 0x3C)) {
        const LONG count = InterlockedIncrement(&logCount);
        if (count <= 8) {
            Log("anim assoc guard: skipped unreadable neutralized association=0x%08X", association);
            LogMemoryRegion("anim-assoc", association);
        }
        return false;
    }

    uint16_t numBlendNodes = 0;
    uintptr_t blendNodes = 0;
    uintptr_t blendHier = 0;
    uint16_t flags = 0;
    __try {
        numBlendNodes = *reinterpret_cast<const uint16_t*>(association + 0x0C);
        blendNodes = *reinterpret_cast<const uintptr_t*>(association + 0x10);
        blendHier = *reinterpret_cast<const uintptr_t*>(association + 0x14);
        flags = *reinterpret_cast<const uint16_t*>(association + 0x2E);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        const LONG count = InterlockedIncrement(&logCount);
        if (count <= 8) {
            Log("anim assoc guard: read exception while skipping neutralized association=0x%08X", association);
        }
        return false;
    }

    uintptr_t next = 0;
    uintptr_t prev = 0;
    __try {
        next = *reinterpret_cast<const uintptr_t*>(association + 0x00);
        prev = *reinterpret_cast<const uintptr_t*>(association + 0x04);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        next = 0;
        prev = 0;
    }

    constexpr uint16_t kAnimationBlendAutoRemove = 0x0004;
    constexpr float kFadeOutNow = -1.0f;
    bool patchedAutoRemove = false;
    if (IsWritableCommitted(association + 0x1C, sizeof(float)) &&
        IsWritableCommitted(association + 0x2E, sizeof(uint16_t))) {
        __try {
            *reinterpret_cast<float*>(association + 0x18) = 0.0f;
            *reinterpret_cast<float*>(association + 0x1C) = kFadeOutNow;
            *reinterpret_cast<uint16_t*>(association + 0x2E) = static_cast<uint16_t>(flags | kAnimationBlendAutoRemove);
            patchedAutoRemove = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }
    ConsumeNeutralizedAnimAssociation(association);

    const LONG count = InterlockedIncrement(&logCount);
    if (count <= 8) {
        Log("anim assoc guard: auto-removing neutralized association assoc=0x%08X nodes=%u nodePtr=0x%08X hier=0x%08X flags=0x%04X patched=%u next=0x%08X prev=0x%08X",
            association,
            numBlendNodes,
            blendNodes,
            blendHier,
            flags,
            patchedAutoRemove ? 1 : 0,
            next,
            prev);
    }
    return true;
}

extern "C" bool __stdcall Bridge_PrepareAnimFrameUpdateData(uintptr_t updateData)
{
    static LONG logCount = 0;

    constexpr size_t kUpdateDataSize = 4 + 12 * sizeof(uintptr_t);
    constexpr size_t kNodeSize = 0x18;
    if (!IsReadableCommitted(updateData, kUpdateDataSize)) {
        const LONG count = InterlockedIncrement(&logCount);
        if (count <= 4) {
            Log("anim frame guard: skipped unreadable update data=0x%08X", updateData);
            LogMemoryRegion("anim-frame-update-data", updateData);
        }
        return false;
    }

    uintptr_t firstNodeArray = 0;
    __try {
        firstNodeArray = *reinterpret_cast<const uintptr_t*>(updateData + 4);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    if (!firstNodeArray) {
        const LONG count = InterlockedIncrement(&logCount);
        if (count <= 4) {
            const uint32_t includePartial = *reinterpret_cast<const uint32_t*>(updateData);
            Log("anim frame guard: skipped empty BlendNodeArrays updateData=0x%08X includePartial=%u", updateData, includePartial);
        }
        return false;
    }

    for (int i = 0; i < 12; ++i) {
        uintptr_t node = 0;
        __try {
            node = *reinterpret_cast<const uintptr_t*>(updateData + 4 + i * sizeof(uintptr_t));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            node = 0;
        }

        if (!node) {
            return true;
        }

        bool valid = IsReadableCommitted(node, kNodeSize);
        uintptr_t sequence = 0;
        uintptr_t association = 0;
        if (valid) {
            __try {
                sequence = *reinterpret_cast<const uintptr_t*>(node + 0x10);
                association = *reinterpret_cast<const uintptr_t*>(node + 0x14);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                valid = false;
            }
        }

        if (valid && sequence && !IsReadableCommitted(sequence, 0x10)) {
            valid = false;
        }

        if (valid && association && !IsReadableCommitted(association, 0x30)) {
            valid = false;
        }

        if (valid) {
            continue;
        }

        const LONG count = InterlockedIncrement(&logCount);
        if (count <= 4) {
            Log("anim frame guard: skipped frame with invalid BlendNodeArrays updateData=0x%08X index=%d node=0x%08X seq=0x%08X assoc=0x%08X",
                updateData,
                i,
                node,
                sequence,
                association);
            LogMemoryRegion("anim-frame-node", node);
            LogMemoryRegion("anim-frame-seq", sequence);
            LogMemoryRegion("anim-frame-assoc", association);
        }

        return false;
    }

    return true;
}

extern "C" bool __stdcall Bridge_ShouldSkipRpAnimBlendClumpInit(uintptr_t clump, uintptr_t returnAddress, uintptr_t stack)
{
    uint32_t pluginOffset = 0;
    uint32_t firstLink = 0;
    uint32_t frame = 0;
    uint32_t firstAtomic = 0;
    uint32_t geometry = 0;
    const uintptr_t sentinel = clump + 0x08;

    bool valid = clump >= 0x10000 &&
        IsReadableCommitted(clump, 0x20) &&
        SafeReadU32(kRwClumpAnimPluginOffset, &pluginOffset) &&
        pluginOffset > 0 &&
        pluginOffset < 0x1000 &&
        IsWritableCommitted(clump + pluginOffset, sizeof(uintptr_t)) &&
        SafeReadU32(clump + 0x08, &firstLink) &&
        firstLink != 0 &&
        SafeReadU32(clump + 0x04, &frame) &&
        frame >= 0x10000 &&
        IsReadableCommitted(frame, 0x44);

    if (valid && firstLink == sentinel) {
        return false;
    } else if (valid) {
        uint32_t nextLink = 0;
        const uintptr_t atomic = firstLink >= 0x40 ? firstLink - 0x40 : 0;
        valid = firstLink >= 0x10000 &&
            IsReadableCommitted(firstLink, 0x08) &&
            SafeReadU32(firstLink, &nextLink) &&
            atomic >= 0x10000 &&
            IsReadableCommitted(atomic, 0x1C) &&
            SafeReadU32(atomic + 0x18, &geometry) &&
            geometry >= 0x10000 &&
            IsReadableCommitted(geometry, 0x20) &&
            (nextLink == sentinel || IsReadableCommitted(nextLink, sizeof(uintptr_t)));
        firstAtomic = static_cast<uint32_t>(atomic);
    }

    if (valid) {
        return false;
    }

    const LONG count = InterlockedIncrement(&g_rpAnimBlendClumpInitGuardLogs);
    if (count <= 32) {
        char returnModule[MAX_PATH]{};
        const uintptr_t returnBase = ModuleBaseFromAddress(returnAddress, returnModule, sizeof(returnModule));
        Log("rp anim clump init guard: skipped clump=0x%08X pluginOffset=0x%08X firstLink=0x%08X sentinel=0x%08X firstAtomic=0x%08X geometry=0x%08X frame=0x%08X return=0x%08X (%s+0x%X) stack=0x%08X",
            clump,
            pluginOffset,
            firstLink,
            sentinel,
            firstAtomic,
            geometry,
            frame,
            returnAddress,
            returnModule,
            returnBase ? returnAddress - returnBase : 0,
            stack);
        LogMemoryRegion("rp-anim-clump", clump);
        if (clump >= 0x10000) {
            LogDwords("rp-anim-clump-dwords", clump, 8);
        }
        if (firstLink >= 0x10000) {
            LogDwords("rp-anim-clump-link", firstLink, 4);
        }
        if (firstAtomic >= 0x10000) {
            LogDwords("rp-anim-first-atomic", firstAtomic, 8);
        }
        if (stack) {
            LogStackModules(stack);
        }
    }

    return true;
}

bool IsCachedExecutableRwClumpCallback(uintptr_t callback)
{
    if (callback < 0x10000) {
        return false;
    }

    const size_t slot = ((callback >> 4) ^ (callback >> 12)) & (kRwClumpCallbackCacheSize - 1);
    if (g_rwClumpExecutableCallbacks[slot].load(std::memory_order_relaxed) == callback) {
        return true;
    }
    if (!IsExecutableCommitted(callback)) {
        return false;
    }

    g_rwClumpExecutableCallbacks[slot].store(callback, std::memory_order_relaxed);
    return true;
}

extern "C" bool __stdcall Bridge_IsSafeRpClumpForAllAtomicsCall(
    uintptr_t clump,
    uintptr_t callback,
    uintptr_t data,
    uintptr_t returnAddress,
    uintptr_t stack)
{
    const uintptr_t sentinel = clump + 0x08;
    uint32_t firstLink = 0;
    bool valid = false;

    if (clump >= 0x10000) {
        __try {
            firstLink = *reinterpret_cast<const uint32_t*>(clump + 0x08);
            if (firstLink == sentinel) {
                return true;
            }

            if (firstLink >= 0x10000 && IsCachedExecutableRwClumpCallback(callback)) {
                const uint32_t nextLink = *reinterpret_cast<const uint32_t*>(firstLink);
                if (nextLink == sentinel) {
                    valid = true;
                } else if (nextLink >= 0x10000) {
                    volatile const uint32_t nextValue = *reinterpret_cast<volatile const uint32_t*>(nextLink);
                    (void)nextValue;
                    valid = true;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            valid = false;
        }
    }

    if (valid) {
        return true;
    }

    const LONG count = InterlockedIncrement(&g_rwClumpForAllAtomicsGuardLogs);
    if (count <= 32) {
        char returnModule[MAX_PATH]{};
        const uintptr_t returnBase = ModuleBaseFromAddress(returnAddress, returnModule, sizeof(returnModule));
        char callbackModule[MAX_PATH]{};
        const uintptr_t callbackBase = ModuleBaseFromAddress(callback, callbackModule, sizeof(callbackModule));

        Log("rw clump guard: blocked RpClumpForAllAtomics clump=0x%08X callback=0x%08X data=0x%08X firstLink=0x%08X sentinel=0x%08X return=0x%08X (%s+0x%X) callbackModule=%s+0x%X stack=0x%08X",
            clump,
            callback,
            data,
            firstLink,
            sentinel,
            returnAddress,
            returnModule,
            returnBase ? returnAddress - returnBase : 0,
            callbackModule,
            callbackBase ? callback - callbackBase : 0,
            stack);
        LogMemoryRegion("rw-clump", clump);
        LogMemoryRegion("rw-clump-callback", callback);
        if (clump >= 0x10000) {
            LogDwords("rw-clump-dwords", clump, 8);
        }
        if (stack) {
            LogStackModules(stack);
        }
    }

    return false;
}

void InstallRwClumpForAllAtomicsGuard()
{
#if defined(_M_IX86)
    constexpr uintptr_t patchAddress = kRpClumpForAllAtomics;
    constexpr uintptr_t continueAddress = kRpClumpForAllAtomics + 5;
    static const uint8_t expectedBytes[] = {
        0x8B, 0x44, 0x24, 0x04, // mov eax, [esp+4]
        0x53                    // push ebx
    };

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("rw clump guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("rw clump guard: read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_RpClumpForAllAtomics_Guard);
    if (currentTarget == guardTarget) {
        Log("rw clump guard: already installed at RpClumpForAllAtomics");
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("rw clump guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X currentTarget=0x%08X",
            patchAddress,
            current[0], current[1], current[2], current[3], current[4],
            currentTarget);
        return;
    }

    g_rwClumpForAllAtomicsContinue = continueAddress;
    if (WriteRel32Jump(patchAddress, guardTarget)) {
        Log("rw clump guard: installed at RpClumpForAllAtomics target=0x%08X continue=0x%08X",
            guardTarget,
            g_rwClumpForAllAtomicsContinue);
    }
#else
    Log("rw clump guard: unsupported architecture");
#endif
}

void InstallRpAnimBlendClumpInitGuard()
{
#if defined(_M_IX86)
    constexpr uintptr_t patchAddress = kRpAnimBlendClumpInit;
    constexpr uintptr_t continueAddress = kRpAnimBlendClumpInit + 5;
    static const uint8_t expectedBytes[] = {
        0x56,                   // push esi
        0x8B, 0x74, 0x24, 0x08  // mov esi, [esp+8]
    };

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("rp anim clump init guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("rp anim clump init guard: read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_RpAnimBlendClumpInit_Guard);
    if (currentTarget == guardTarget) {
        Log("rp anim clump init guard: already installed at RpAnimBlendClumpInit");
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("rp anim clump init guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X currentTarget=0x%08X",
            patchAddress,
            current[0], current[1], current[2], current[3], current[4],
            currentTarget);
        return;
    }

    g_rpAnimBlendClumpInitContinue = continueAddress;
    if (WriteRel32Jump(patchAddress, guardTarget)) {
        Log("rp anim clump init guard: installed at RpAnimBlendClumpInit target=0x%08X continue=0x%08X",
            guardTarget,
            g_rpAnimBlendClumpInitContinue);
    }
#else
    Log("rp anim clump init guard: unsupported architecture");
#endif
}

void InstallAnimUncompressNullGuard()
{
#if defined(_M_IX86)
    constexpr uintptr_t patchAddress = 0x004D41C0;
    constexpr uintptr_t continueAddress = 0x004D41C5;
    static const uint8_t expectedBytes[] = {
        0x57,                   // push edi
        0x8B, 0x7C, 0x24, 0x08  // mov edi, [esp+8]
    };

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("anim guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("anim guard: read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_AnimUncompress_NullGuard);
    if (currentTarget == guardTarget) {
        Log("anim guard: already installed at CAnimManager::UncompressAnimation");
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("anim guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X currentTarget=0x%08X",
            patchAddress, current[0], current[1], current[2], current[3], current[4], currentTarget);
        return;
    }

    g_animUncompressContinue = continueAddress;
    if (WriteRel32Jump(patchAddress, guardTarget)) {
        Log("anim guard: installed at CAnimManager::UncompressAnimation target=0x%08X continue=0x%08X",
            guardTarget, g_animUncompressContinue);
    }
#else
    Log("anim guard: unsupported architecture");
#endif
}

void InstallAnimStaticAssocInitGuard()
{
#if defined(_M_IX86)
    constexpr uintptr_t patchAddress = 0x004CEEC0;
    constexpr uintptr_t continueAddress = 0x004CEEC8;
    static const uint8_t expectedBytes[] = {
        0x56,                   // push esi
        0x8B, 0xF1,             // mov esi, ecx
        0x57,                   // push edi
        0x8B, 0x7C, 0x24, 0x0C  // mov edi, [esp+0Ch]
    };

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("static assoc guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("static assoc guard: read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_AnimStaticAssocInit_Guard);
    if (currentTarget == guardTarget) {
        Log("static assoc guard: already installed at CAnimBlendAssociation::Init(static)");
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("static assoc guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X %02X %02X %02X currentTarget=0x%08X",
            patchAddress,
            current[0], current[1], current[2], current[3],
            current[4], current[5], current[6], current[7],
            currentTarget);
        return;
    }

    g_animStaticAssocInitContinue = continueAddress;
    uint8_t patch[sizeof(expectedBytes)]{};
    patch[0] = 0xE9;
    const int32_t rel = static_cast<int32_t>(guardTarget - (patchAddress + 5));
    std::memcpy(patch + 1, &rel, sizeof(rel));
    for (size_t i = 5; i < sizeof(patch); ++i) {
        patch[i] = 0x90;
    }

    if (WriteBytesWithProtect(patchAddress, patch, sizeof(patch))) {
        Log("static assoc guard: installed at CAnimBlendAssociation::Init(static) target=0x%08X continue=0x%08X",
            guardTarget, g_animStaticAssocInitContinue);
    }
#else
    Log("static assoc guard: unsupported architecture");
#endif
}

void InstallAnimUpdateBlendGuard()
{
#if defined(_M_IX86)
    constexpr uintptr_t patchAddress = 0x004D1490;
    constexpr uintptr_t continueAddress = 0x004D1497;
    static const uint8_t expectedBytes[] = {
        0xD9, 0x44, 0x24, 0x04, // fld dword ptr [esp+4]
        0x56,                   // push esi
        0x8B, 0xF1              // mov esi, ecx
    };

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("anim assoc guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("anim assoc guard: read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_AnimUpdateBlend_Guard);
    if (currentTarget == guardTarget) {
        Log("anim assoc guard: already installed at CAnimBlendAssociation::UpdateBlend");
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("anim assoc guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X %02X %02X currentTarget=0x%08X",
            patchAddress,
            current[0], current[1], current[2], current[3],
            current[4], current[5], current[6],
            currentTarget);
        return;
    }

    g_animUpdateBlendContinue = continueAddress;
    uint8_t patch[sizeof(expectedBytes)]{};
    patch[0] = 0xE9;
    const int32_t rel = static_cast<int32_t>(guardTarget - (patchAddress + 5));
    std::memcpy(patch + 1, &rel, sizeof(rel));
    for (size_t i = 5; i < sizeof(patch); ++i) {
        patch[i] = 0x90;
    }

    if (WriteBytesWithProtect(patchAddress, patch, sizeof(patch))) {
        Log("anim assoc guard: installed at CAnimBlendAssociation::UpdateBlend target=0x%08X continue=0x%08X",
            guardTarget,
            g_animUpdateBlendContinue);
    }
#else
    Log("anim assoc guard: unsupported architecture");
#endif
}

void InstallAnimBlendGroupGuard()
{
#if defined(_M_IX86)
    constexpr uintptr_t patchAddress = 0x004D4610;
    constexpr uintptr_t continueAddress = 0x004D4617;
    static const uint8_t expectedBytes[] = {
        0x83, 0xEC, 0x14,       // sub esp, 14h
        0x8B, 0x4C, 0x24, 0x18  // mov ecx, [esp+18h]
    };

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("anim blend guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("anim blend guard: read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_AnimBlendGroup_Guard);
    if (currentTarget == guardTarget) {
        Log("anim blend guard: already installed at CAnimManager::BlendAnimation(group)");
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("anim blend guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X %02X %02X currentTarget=0x%08X",
            patchAddress,
            current[0], current[1], current[2], current[3],
            current[4], current[5], current[6],
            currentTarget);
        return;
    }

    g_animBlendGroupContinue = continueAddress;
    uint8_t patch[sizeof(expectedBytes)]{};
    patch[0] = 0xE9;
    const int32_t rel = static_cast<int32_t>(guardTarget - (patchAddress + 5));
    std::memcpy(patch + 1, &rel, sizeof(rel));
    for (size_t i = 5; i < sizeof(patch); ++i) {
        patch[i] = 0x90;
    }

    if (WriteBytesWithProtect(patchAddress, patch, sizeof(patch))) {
        Log("anim blend guard: installed at CAnimManager::BlendAnimation(group) target=0x%08X continue=0x%08X",
            guardTarget,
            g_animBlendGroupContinue);
    }
#else
    Log("anim blend guard: unsupported architecture");
#endif
}

void InstallAnimFrameUpdateSkinnedVelocityGuard()
{
#if defined(_M_IX86)
    constexpr uintptr_t patchAddress = 0x004D1680;
    constexpr uintptr_t continueAddress = 0x004D1685;
    static const uint8_t expectedBytes[] = {
        0x83, 0xEC, 0x68, // sub esp, 68h
        0x55,             // push ebp
        0x56              // push esi
    };

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("anim frame guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("anim frame guard: read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_AnimFrameUpdateSkinnedVelocity_Guard);
    if (currentTarget == guardTarget) {
        Log("anim frame guard: already installed at FrameUpdateCallBackSkinnedWithVelocityExtraction");
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("anim frame guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X currentTarget=0x%08X",
            patchAddress,
            current[0], current[1], current[2], current[3], current[4],
            currentTarget);
        return;
    }

    g_animFrameUpdateSkinnedVelocityContinue = continueAddress;
    if (WriteRel32Jump(patchAddress, guardTarget)) {
        Log("anim frame guard: installed at FrameUpdateCallBackSkinnedWithVelocityExtraction target=0x%08X continue=0x%08X",
            guardTarget,
            g_animFrameUpdateSkinnedVelocityContinue);
    }
#else
    Log("anim frame guard: unsupported architecture");
#endif
}

void InstallAnimFrameUpdateSkinnedGuard()
{
#if defined(_M_IX86)
    constexpr uintptr_t patchAddress = 0x004D2B90;
    constexpr uintptr_t continueAddress = 0x004D2B98;
    static const uint8_t expectedBytes[] = {
        0x83, 0xEC, 0x3C,       // sub esp, 3Ch
        0x55,                   // push ebp
        0x8B, 0x6C, 0x24, 0x44  // mov ebp, [esp+44h]
    };

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("anim frame guard: skinned patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("anim frame guard: skinned read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_AnimFrameUpdateSkinned_Guard);
    if (currentTarget == guardTarget) {
        Log("anim frame guard: already installed at FrameUpdateCallBackSkinned");
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("anim frame guard: skinned unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X %02X %02X %02X currentTarget=0x%08X",
            patchAddress,
            current[0], current[1], current[2], current[3],
            current[4], current[5], current[6], current[7],
            currentTarget);
        return;
    }

    g_animFrameUpdateSkinnedContinue = continueAddress;
    uint8_t patch[sizeof(expectedBytes)]{};
    patch[0] = 0xE9;
    const int32_t rel = static_cast<int32_t>(guardTarget - (patchAddress + 5));
    std::memcpy(patch + 1, &rel, sizeof(rel));
    for (size_t i = 5; i < sizeof(patch); ++i) {
        patch[i] = 0x90;
    }

    if (WriteBytesWithProtect(patchAddress, patch, sizeof(patch))) {
        Log("anim frame guard: installed at FrameUpdateCallBackSkinned target=0x%08X continue=0x%08X",
            guardTarget,
            g_animFrameUpdateSkinnedContinue);
    }
#else
    Log("anim frame guard: unsupported architecture");
#endif
}

void InstallAnimEmptyUpdateGuard()
{
#if defined(_M_IX86)
    constexpr uintptr_t patchAddress = 0x004D360E;
    constexpr uintptr_t continueAddress = 0x004D3616;
    constexpr uintptr_t emptyAddress = 0x004D3715;
    static const uint8_t expectedBytes[] = {
        0xC7, 0x44, 0x9C, 0x1C, 0x00, 0x00, 0x00, 0x00
    };

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("anim empty update guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("anim empty update guard: read exception at 0x%08X", patchAddress);
        return;
    }

    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_AnimClumpFinalizeNodes_Guard);
    if (DecodeRel32JumpTarget(patchAddress) == guardTarget) {
        Log("anim empty update guard: already installed at RpAnimBlendClumpUpdateAnimations");
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("anim empty update guard: unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X %02X %02X %02X",
            patchAddress,
            current[0], current[1], current[2], current[3],
            current[4], current[5], current[6], current[7]);
        return;
    }

    g_animClumpFinalizeNodesContinue = continueAddress;
    g_animClumpFinalizeNodesEmpty = emptyAddress;
    uint8_t patch[sizeof(expectedBytes)]{};
    patch[0] = 0xE9;
    const int32_t rel = static_cast<int32_t>(guardTarget - (patchAddress + 5));
    std::memcpy(patch + 1, &rel, sizeof(rel));
    for (size_t i = 5; i < sizeof(patch); ++i) {
        patch[i] = 0x90;
    }

    if (WriteBytesWithProtect(patchAddress, patch, sizeof(patch))) {
        Log("anim empty update guard: installed once-per-clump check target=0x%08X continue=0x%08X empty=0x%08X",
            guardTarget,
            g_animClumpFinalizeNodesContinue,
            g_animClumpFinalizeNodesEmpty);
    }
#else
    Log("anim empty update guard: unsupported architecture");
#endif
}

void InstallAnimLifecycleDiagnostics()
{
#if defined(_M_IX86)
    {
        constexpr uintptr_t patchAddress = 0x004CDFF0;
        constexpr uintptr_t continueAddress = 0x004CDFF6;
        static const uint8_t expectedBytes[] = {
            0x56, 0x8B, 0xF1, 0x8B, 0x4E, 0x04
        };
        uint8_t current[sizeof(expectedBytes)]{};
        __try {
            std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Log("anim lifecycle: DestroyAssociations read exception at 0x%08X", patchAddress);
            current[0] = 0;
        }

        const uintptr_t target = reinterpret_cast<uintptr_t>(Bridge_AnimDestroyAssociations_Diagnostic);
        if (DecodeRel32JumpTarget(patchAddress) == target) {
            Log("anim lifecycle: DestroyAssociations diagnostic already installed");
        } else if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
            Log("anim lifecycle: DestroyAssociations unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X %02X",
                patchAddress,
                current[0], current[1], current[2], current[3], current[4], current[5]);
        } else {
            g_animDestroyAssociationsContinue = continueAddress;
            uint8_t patch[sizeof(expectedBytes)]{};
            patch[0] = 0xE9;
            const int32_t rel = static_cast<int32_t>(target - (patchAddress + 5));
            std::memcpy(patch + 1, &rel, sizeof(rel));
            patch[5] = 0x90;
            if (WriteBytesWithProtect(patchAddress, patch, sizeof(patch))) {
                Log("anim lifecycle: installed DestroyAssociations diagnostic target=0x%08X continue=0x%08X",
                    target,
                    g_animDestroyAssociationsContinue);
            }
        }
    }

    {
        constexpr uintptr_t patchAddress = 0x004D3F40;
        constexpr uintptr_t continueAddress = 0x004D3F45;
        static const uint8_t expectedBytes[] = {
            0xA1, 0x28, 0xEA, 0xB4, 0x00
        };
        uint8_t current[sizeof(expectedBytes)]{};
        __try {
            std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Log("anim lifecycle: RemoveAnimBlock read exception at 0x%08X", patchAddress);
            current[0] = 0;
        }

        const uintptr_t target = reinterpret_cast<uintptr_t>(Bridge_AnimRemoveBlock_Diagnostic);
        if (DecodeRel32JumpTarget(patchAddress) == target) {
            Log("anim lifecycle: RemoveAnimBlock diagnostic already installed");
        } else if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
            Log("anim lifecycle: RemoveAnimBlock unexpected bytes at 0x%08X old=%02X %02X %02X %02X %02X",
                patchAddress,
                current[0], current[1], current[2], current[3], current[4]);
        } else {
            g_animRemoveBlockContinue = continueAddress;
            if (WriteRel32Jump(patchAddress, target)) {
                Log("anim lifecycle: installed RemoveAnimBlock diagnostic target=0x%08X continue=0x%08X",
                    target,
                    g_animRemoveBlockContinue);
            }
        }
    }
#else
    Log("anim lifecycle: unsupported architecture");
#endif
}

