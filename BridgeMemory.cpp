#include "FLACompatBridgeInternal.h"

#if defined(_M_IX86)
extern "C" int __stdcall Bridge_IsReadableMemory(uintptr_t address, size_t size)
{
    if (!address || !size) {
        return 0;
    }

    const uintptr_t last = address + size - 1;
    if (last < address) {
        return 0;
    }

    __try {
        volatile uint8_t sink = *reinterpret_cast<volatile const uint8_t*>(address);
        if (last != address) {
            sink ^= *reinterpret_cast<volatile const uint8_t*>(last);
        }

        uintptr_t page = (address & ~static_cast<uintptr_t>(0xFFF)) + 0x1000;
        while (page > address && page < last) {
            sink ^= *reinterpret_cast<volatile const uint8_t*>(page);
            const uintptr_t nextPage = page + 0x1000;
            if (nextPage <= page) {
                break;
            }
            page = nextPage;
        }
        (void)sink;
        return 1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}
#endif

const char* BaseName(const char* path)
{
    const char* slash = std::strrchr(path, '\\');
    const char* slash2 = std::strrchr(path, '/');
    if (slash2 && (!slash || slash2 > slash)) {
        slash = slash2;
    }
    return slash ? slash + 1 : path;
}

bool FileContainsPattern(const char* path, const uint8_t* pattern, size_t patternSize)
{
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > 64ll * 1024ll * 1024ll) {
        CloseHandle(file);
        return false;
    }

    auto* bytes = new uint8_t[static_cast<size_t>(size.QuadPart)];
    DWORD read = 0;
    const bool ok = ReadFile(file, bytes, static_cast<DWORD>(size.QuadPart), &read, nullptr) && read == size.QuadPart;
    CloseHandle(file);

    bool found = false;
    if (ok && read >= patternSize) {
        for (DWORD i = 0; i <= read - patternSize; ++i) {
            if (std::memcmp(bytes + i, pattern, patternSize) == 0) {
                found = true;
                break;
            }
        }
    }

    delete[] bytes;
    return found;
}

DWORD FindProcessMainThreadId()
{
    const DWORD processId = GetCurrentProcessId();
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    DWORD result = 0;
    ULARGE_INTEGER earliest{};
    earliest.QuadPart = ~0ull;
    THREADENTRY32 entry{};
    entry.dwSize = sizeof(entry);
    if (Thread32First(snapshot, &entry)) {
        do {
            if (entry.th32OwnerProcessID != processId) {
                continue;
            }

            HANDLE thread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ThreadID);
            if (!thread) {
                continue;
            }

            FILETIME created{}, exited{}, kernel{}, user{};
            if (GetThreadTimes(thread, &created, &exited, &kernel, &user)) {
                ULARGE_INTEGER createdValue{};
                createdValue.LowPart = created.dwLowDateTime;
                createdValue.HighPart = created.dwHighDateTime;
                if (createdValue.QuadPart < earliest.QuadPart) {
                    earliest = createdValue;
                    result = entry.th32ThreadID;
                }
            }
            CloseHandle(thread);
        } while (Thread32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return result;
}

uintptr_t ModuleBaseFromAddress(uintptr_t address, char* moduleName, size_t moduleNameSize)
{
    MODULEENTRY32 me{};
    me.dwSize = sizeof(me);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE) {
        strncpy_s(moduleName, moduleNameSize, "<snapshot-failed>", _TRUNCATE);
        return 0;
    }

    uintptr_t base = 0;
    if (Module32First(snapshot, &me)) {
        do {
            const uintptr_t modBase = reinterpret_cast<uintptr_t>(me.modBaseAddr);
            const uintptr_t modEnd = modBase + me.modBaseSize;
            if (address >= modBase && address < modEnd) {
                base = modBase;
                strncpy_s(moduleName, moduleNameSize, me.szModule, _TRUNCATE);
                break;
            }
        } while (Module32Next(snapshot, &me));
    }
    CloseHandle(snapshot);

    if (!base) {
        strncpy_s(moduleName, moduleNameSize, "<unknown-module>", _TRUNCATE);
    }
    return base;
}

