#include "FLACompatBridgeInternal.h"

void Log(const char* fmt, ...)
{
    FILE* file = nullptr;
    if (fopen_s(&file, g_logPath, "a") != 0 || !file) {
        return;
    }

    SYSTEMTIME st{};
    GetLocalTime(&st);
    std::fprintf(file, "[%04u-%02u-%02u %02u:%02u:%02u.%03u] ",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    va_list args;
    va_start(args, fmt);
    std::vfprintf(file, fmt, args);
    va_end(args);

    std::fputc('\n', file);
    std::fclose(file);
}

void LogMemoryRegion(const char* label, uintptr_t address)
{
    if (!g_config.enableMemoryRegionDiagnostics) {
        return;
    }

    MEMORY_BASIC_INFORMATION mbi{};
    const SIZE_T result = VirtualQuery(reinterpret_cast<const void*>(address), &mbi, sizeof(mbi));
    if (!result) {
        Log("memory[%s]: address=0x%08X VirtualQuery failed gle=%lu", label, address, GetLastError());
        return;
    }

    Log("memory[%s]: address=0x%08X base=0x%08X allocBase=0x%08X size=0x%08X state=%s protect=%s rawProtect=0x%08lX type=%s",
        label,
        address,
        reinterpret_cast<uintptr_t>(mbi.BaseAddress),
        reinterpret_cast<uintptr_t>(mbi.AllocationBase),
        static_cast<uintptr_t>(mbi.RegionSize),
        StateName(mbi.State),
        ProtectName(mbi.Protect),
        mbi.Protect,
        TypeName(mbi.Type));
}

void LogStackModules(uintptr_t esp)
{
    if (!g_config.enableStackScanDiagnostics) {
        return;
    }

    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(reinterpret_cast<const void*>(esp), &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT ||
        (mbi.Protect & PAGE_NOACCESS) || (mbi.Protect & PAGE_GUARD)) {
        Log("stack-scan: esp=0x%08X not readable", esp);
        return;
    }

    Log("stack-scan begin: esp=0x%08X", esp);
    for (int i = 0; i < 32; ++i) {
        uintptr_t value = 0;
        __try {
            value = reinterpret_cast<const uintptr_t*>(esp)[i];
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Log("stack-scan: +0x%03X unreadable", i * 4);
            break;
        }

        char moduleName[MAX_PATH]{};
        const uintptr_t moduleBase = ModuleBaseFromAddress(value, moduleName, sizeof(moduleName));
        if (moduleBase) {
            Log("stack-scan: +0x%03X value=0x%08X -> %s+0x%X", i * 4, value, moduleName, value - moduleBase);
        } else if (value >= 0x00400000 && value < 0x80000000) {
            Log("stack-scan: +0x%03X value=0x%08X -> <no-module>", i * 4, value);
        }
    }
    Log("stack-scan end");
}

void LogDwords(const char* label, uintptr_t address, int count)
{
    LogMemoryRegion(label, address);
    for (int i = 0; i < count; ++i) {
        uint32_t value = 0;
        const uintptr_t current = address + static_cast<uintptr_t>(i * sizeof(uint32_t));
        if (!SafeReadU32(current, &value)) {
            Log("dump[%s]: +0x%02X unreadable", label, i * 4);
            break;
        }

        char moduleName[MAX_PATH]{};
        const uintptr_t moduleBase = ModuleBaseFromAddress(value, moduleName, sizeof(moduleName));
        if (moduleBase) {
            Log("dump[%s]: +0x%02X = 0x%08X -> %s+0x%X", label, i * 4, value, moduleName, value - moduleBase);
        } else {
            Log("dump[%s]: +0x%02X = 0x%08X", label, i * 4, value);
        }
    }
}

void LogBytes(const char* label, uintptr_t address, size_t count)
{
    LogMemoryRegion(label, address);
    if (!count || !IsReadableCommitted(address, count)) {
        Log("bytes[%s]: address=0x%08X count=0x%X unreadable", label, address, static_cast<unsigned>(count));
        return;
    }

    uint8_t bytes[128]{};
    const size_t limited = count < sizeof(bytes) ? count : sizeof(bytes);
    __try {
        std::memcpy(bytes, reinterpret_cast<const void*>(address), limited);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("bytes[%s]: address=0x%08X count=0x%X read exception", label, address, static_cast<unsigned>(limited));
        return;
    }

    for (size_t i = 0; i < limited; i += 16) {
        char line[3 * 16 + 1]{};
        size_t pos = 0;
        const size_t lineCount = (limited - i) < 16 ? (limited - i) : 16;
        for (size_t j = 0; j < lineCount && pos + 4 < sizeof(line); ++j) {
            const int written = sprintf_s(line + pos, sizeof(line) - pos, "%02X ", bytes[i + j]);
            if (written <= 0) {
                break;
            }
            pos += static_cast<size_t>(written);
        }
        Log("bytes[%s]: 0x%08X: %s", label, address + i, line);
    }
}

