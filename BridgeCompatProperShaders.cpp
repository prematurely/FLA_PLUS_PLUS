#include "FLACompatBridgeInternal.h"

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_ProperShadersAddTxdSlot_Combined()
{
    __asm
    {
        // Insert our continuation below AddTxdSlot's saved ESI. FLA then pops
        // the real saved ESI and returns here with [caller-ret, name] intact.
        pop ecx
        push afterFla
        push ecx
        jmp dword ptr [g_flaAddTxdSlotThunk]

    afterFla:
        jmp dword ptr [g_properShadersAddTxdSlotThunk]
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_TxdLoadDictionaryWriteGuard()
{
    __asm
    {
        push eax
        push ecx
        push edx
        push 4
        push esi
        call Bridge_IsWritableMemory
        test al, al
        pop edx
        pop ecx
        pop eax
        jz invalidSlot

        mov dword ptr [esi], eax
        test eax, eax
        jz noDictionary

        push ebx
        push 0x00731E18
        retn

    noDictionary:
        push 0x00731E20
        retn

    invalidSlot:
        push eax
        push ecx
        push edx
        push ebx
        push eax
        push esi
        call Bridge_LogInvalidTxdLoadDictionaryWrite
        pop edx
        pop ecx
        pop eax
        xor eax, eax
        push 0x00731E2D
        retn
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_RwTexDictionaryFindNamedTexture_DictGuard()
{
    __asm
    {
        add eax, 8
        push ecx
        push edx
        push eax
        push 4
        push eax
        call Bridge_IsReadableMemory
        test al, al
        pop eax
        pop edx
        pop ecx
        jz invalidDict

        push ebp
        push esi
        push edi
        mov ebx, dword ptr [eax]
        mov dword ptr [esp + 0x14], eax
        push 0x007F3A01
        retn

    invalidDict:
        pop ebx
        xor eax, eax
        retn
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_RwTexDictionaryFindNamedTexture_NameGuard()
{
    __asm
    {
        push eax
        push edx
        push ecx
        push 1
        push ecx
        call Bridge_IsReadableMemory
        test al, al
        pop ecx
        pop edx
        pop eax
        jz invalidName

        mov esi, ecx
        mov edi, ebp
        mov cl, byte ptr [esi]
        push 0x007F3A19
        retn

    invalidName:
        push 0x007F3A5C
        retn
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_RwTexDictionaryFindNamedTexture_LinkGuard()
{
    __asm
    {
        push eax
        push edx
        push ecx
        push ebx
        push 4
        push ebx
        call Bridge_IsReadableMemory
        test al, al
        pop ebx
        pop ecx
        pop edx
        pop eax
        jz invalidLink

        mov ebx, dword ptr [ebx]
        mov eax, dword ptr [esp + 0x14]
        push 0x007F3A58
        retn

    invalidLink:
        push 0x007F3A5C
        retn
    }
}
#endif

#if defined(_M_IX86)
extern "C" __declspec(naked) void Bridge_WidescreenFixSpriteNameGuard()
{
    __asm
    {
        cmp ecx, 0x10000
        jb skipSprite

        mov ecx, dword ptr [ecx]
        cmp ecx, 0x10000
        jb skipSprite

        add ecx, 0x10
        push dword ptr [g_widescreenFixSpriteNameGuardContinue]
        retn

    skipSprite:
        push dword ptr [g_widescreenFixSpriteNameGuardSkip]
        retn
    }
}
#endif

bool IsSupportedProperShadersTextHash(uint32_t hash)
{
    return hash == kSupportedProperShadersTextHash;
}

bool IsProperShadersAddTxdSlotThunk(HMODULE psAsi, uintptr_t target)
{
    if (!psAsi || !target || !IsReadableCommitted(target, 11)) {
        return false;
    }

    char targetModule[MAX_PATH]{};
    if (ModuleBaseFromAddress(target, targetModule, sizeof(targetModule)) != reinterpret_cast<uintptr_t>(psAsi)) {
        return false;
    }

    uint8_t bytes[11]{};
    __try {
        std::memcpy(bytes, reinterpret_cast<const void*>(target), sizeof(bytes));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    if (bytes[0] != 0xA3 || bytes[5] != 0xE8 || bytes[10] != 0xC3) {
        return false;
    }

    uint32_t txdSlotStorage = 0;
    std::memcpy(&txdSlotStorage, bytes + 1, sizeof(txdSlotStorage));
    const uintptr_t callback = DecodeRel32JumpTarget(target + 5);
    char callbackModule[MAX_PATH]{};
    return IsWritableCommitted(txdSlotStorage, sizeof(uint32_t)) &&
        ModuleBaseFromAddress(callback, callbackModule, sizeof(callbackModule)) == reinterpret_cast<uintptr_t>(psAsi) &&
        IsExecutableCommitted(callback);
}

uintptr_t FindFlaAddTxdSlotThunk()
{
    HMODULE fla = FindFlaModule();
    if (!fla) {
        return 0;
    }

    static const uint8_t pattern[] = {
        0x03, 0xC2, 0x50, 0x50, 0xE8, 0, 0, 0, 0,
        0x83, 0xC4, 0x04, 0x58, 0x5E, 0xC3
    };
    const uintptr_t thunk = FindPatternInModuleText(fla, pattern, "xxxxx????xxxxxx", sizeof(pattern));
    if (!thunk) {
        return 0;
    }

    const uintptr_t callback = DecodeRel32JumpTarget(thunk + 4);
    char callbackModule[MAX_PATH]{};
    if (ModuleBaseFromAddress(callback, callbackModule, sizeof(callbackModule)) != reinterpret_cast<uintptr_t>(fla) ||
        !IsExecutableCommitted(callback)) {
        return 0;
    }
    return thunk;
}

void InstallProperShadersVtableGuard()
{
#if defined(_M_IX86)
    HMODULE psAsi = GetModuleHandleA("ProperShaders.asi");
    if (!psAsi) {
        psAsi = GetModuleHandleA("propershaders.asi");
    }
    if (!psAsi) {
        return;
    }

    const uintptr_t psBase = reinterpret_cast<uintptr_t>(psAsi);
    const uint32_t psTextHash = CalculateModuleFileTextHash(psAsi);
    uint8_t hookBytes[8]{};
    if (!IsReadableCommitted(0x731CC8, sizeof(hookBytes))) {
        Log("proper shaders AddTxdSlot adapter: GTA hook bytes unreadable");
        return;
    }
    __try {
        std::memcpy(hookBytes, reinterpret_cast<const void*>(0x731CC8), sizeof(hookBytes));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("proper shaders AddTxdSlot adapter: GTA hook byte read failed");
        return;
    }

    const uintptr_t adapter = reinterpret_cast<uintptr_t>(Bridge_ProperShadersAddTxdSlot_Combined);
    const uintptr_t currentTarget = hookBytes[0] == 0xE9 ? DecodeRel32JumpTarget(0x731CC8) : 0;
    const bool overlapPresent = currentTarget != adapter && hookBytes[0] == 0xE9 && hookBytes[3] == 0xE9;
    const uintptr_t flaThunk = FindFlaAddTxdSlotThunk();
    uintptr_t psTarget = 0;
    const char* psTargetSource = "none";

    if (overlapPresent) {
        int32_t psOffset = 0;
        std::memcpy(&psOffset, hookBytes + 4, sizeof(psOffset));
        const uintptr_t overlapTarget = 0x731CCB + 5 + static_cast<intptr_t>(psOffset);
        if (IsProperShadersAddTxdSlotThunk(psAsi, overlapTarget)) {
            psTarget = overlapTarget;
            psTargetSource = "overlap";
        }
    }

    // If FLA overwrote a previously installed PS tail hook, recover the verified
    // PS thunk from the known build instead of waiting for another overlap write.
    if (!psTarget && IsSupportedProperShadersTextHash(psTextHash)) {
        const uintptr_t verifiedTarget = psBase + 0x51630;
        if (IsProperShadersAddTxdSlotThunk(psAsi, verifiedTarget)) {
            psTarget = verifiedTarget;
            psTargetSource = "verified-rva";
        }
    }

    const bool flaOwnsEntry = hookBytes[0] == 0xE9 && flaThunk &&
        (currentTarget == flaThunk || overlapPresent);
    if (currentTarget != adapter && flaOwnsEntry && psTarget) {
        g_flaAddTxdSlotThunk = flaThunk;
        g_properShadersAddTxdSlotThunk = psTarget;
        if (WriteRel32Jump(0x731CC8, adapter)) {
            Log("proper shaders AddTxdSlot adapter: installed GTA=0x731CC8 bridge=0x%08X FLA=0x%08X PS=0x%08X source=%s fileTextHash=0x%08X",
                adapter, flaThunk, psTarget, psTargetSource, psTextHash);
        }
    } else if (currentTarget == adapter) {
        static LONG alreadyLogs = 0;
        if (InterlockedIncrement(&alreadyLogs) <= 2) {
            Log("proper shaders AddTxdSlot adapter: already installed FLA=0x%08X PS=0x%08X",
                g_flaAddTxdSlotThunk, g_properShadersAddTxdSlotThunk);
        }
    } else if (overlapPresent) {
        Log("proper shaders AddTxdSlot adapter: overlap rejected flaThunk=0x%08X psTarget=0x%08X psBase=0x%08X fileTextHash=0x%08X",
            flaThunk, psTarget, psBase, psTextHash);
    }

    // This is a fixed-RVA patch and is allowed only for the verified local build.
    if (!IsSupportedProperShadersTextHash(psTextHash)) {
        static LONG hashMismatchLogs = 0;
        if (InterlockedIncrement(&hashMismatchLogs) <= 2) {
            Log("proper shaders fixed patch: unsupported file text hash=0x%08X expected=0x%08X; skipped",
                psTextHash, kSupportedProperShadersTextHash);
        }
        return;
    }

    const uintptr_t patchAddr = psBase + 0x50C1F;
    uint8_t currentBytes[3]{};
    if (!IsReadableCommitted(patchAddr, sizeof(currentBytes))) {
        return;
    }
    __try {
        std::memcpy(currentBytes, reinterpret_cast<const void*>(patchAddr), sizeof(currentBytes));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return;
    }

    static const uint8_t expected[] = { 0x0F, 0x44, 0xC2 };
    static const uint8_t replacement[] = { 0x0F, 0x1F, 0x00 };
    if (std::memcmp(currentBytes, expected, sizeof(expected)) == 0) {
        if (WriteBytesWithProtect(patchAddr, replacement, sizeof(replacement))) {
            Log("proper shaders fixed patch: applied PS+0x50C1F textHash=0x%08X", psTextHash);
        }
    } else if (std::memcmp(currentBytes, replacement, sizeof(replacement)) != 0) {
        Log("proper shaders fixed patch: signature mismatch at PS+0x50C1F got=%02X %02X %02X textHash=0x%08X",
            currentBytes[0], currentBytes[1], currentBytes[2], psTextHash);
    }
#endif
}

void ApplyProperShadersCompat()
{
#if defined(_M_IX86)
    const uint32_t runtimeStreamingValue = static_cast<uint32_t>(g_relocatedStreamingInfo);
    auto isUsableStreamingTable = [](uint32_t value) -> bool {
        return value != 0 &&
            value != kOriginalStreamingInfo &&
            value >= 0x00400000 &&
            IsReadableCommitted(value, sizeof(uintptr_t));
    };

    HMODULE psAsi = GetModuleHandleA("ProperShaders.asi");
    if (!psAsi) {
        psAsi = GetModuleHandleA("propershaders.asi");
    }
    if (!psAsi || !isUsableStreamingTable(runtimeStreamingValue)) {
        Log("proper shaders compat: PS CStreaming patch skipped module=0x%p runtimeStreaming=0x%08X",
            psAsi, runtimeStreamingValue);
        return;
    }

    uintptr_t psBase = reinterpret_cast<uintptr_t>(psAsi);
    const uint32_t psTextHash = CalculateModuleFileTextHash(psAsi);
    if (!IsSupportedProperShadersTextHash(psTextHash)) {
        Log("proper shaders compat: CStreaming rewrite skipped unsupported textHash=0x%08X", psTextHash);
        return;
    }

    IMAGE_DOS_HEADER* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(psBase);
    if (!IsReadableCommitted(psBase, sizeof(IMAGE_DOS_HEADER)) || dos->e_magic != IMAGE_DOS_SIGNATURE) {
        Log("proper shaders compat: PS CStreaming patch skipped invalid DOS header base=0x%08X", psBase);
        return;
    }

    uintptr_t ntAddress = psBase + static_cast<uintptr_t>(dos->e_lfanew);
    IMAGE_NT_HEADERS* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(ntAddress);
    if (!IsReadableCommitted(ntAddress, sizeof(IMAGE_NT_HEADERS)) || nt->Signature != IMAGE_NT_SIGNATURE) {
        Log("proper shaders compat: PS CStreaming patch skipped invalid NT header base=0x%08X nt=0x%08X",
            psBase, ntAddress);
        return;
    }

    IMAGE_SECTION_HEADER* sections = IMAGE_FIRST_SECTION(nt);
    const size_t sectionBytes = static_cast<size_t>(nt->FileHeader.NumberOfSections) * sizeof(IMAGE_SECTION_HEADER);
    if (!IsReadableCommitted(reinterpret_cast<uintptr_t>(sections), sectionBytes)) {
        Log("proper shaders compat: PS CStreaming patch skipped unreadable section table base=0x%08X", psBase);
        return;
    }

    uintptr_t textStart = 0;
    size_t textSize = 0;
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (std::memcmp(sections[i].Name, ".text\0\0\0", 8) == 0) {
            textStart = psBase + sections[i].VirtualAddress;
            textSize = sections[i].Misc.VirtualSize;
            break;
        }
    }
    if (!textStart || textSize < 5 || !IsReadableCommitted(textStart, textSize)) {
        Log("proper shaders compat: PS CStreaming patch skipped invalid .text base=0x%08X start=0x%08X size=0x%X",
            psBase, textStart, static_cast<unsigned>(textSize));
        return;
    }

    uint8_t* textCopy = new uint8_t[textSize];
    bool copied = false;
    __try {
        std::memcpy(textCopy, reinterpret_cast<const void*>(textStart), textSize);
        copied = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        copied = false;
    }
    if (!copied) {
        delete[] textCopy;
        Log("proper shaders compat: PS CStreaming patch skipped .text copy fault start=0x%08X size=0x%X",
            textStart, static_cast<unsigned>(textSize));
        return;
    }

    uint32_t streamingPatched = 0;
    uint32_t streamingAlready = 0;
    uint32_t streamingCandidates = 0;
    for (size_t offset = 1; offset + sizeof(uint32_t) <= textSize; ++offset) {
        uint32_t val = 0;
        std::memcpy(&val, textCopy + offset, sizeof(val));
        if (val != kOriginalStreamingInfo && val != runtimeStreamingValue) {
            continue;
        }

        if (textCopy[offset - 1] != 0xB9) {
            continue;
        }

        ++streamingCandidates;
        if (val == runtimeStreamingValue) {
            ++streamingAlready;
            continue;
        }

        const uintptr_t patchAddress = textStart + offset;
        if (WriteBytesWithProtect(patchAddress, reinterpret_cast<const uint8_t*>(&runtimeStreamingValue), sizeof(runtimeStreamingValue))) {
            ++streamingPatched;
            std::memcpy(textCopy + offset, &runtimeStreamingValue, sizeof(runtimeStreamingValue));
        }
    }
    delete[] textCopy;

    Log("proper shaders compat: PS CStreaming .text scan module=0x%08X imageSize=0x%X textSize=0x%X textHash=0x%08X patched=%u already=%u candidates=%u (0x%08X -> 0x%08X)",
        psBase,
        nt->OptionalHeader.SizeOfImage,
        static_cast<unsigned>(textSize),
        psTextHash,
        streamingPatched,
        streamingAlready,
        streamingCandidates,
        kOriginalStreamingInfo,
        runtimeStreamingValue);
#endif
}

void InstallRwTexDictionaryFindNamedTextureGuard()
{
#if defined(_M_IX86)
    auto installPatch = [](uintptr_t patchAddress, const uint8_t* expectedBytes, size_t patchSize, uintptr_t bridgeTarget, const char* label) -> bool {
        const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
        if (currentTarget == bridgeTarget) {
            Log("rw texdict guard: %s already installed at 0x%08X", label, patchAddress);
            return true;
        }

        if (!IsReadableCommitted(patchAddress, patchSize)) {
            Log("rw texdict guard: %s patch address unreadable 0x%08X size=0x%X",
                label, patchAddress, static_cast<unsigned>(patchSize));
            return false;
        }

        uint8_t currentBytes[16]{};
        if (patchSize > sizeof(currentBytes)) {
            Log("rw texdict guard: %s patch size too large size=0x%X", label, static_cast<unsigned>(patchSize));
            return false;
        }

        __try {
            std::memcpy(currentBytes, reinterpret_cast<const void*>(patchAddress), patchSize);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Log("rw texdict guard: %s read exception at 0x%08X", label, patchAddress);
            return false;
        }

        if (std::memcmp(currentBytes, expectedBytes, patchSize) != 0) {
            Log("rw texdict guard: %s bytes mismatch at 0x%08X got %02X %02X %02X %02X %02X %02X",
                label, patchAddress,
                currentBytes[0], currentBytes[1], currentBytes[2],
                currentBytes[3], currentBytes[4], currentBytes[5]);
            return false;
        }

        const int64_t rel64 = static_cast<int64_t>(bridgeTarget) - static_cast<int64_t>(patchAddress + 5);
        if (rel64 < INT32_MIN || rel64 > INT32_MAX) {
            Log("rw texdict guard: %s bridge out of rel32 range source=0x%08X target=0x%08X diff=%lld",
                label, patchAddress, bridgeTarget, rel64);
            return false;
        }

        uint8_t patch[16]{};
        std::memset(patch, 0x90, patchSize);
        patch[0] = 0xE9;
        const int32_t rel32 = static_cast<int32_t>(rel64);
        std::memcpy(patch + 1, &rel32, sizeof(rel32));

        if (!WriteBytesWithProtect(patchAddress, patch, patchSize)) {
            Log("rw texdict guard: %s patch write failed at 0x%08X", label, patchAddress);
            return false;
        }

        Log("rw texdict guard: installed %s at 0x%08X -> 0x%08X size=%u",
            label, patchAddress, bridgeTarget, static_cast<unsigned>(patchSize));
        return true;
    };

    const uint8_t dictExpected[] = { 0x83, 0xC0, 0x08, 0x55, 0x56, 0x57, 0x8B, 0x18, 0x89, 0x44, 0x24, 0x14 };
    const uint8_t nameExpected[] = { 0x8B, 0xF1, 0x8B, 0xFD, 0x8A, 0x0E };
    const uint8_t linkExpected[] = { 0x8B, 0x1B, 0x8B, 0x44, 0x24, 0x14 };

    installPatch(0x007F39F5, dictExpected, sizeof(dictExpected),
        reinterpret_cast<uintptr_t>(Bridge_RwTexDictionaryFindNamedTexture_DictGuard), "dictionary pointer");
    installPatch(0x007F3A13, nameExpected, sizeof(nameExpected),
        reinterpret_cast<uintptr_t>(Bridge_RwTexDictionaryFindNamedTexture_NameGuard), "name pointer");
    installPatch(0x007F3A52, linkExpected, sizeof(linkExpected),
        reinterpret_cast<uintptr_t>(Bridge_RwTexDictionaryFindNamedTexture_LinkGuard), "list link");
#else
    Log("rw texdict guard: unsupported architecture");
#endif
}

void InstallTxdLoadDictionaryWriteGuard()
{
#if defined(_M_IX86)
    constexpr uintptr_t patchAddress = 0x00731E13;
    static const uint8_t expectedBytes[] = {
        0x89, 0x06,             // mov [esi], eax
        0x74, 0x09,             // jz 0x731E20
        0x53                    // push ebx
    };

    const uintptr_t guardTarget = reinterpret_cast<uintptr_t>(Bridge_TxdLoadDictionaryWriteGuard);
    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    if (currentTarget == guardTarget) {
        Log("txd load guard: already installed at 0x%08X", patchAddress);
        return;
    }

    uint8_t current[sizeof(expectedBytes)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("txd load guard: patch address unreadable 0x%08X", patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("txd load guard: read exception at 0x%08X", patchAddress);
        return;
    }

    if (std::memcmp(current, expectedBytes, sizeof(expectedBytes)) != 0) {
        Log("txd load guard: unexpected bytes at 0x%08X got %02X %02X %02X %02X %02X currentTarget=0x%08X",
            patchAddress,
            current[0],
            current[1],
            current[2],
            current[3],
            current[4],
            currentTarget);
        return;
    }

    if (WriteRel32Jump(patchAddress, guardTarget)) {
        Log("txd load guard: installed at 0x%08X bridge=0x%08X",
            patchAddress,
            guardTarget);
    }
#else
    Log("txd load guard: unsupported architecture");
#endif
}

void InstallWidescreenFixSpriteNameGuard()
{
#if defined(_M_IX86)
    HMODULE module = GetModuleHandleA("GTASA.WidescreenFix.asi");
    if (!module) {
        module = GetModuleHandleA("gtasa.widescreenfix.asi");
    }
    if (!module) {
        Log("widescreenfix sprite guard: module not loaded, skipping");
        return;
    }

    const uintptr_t base = reinterpret_cast<uintptr_t>(module);
    const uintptr_t patchAddress = base + 0x1DB3E;
    const uintptr_t bridgeTarget = reinterpret_cast<uintptr_t>(Bridge_WidescreenFixSpriteNameGuard);
    g_widescreenFixSpriteNameGuardContinue = base + 0x1DB43;
    g_widescreenFixSpriteNameGuardSkip = base + 0x1DD91;

    const uintptr_t currentTarget = DecodeRel32JumpTarget(patchAddress);
    if (currentTarget == bridgeTarget) {
        Log("widescreenfix sprite guard: already installed base=0x%08X patch=0x%08X", base, patchAddress);
        return;
    }

    const uint8_t expected[] = { 0x8B, 0x09, 0x83, 0xC1, 0x10 };
    uint8_t current[sizeof(expected)]{};
    if (!IsReadableCommitted(patchAddress, sizeof(current))) {
        Log("widescreenfix sprite guard: patch address unreadable base=0x%08X patch=0x%08X", base, patchAddress);
        return;
    }

    __try {
        std::memcpy(current, reinterpret_cast<const void*>(patchAddress), sizeof(current));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("widescreenfix sprite guard: read exception base=0x%08X patch=0x%08X", base, patchAddress);
        return;
    }

    if (std::memcmp(current, expected, sizeof(expected)) != 0) {
        Log("widescreenfix sprite guard: bytes mismatch base=0x%08X patch=0x%08X got %02X %02X %02X %02X %02X",
            base, patchAddress, current[0], current[1], current[2], current[3], current[4]);
        return;
    }

    if (WriteRel32Jump(patchAddress, bridgeTarget)) {
        Log("widescreenfix sprite guard: installed base=0x%08X patch=0x%08X bridge=0x%08X continue=0x%08X skip=0x%08X",
            base, patchAddress, bridgeTarget, g_widescreenFixSpriteNameGuardContinue, g_widescreenFixSpriteNameGuardSkip);
    }
#else
    Log("widescreenfix sprite guard: unsupported architecture");
#endif
}

bool IsProperShadersAddTxdSlotConflictPresent()
{
#if defined(_M_IX86)
    uint8_t hookBytes[8]{};
    if (!IsReadableCommitted(0x731CC8, sizeof(hookBytes))) {
        return false;
    }

    __try {
        std::memcpy(hookBytes, reinterpret_cast<const void*>(0x731CC8), sizeof(hookBytes));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    if (hookBytes[0] != 0xE9) {
        return false;
    }

    const int32_t currentOffset = *reinterpret_cast<const int32_t*>(&hookBytes[1]);
    const uintptr_t currentTarget = 0x731CC8 + 5 + currentOffset;
    if (currentTarget == reinterpret_cast<uintptr_t>(Bridge_ProperShadersAddTxdSlot_Combined)) {
        return false;
    }
    return hookBytes[3] == 0xE9;
#else
    return false;
#endif
}

void InstallEarlyProperShadersCompatOnce(bool includeTextureGuards)
{
    if (!g_config.enableProperShadersCompat) {
        return;
    }

    InstallProperShadersVtableGuard();
    if (includeTextureGuards) {
        InstallRwTexDictionaryFindNamedTextureGuard();
        InstallTxdLoadDictionaryWriteGuard();
    }
}

DWORD WINAPI EarlyProperShadersCompatThread(void*)
{
    bool installedTextureGuards = false;
    bool touchedProperShadersOnce = false;
    bool appliedRuntimeCompat = false;

    for (int attempt = 0; attempt < 400; ++attempt) {
        const bool psLoaded =
            GetModuleHandleA("ProperShaders.asi") != nullptr ||
            GetModuleHandleA("propershaders.asi") != nullptr;
        const bool conflict = IsProperShadersAddTxdSlotConflictPresent();
        bool adapterInstalled = false;
        if (IsReadableCommitted(0x731CC8, 5)) {
            uint8_t opcode = 0;
            __try {
                opcode = *reinterpret_cast<const uint8_t*>(0x731CC8);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                opcode = 0;
            }
            adapterInstalled = opcode == 0xE9 &&
                DecodeRel32JumpTarget(0x731CC8) == reinterpret_cast<uintptr_t>(Bridge_ProperShadersAddTxdSlot_Combined);
        }
        const bool periodicRetry = psLoaded && !adapterInstalled && (attempt % 40) == 0;

        if (psLoaded && (!touchedProperShadersOnce || conflict || periodicRetry)) {
            InstallEarlyProperShadersCompatOnce(!installedTextureGuards);
            touchedProperShadersOnce = true;
            installedTextureGuards = true;
        }
        if (psLoaded && !appliedRuntimeCompat && g_relocatedStreamingInfo) {
            ApplyProperShadersCompat();
            appliedRuntimeCompat = true;
        }

        if (attempt == 399 && conflict) {
            Log("early proper shaders compat: conflict still present after polling");
        }

        Sleep(25);
    }

    return 0;
}