const char* ProtectName(DWORD protect)
{
    protect &= 0xFF;
    switch (protect) {
    case PAGE_NOACCESS: return "NOACCESS";
    case PAGE_READONLY: return "READONLY";
    case PAGE_READWRITE: return "READWRITE";
    case PAGE_WRITECOPY: return "WRITECOPY";
    case PAGE_EXECUTE: return "EXECUTE";
    case PAGE_EXECUTE_READ: return "EXECUTE_READ";
    case PAGE_EXECUTE_READWRITE: return "EXECUTE_READWRITE";
    case PAGE_EXECUTE_WRITECOPY: return "EXECUTE_WRITECOPY";
    default: return "UNKNOWN";
    }
}

const char* StateName(DWORD state)
{
    switch (state) {
    case MEM_COMMIT: return "COMMIT";
    case MEM_RESERVE: return "RESERVE";
    case MEM_FREE: return "FREE";
    default: return "UNKNOWN";
    }
}

const char* TypeName(DWORD type)
{
    switch (type) {
    case MEM_IMAGE: return "IMAGE";
    case MEM_MAPPED: return "MAPPED";
    case MEM_PRIVATE: return "PRIVATE";
    default: return "UNKNOWN";
    }
}

bool SafeReadU32(uintptr_t address, uint32_t* out)
{
    if (!out || !address) {
        return false;
    }

    __try {
        *out = *reinterpret_cast<const uint32_t*>(address);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool SafeReadU8(uintptr_t address, uint8_t* out)
{
    if (!out || !address) {
        return false;
    }

    __try {
        *out = *reinterpret_cast<const uint8_t*>(address);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool SafeReadF32(uintptr_t address, float* out)
{
    if (!out || !address) {
        return false;
    }

    __try {
        *out = *reinterpret_cast<const float*>(address);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool IsReadableCommitted(uintptr_t address, size_t size)
{
    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(reinterpret_cast<const void*>(address), &mbi, sizeof(mbi))) {
        return false;
    }

    const uintptr_t base = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
    const uintptr_t end = base + static_cast<uintptr_t>(mbi.RegionSize);
    if (address < base || address + size > end) {
        return false;
    }

    return mbi.State == MEM_COMMIT && !(mbi.Protect & PAGE_NOACCESS) && !(mbi.Protect & PAGE_GUARD);
}

bool IsWritableCommitted(uintptr_t address, size_t size)
{
    if (!address || !size) {
        return false;
    }

    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(reinterpret_cast<const void*>(address), &mbi, sizeof(mbi))) {
        return false;
    }

    const uintptr_t base = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
    const uintptr_t end = base + static_cast<uintptr_t>(mbi.RegionSize);
    if (address < base || address + size > end) {
        return false;
    }

    if (mbi.State != MEM_COMMIT || (mbi.Protect & PAGE_NOACCESS) || (mbi.Protect & PAGE_GUARD)) {
        return false;
    }

    const DWORD protect = mbi.Protect & 0xFF;
    return protect == PAGE_READWRITE || protect == PAGE_WRITECOPY ||
        protect == PAGE_EXECUTE_READWRITE || protect == PAGE_EXECUTE_WRITECOPY;
}

bool BytePatternMatches(uintptr_t address, const uint8_t* bytes, size_t size)
{
    if (!bytes || !size || !IsReadableCommitted(address, size)) {
        return false;
    }

    __try {
        return std::memcmp(reinterpret_cast<const void*>(address), bytes, size) == 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

extern "C" bool __stdcall Bridge_IsWritableMemory(uintptr_t address, size_t size)
{
    return IsWritableCommitted(address, size);
}

extern "C" bool __stdcall Bridge_IsExecutableMemory(uintptr_t address)
{
    return IsExecutableCommitted(address);
}

bool CopyMemoryWithProtect(uintptr_t destination, uintptr_t source, size_t size)
{
    if (!IsReadableCommitted(source, size)) {
        Log("legacy shadow: source unreadable address=0x%08X size=0x%X", source, static_cast<unsigned>(size));
        return false;
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(reinterpret_cast<void*>(destination), size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Log("legacy shadow: destination protect failed address=0x%08X size=0x%X gle=%lu",
            destination, static_cast<unsigned>(size), GetLastError());
        return false;
    }

    std::memcpy(reinterpret_cast<void*>(destination), reinterpret_cast<const void*>(source), size);

    DWORD ignored = 0;
    VirtualProtect(reinterpret_cast<void*>(destination), size, oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<const void*>(destination), size);
    return true;
}

bool WriteBytesWithProtect(uintptr_t destination, const uint8_t* bytes, size_t size)
{
    DWORD oldProtect = 0;
    if (!VirtualProtect(reinterpret_cast<void*>(destination), size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Log("patch: protect failed address=0x%08X size=0x%X gle=%lu",
            destination, static_cast<unsigned>(size), GetLastError());
        return false;
    }

    std::memcpy(reinterpret_cast<void*>(destination), bytes, size);

    DWORD ignored = 0;
    VirtualProtect(reinterpret_cast<void*>(destination), size, oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<const void*>(destination), size);
    return true;
}

uintptr_t DecodeRel32JumpTarget(uintptr_t address)
{
    uint8_t bytes[5]{};
    if (!IsReadableCommitted(address, sizeof(bytes))) {
        return 0;
    }

    __try {
        std::memcpy(bytes, reinterpret_cast<const void*>(address), sizeof(bytes));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }

    if (bytes[0] != 0xE8 && bytes[0] != 0xE9) {
        return 0;
    }

    int32_t rel = 0;
    std::memcpy(&rel, bytes + 1, sizeof(rel));
    return address + 5 + rel;
}

bool IsExecutableCommitted(uintptr_t address)
{
    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(reinterpret_cast<const void*>(address), &mbi, sizeof(mbi))) {
        return false;
    }
    if (mbi.State != MEM_COMMIT || (mbi.Protect & PAGE_NOACCESS) || (mbi.Protect & PAGE_GUARD)) {
        return false;
    }

    const DWORD protect = mbi.Protect & 0xFF;
    return protect == PAGE_EXECUTE || protect == PAGE_EXECUTE_READ ||
        protect == PAGE_EXECUTE_READWRITE || protect == PAGE_EXECUTE_WRITECOPY;
}

bool WriteRel32Jump(uintptr_t source, uintptr_t target)
{
    const int64_t diff = static_cast<int64_t>(target) - static_cast<int64_t>(source + 5);
    if (diff < INT32_MIN || diff > INT32_MAX) {
        Log("bridge stub: rel32 jump out of range source=0x%08X target=0x%08X diff=%lld",
            source, target, diff);
        return false;
    }

    uint8_t bytes[5]{ 0xE9, 0, 0, 0, 0 };
    const int32_t rel = static_cast<int32_t>(diff);
    std::memcpy(bytes + 1, &rel, sizeof(rel));
    return WriteBytesWithProtect(source, bytes, sizeof(bytes));
}

uintptr_t CreateRel32Trampoline(uintptr_t source, size_t stolenBytes)
{
    if (stolenBytes < 5 || !IsReadableCommitted(source, stolenBytes)) {
        return 0;
    }

    uint8_t* gateway = reinterpret_cast<uint8_t*>(VirtualAlloc(nullptr, stolenBytes + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!gateway) {
        Log("trampoline: VirtualAlloc failed source=0x%08X stolen=%u gle=%lu", source, static_cast<uint32_t>(stolenBytes), GetLastError());
        return 0;
    }

    __try {
        std::memcpy(gateway, reinterpret_cast<const void*>(source), stolenBytes);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        VirtualFree(gateway, 0, MEM_RELEASE);
        Log("trampoline: read exception source=0x%08X stolen=%u", source, static_cast<uint32_t>(stolenBytes));
        return 0;
    }

    const uintptr_t jumpBackSource = reinterpret_cast<uintptr_t>(gateway) + stolenBytes;
    const uintptr_t jumpBackTarget = source + stolenBytes;
    const int64_t diff = static_cast<int64_t>(jumpBackTarget) - static_cast<int64_t>(jumpBackSource + 5);
    if (diff < INT32_MIN || diff > INT32_MAX) {
        VirtualFree(gateway, 0, MEM_RELEASE);
        Log("trampoline: jump back out of range source=0x%08X target=0x%08X diff=%lld", jumpBackSource, jumpBackTarget, diff);
        return 0;
    }

    gateway[stolenBytes] = 0xE9;
    const int32_t rel = static_cast<int32_t>(diff);
    std::memcpy(gateway + stolenBytes + 1, &rel, sizeof(rel));
    return reinterpret_cast<uintptr_t>(gateway);
}

bool ReadU32(uintptr_t address, uint32_t* out)
{
    if (!out || !address) {
        return false;
    }

    __try {
        std::memcpy(out, reinterpret_cast<const void*>(address), sizeof(*out));
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool FindModuleRangeFromAddress(uintptr_t address, char* moduleName, size_t moduleNameSize, uintptr_t* moduleBase, uintptr_t* moduleEnd)
{
    if (moduleName && moduleNameSize) {
        moduleName[0] = '\0';
    }
    if (moduleBase) {
        *moduleBase = 0;
    }
    if (moduleEnd) {
        *moduleEnd = 0;
    }

    MODULEENTRY32 me{};
    me.dwSize = sizeof(me);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE) {
        if (moduleName && moduleNameSize) {
            strncpy_s(moduleName, moduleNameSize, "<snapshot-failed>", _TRUNCATE);
        }
        return false;
    }

    bool found = false;
    if (Module32First(snapshot, &me)) {
        do {
            const uintptr_t base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
            const uintptr_t end = base + me.modBaseSize;
            if (address >= base && address < end) {
                if (moduleName && moduleNameSize) {
                    strncpy_s(moduleName, moduleNameSize, me.szModule, _TRUNCATE);
                }
                if (moduleBase) {
                    *moduleBase = base;
                }
                if (moduleEnd) {
                    *moduleEnd = end;
                }
                found = true;
                break;
            }
        } while (Module32Next(snapshot, &me));
    }

    CloseHandle(snapshot);
    if (!found && moduleName && moduleNameSize) {
        strncpy_s(moduleName, moduleNameSize, "<unknown-module>", _TRUNCATE);
    }
    return found;
}

bool ModuleMemoryContainsU32(uintptr_t moduleBase, uintptr_t moduleEnd, uint32_t needle)
{
    if (!needle || moduleEnd <= moduleBase) {
        return false;
    }

    uintptr_t cursor = moduleBase;
    while (cursor < moduleEnd) {
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

        const uintptr_t scanStart = regionBase < moduleBase ? moduleBase : regionBase;
        const uintptr_t scanEnd = regionEnd > moduleEnd ? moduleEnd : regionEnd;
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
            if (value == needle) {
                delete[] buffer;
                return true;
            }
        }

        delete[] buffer;
    }

    return false;
}

uintptr_t FindPatternInModuleText(HMODULE module, const uint8_t* pattern, const char* mask, size_t patternLen)
{
#if defined(_M_IX86)
    if (!module || !pattern || !mask || patternLen == 0) {
        return 0;
    }

    uintptr_t base = reinterpret_cast<uintptr_t>(module);
    MODULEINFO mi{};
    if (!GetModuleInformation(GetCurrentProcess(), module, &mi, sizeof(mi))) {
        return 0;
    }

    PIMAGE_DOS_HEADER dos = reinterpret_cast<PIMAGE_DOS_HEADER>(base);
    if (!IsReadableCommitted(base, sizeof(IMAGE_DOS_HEADER)) || dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return 0;
    }

    PIMAGE_NT_HEADERS nt = reinterpret_cast<PIMAGE_NT_HEADERS>(base + dos->e_lfanew);
    if (!IsReadableCommitted(reinterpret_cast<uintptr_t>(nt), sizeof(IMAGE_NT_HEADERS)) ||
        nt->Signature != IMAGE_NT_SIGNATURE) {
        return 0;
    }

    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt);
    if (!IsReadableCommitted(reinterpret_cast<uintptr_t>(section),
                             nt->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER))) {
        return 0;
    }

    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section) {
        if (memcmp(section->Name, ".text\0\0\0", 8) != 0) {
            continue;
        }

        uintptr_t textBase = base + section->VirtualAddress;
        size_t textSize = section->Misc.VirtualSize;
        if (!IsReadableCommitted(textBase, textSize) || textSize < patternLen) {
            return 0;
        }

        const uint8_t* scan = reinterpret_cast<const uint8_t*>(textBase);
        for (size_t j = 0; j + patternLen <= textSize; ++j) {
            bool match = true;
            for (size_t k = 0; k < patternLen; ++k) {
                if (mask[k] == 'x' && scan[j + k] != pattern[k]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return textBase + j;
            }
        }
        break;
    }
#endif
    return 0;
}

uint32_t CalculateModuleTextHash(HMODULE module)
{
    uint32_t hash = 0xFFFFFFFFu;
#if defined(_M_IX86)
    if (!module) {
        return 0;
    }

    uintptr_t base = reinterpret_cast<uintptr_t>(module);
    MODULEINFO mi{};
    if (!GetModuleInformation(GetCurrentProcess(), module, &mi, sizeof(mi))) {
        return 0;
    }

    PIMAGE_DOS_HEADER dos = reinterpret_cast<PIMAGE_DOS_HEADER>(base);
    if (!IsReadableCommitted(base, sizeof(IMAGE_DOS_HEADER)) || dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return 0;
    }

    PIMAGE_NT_HEADERS nt = reinterpret_cast<PIMAGE_NT_HEADERS>(base + dos->e_lfanew);
    if (!IsReadableCommitted(reinterpret_cast<uintptr_t>(nt), sizeof(IMAGE_NT_HEADERS)) ||
        nt->Signature != IMAGE_NT_SIGNATURE) {
        return 0;
    }

    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt);
    if (!IsReadableCommitted(reinterpret_cast<uintptr_t>(section),
                             nt->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER))) {
        return 0;
    }

    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section) {
        if (memcmp(section->Name, ".text\0\0\0", 8) != 0) {
            continue;
        }

        uintptr_t textBase = base + section->VirtualAddress;
        size_t textSize = section->Misc.VirtualSize;
        if (!IsReadableCommitted(textBase, textSize)) {
            return 0;
        }

        // Simple CRC32 (IEEE 802.3 polynomial)
        static uint32_t crcTable[256];
        static bool crcTableReady = false;
        if (!crcTableReady) {
            for (int n = 0; n < 256; ++n) {
                uint32_t c = static_cast<uint32_t>(n);
                for (int k = 0; k < 8; ++k) {
                    c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
                }
                crcTable[n] = c;
            }
            crcTableReady = true;
        }

        const uint8_t* data = reinterpret_cast<const uint8_t*>(textBase);
        for (size_t j = 0; j < textSize; ++j) {
            hash = crcTable[(hash ^ data[j]) & 0xFF] ^ (hash >> 8);
        }
        hash ^= 0xFFFFFFFFu;
        break;
    }
#endif
    return hash;
}

uint32_t CalculateModuleFileTextHash(HMODULE module)
{
    if (!module) {
        return 0;
    }

    char path[MAX_PATH]{};
    if (!GetModuleFileNameA(module, path, static_cast<DWORD>(sizeof(path)))) {
        return 0;
    }

    HANDLE file = CreateFileA(path, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 0;
    }

    LARGE_INTEGER fileSize{};
    if (!GetFileSizeEx(file, &fileSize) || fileSize.QuadPart <= 0 || fileSize.QuadPart > 64ll * 1024ll * 1024ll) {
        CloseHandle(file);
        return 0;
    }

    const size_t size = static_cast<size_t>(fileSize.QuadPart);
    uint8_t* bytes = new uint8_t[size];
    DWORD bytesRead = 0;
    const bool readOk = ReadFile(file, bytes, static_cast<DWORD>(size), &bytesRead, nullptr) && bytesRead == size;
    CloseHandle(file);
    if (!readOk || size < sizeof(IMAGE_DOS_HEADER)) {
        delete[] bytes;
        return 0;
    }

    const IMAGE_DOS_HEADER* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes);
    const size_t ntOffset = dos->e_lfanew > 0 ? static_cast<size_t>(dos->e_lfanew) : size;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || ntOffset > size || size - ntOffset < sizeof(IMAGE_NT_HEADERS)) {
        delete[] bytes;
        return 0;
    }

    const IMAGE_NT_HEADERS* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(bytes + ntOffset);
    const size_t sectionOffset = ntOffset + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + nt->FileHeader.SizeOfOptionalHeader;
    const size_t sectionBytes = static_cast<size_t>(nt->FileHeader.NumberOfSections) * sizeof(IMAGE_SECTION_HEADER);
    if (nt->Signature != IMAGE_NT_SIGNATURE || sectionOffset > size || sectionBytes > size - sectionOffset) {
        delete[] bytes;
        return 0;
    }

    uint32_t crcTable[256]{};
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t c = i;
        for (int bit = 0; bit < 8; ++bit) {
            c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        }
        crcTable[i] = c;
    }

    uint32_t result = 0;
    const IMAGE_SECTION_HEADER* sections = reinterpret_cast<const IMAGE_SECTION_HEADER*>(bytes + sectionOffset);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        const IMAGE_SECTION_HEADER& section = sections[i];
        if (std::memcmp(section.Name, ".text\0\0\0", 8) != 0) {
            continue;
        }

        const size_t virtualSize = section.Misc.VirtualSize;
        const size_t rawSize = section.SizeOfRawData;
        const size_t rawOffset = section.PointerToRawData;
        if (virtualSize == 0 || virtualSize > 64u * 1024u * 1024u ||
            rawOffset > size || rawSize > size - rawOffset) {
            break;
        }

        uint32_t hash = 0xFFFFFFFFu;
        const size_t fileBackedSize = virtualSize < rawSize ? virtualSize : rawSize;
        for (size_t j = 0; j < fileBackedSize; ++j) {
            hash = crcTable[(hash ^ bytes[rawOffset + j]) & 0xFF] ^ (hash >> 8);
        }
        for (size_t j = fileBackedSize; j < virtualSize; ++j) {
            hash = crcTable[hash & 0xFF] ^ (hash >> 8);
        }
        result = hash ^ 0xFFFFFFFFu;
        break;
    }

    delete[] bytes;
    return result;
}

HMODULE FindLoadedModuleBySubstring(const char* needle)
{
    if (!needle || !*needle) {
        return nullptr;
    }

    MODULEENTRY32 me{};
    me.dwSize = sizeof(me);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE) {
        return nullptr;
    }

    HMODULE result = nullptr;
    if (Module32First(snapshot, &me)) {
        do {
            if (ContainsCaseInsensitive(me.szModule, needle) || ContainsCaseInsensitive(me.szExePath, needle)) {
                result = me.hModule;
                break;
            }
        } while (Module32Next(snapshot, &me));
    }

    CloseHandle(snapshot);
    return result;
}

bool ContainsCaseInsensitive(const char* haystack, const char* needle)
{
    if (!haystack || !needle || !*needle) {
        return false;
    }

    const size_t needleLen = std::strlen(needle);
    for (const char* p = haystack; *p; ++p) {
        if (_strnicmp(p, needle, needleLen) == 0) {
            return true;
        }
    }
    return false;
}

bool MemoryMatches(uintptr_t address, const uint8_t* bytes, size_t size)
{
    if (!bytes || !IsReadableCommitted(address, size)) {
        return false;
    }
    __try {
        return std::memcmp(reinterpret_cast<const void*>(address), bytes, size) == 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool IsExecutableRegion(const MEMORY_BASIC_INFORMATION& mbi)
{
    if (mbi.State != MEM_COMMIT || (mbi.Protect & PAGE_NOACCESS) || (mbi.Protect & PAGE_GUARD)) {
        return false;
    }

    const DWORD protect = mbi.Protect & 0xFF;
    return protect == PAGE_EXECUTE || protect == PAGE_EXECUTE_READ ||
        protect == PAGE_EXECUTE_READWRITE || protect == PAGE_EXECUTE_WRITECOPY;
}

bool BytesMatchInKnownReadableRegion(uintptr_t address, const uint8_t* bytes, size_t size)
{
    if (!bytes || !address || !size) {
        return false;
    }

    __try {
        return std::memcmp(reinterpret_cast<const void*>(address), bytes, size) == 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool HasBytesInRange(uintptr_t address, size_t rangeSize, const uint8_t* bytes, size_t bytesSize)
{
    if (!bytes || !bytesSize || rangeSize < bytesSize) {
        return false;
    }

    for (size_t offset = 0; offset + bytesSize <= rangeSize; ++offset) {
        if (BytesMatchInKnownReadableRegion(address + offset, bytes, bytesSize)) {
            return true;
        }
    }
    return false;
}

bool IsExecutableProtect(DWORD protect)
{
    const DWORD p = protect & 0xFF;
    return p == PAGE_EXECUTE || p == PAGE_EXECUTE_READ ||
        p == PAGE_EXECUTE_READWRITE || p == PAGE_EXECUTE_WRITECOPY;
}

bool StackMentionsModule(uintptr_t esp, const char* needle)
{
    if (!needle || !*needle || !IsReadableCommitted(esp, 32 * sizeof(uintptr_t))) {
        return false;
    }

    for (int i = 0; i < 32; ++i) {
        uintptr_t value = 0;
        __try {
            value = reinterpret_cast<const uintptr_t*>(esp)[i];
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        char moduleName[MAX_PATH]{};
        if (ModuleBaseFromAddress(value, moduleName, sizeof(moduleName)) &&
            ContainsCaseInsensitive(moduleName, needle)) {
            return true;
        }
    }
    return false;
}

bool FindStackValue(uintptr_t esp, uintptr_t needle, int maxDwords, uintptr_t* valueAddress)
{
    if (!needle || maxDwords <= 0 || !IsReadableCommitted(esp, static_cast<size_t>(maxDwords) * sizeof(uintptr_t))) {
        return false;
    }

    for (int i = 0; i < maxDwords; ++i) {
        uintptr_t value = 0;
        __try {
            value = reinterpret_cast<const uintptr_t*>(esp)[i];
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        if (value == needle) {
            if (valueAddress) {
                *valueAddress = esp + static_cast<uintptr_t>(i) * sizeof(uintptr_t);
            }
            return true;
        }
    }

    return false;
}

bool IsFreeOrNonExecutableRegion(uintptr_t address, DWORD* stateOut, DWORD* protectOut) {
    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(reinterpret_cast<const void*>(address), &mbi, sizeof(mbi))) {
        if (stateOut) {
            *stateOut = 0;
        }
        if (protectOut) {
            *protectOut = 0;
        }
        return true;
    }

    if (stateOut) {
        *stateOut = mbi.State;
    }
    if (protectOut) {
        *protectOut = mbi.Protect;
    }

    if (mbi.State == MEM_FREE || mbi.State != MEM_COMMIT ||
        (mbi.Protect & PAGE_NOACCESS) || (mbi.Protect & PAGE_GUARD)) {
        return true;
    }

    return !IsExecutableCommitted(address);
}

bool StackMentionsGtaScriptOpcodePath(uintptr_t esp)
{
    static const uintptr_t kScriptPathHints[] = {
        0x00456878, // CRunningScript native opcode dispatch return path seen around 0213/0215 crashes
        0x00457332,
        0x0047E63B, // CRunningScript::ProcessCommands500To599, pickup-related command path
        0x0047DDA5, // CRunningScript::ProcessCommands400To499
        0x00469FF7, // CRunningScript::Process
        0x0046A220, // CTheScripts::Process
        0x0083D326
    };

    for (uintptr_t hint : kScriptPathHints) {
        if (FindStackValue(esp, hint, 48, nullptr)) {
            return true;
        }
    }
    return false;
}

