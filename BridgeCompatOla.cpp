#include "FLACompatBridgeInternal.h"

bool IniValueLooksEnabled(const char* value)
{
    if (!value) {
        return false;
    }

    while (*value == ' ' || *value == '\t') {
        ++value;
    }
    if (*value == '\0') {
        return false;
    }
    if ((*value == '0') && (value[1] == '\0' || value[1] == ' ' || value[1] == '\t')) {
        return false;
    }
    if (_stricmp(value, "false") == 0 || _stricmp(value, "off") == 0 || _stricmp(value, "disabled") == 0) {
        return false;
    }
    return true;
}

bool FlaOwnsOlaSaLimit(const OlaSaOverlapSpec& spec)
{
    if (!spec.flaKey || !spec.flaKey[0]) {
        return false;
    }

    char value[128]{};
    if (!ReadSmallTextValue(g_flaIniPath, spec.flaKey, value, sizeof(value))) {
        return false;
    }

    return IniValueLooksEnabled(value);
}

const OlaSaOverlapSpec* FindOlaSaOverlapSpec(const char* key)
{
    if (!key || !key[0]) {
        return nullptr;
    }

    for (const OlaSaOverlapSpec& spec : kOlaSaOverlapSpecs) {
        if (_stricmp(key, spec.key) == 0) {
            return &spec;
        }
    }
    return nullptr;
}

bool TokenListContainsExact(const char* rawList, const char* value)
{
    if (!rawList || !rawList[0] || !value || !value[0]) {
        return false;
    }

    char list[512]{};
    strncpy_s(list, rawList, _TRUNCATE);

    char* context = nullptr;
    for (char* token = strtok_s(list, ";, \t", &context);
        token;
        token = strtok_s(nullptr, ";, \t", &context)) {
        if (_stricmp(token, value) == 0) {
            return true;
        }
    }

    return false;
}

bool ExtractIniSectionName(const char* line, size_t len, char* out, size_t outSize)
{
    if (!line || !out || outSize == 0) {
        return false;
    }

    size_t start = 0;
    while (start < len && (line[start] == ' ' || line[start] == '\t')) {
        ++start;
    }
    if (start >= len || line[start] != '[') {
        return false;
    }

    size_t end = start + 1;
    while (end < len && line[end] != ']') {
        ++end;
    }
    if (end >= len || line[end] != ']') {
        return false;
    }

    size_t nameStart = start + 1;
    while (nameStart < end && (line[nameStart] == ' ' || line[nameStart] == '\t')) {
        ++nameStart;
    }

    size_t nameEnd = end;
    while (nameEnd > nameStart && (line[nameEnd - 1] == ' ' || line[nameEnd - 1] == '\t')) {
        --nameEnd;
    }

    const size_t copyLen = (nameEnd > nameStart) ? min(nameEnd - nameStart, outSize - 1) : 0;
    if (copyLen == 0) {
        out[0] = '\0';
        return false;
    }

    std::memcpy(out, line + nameStart, copyLen);
    out[copyLen] = '\0';
    return true;
}

bool ExtractIniKeyName(const char* line, size_t len, char* out, size_t outSize)
{
    if (!line || !out || outSize == 0) {
        return false;
    }

    size_t start = 0;
    while (start < len && (line[start] == ' ' || line[start] == '\t')) {
        ++start;
    }

    if (start >= len ||
        line[start] == ';' ||
        line[start] == '#' ||
        line[start] == '\r' ||
        line[start] == '\n' ||
        line[start] == '\0') {
        return false;
    }

    size_t eq = start;
    while (eq < len && line[eq] != '=' && line[eq] != '\r' && line[eq] != '\n' && line[eq] != '\0') {
        ++eq;
    }
    if (eq >= len || line[eq] != '=') {
        return false;
    }

    size_t keyEnd = eq;
    while (keyEnd > start && (line[keyEnd - 1] == ' ' || line[keyEnd - 1] == '\t')) {
        --keyEnd;
    }
    if (keyEnd <= start) {
        return false;
    }

    const size_t copyLen = min(keyEnd - start, outSize - 1);
    std::memcpy(out, line + start, copyLen);
    out[copyLen] = '\0';
    return true;
}

bool AppendText(char* output, size_t capacity, size_t* used, const char* text, size_t len)
{
    if (!output || !used || !text) {
        return false;
    }
    if (*used + len >= capacity) {
        return false;
    }

    std::memcpy(output + *used, text, len);
    *used += len;
    output[*used] = '\0';
    return true;
}

bool ExtractModloaderIgnoreEntry(const char* line, size_t len, char* out, size_t outSize)
{
    if (!line || !out || outSize == 0) {
        return false;
    }

    size_t start = 0;
    while (start < len && (line[start] == ' ' || line[start] == '\t')) {
        ++start;
    }

    if (start >= len ||
        line[start] == ';' ||
        line[start] == '#' ||
        line[start] == '[' ||
        line[start] == '\r' ||
        line[start] == '\n' ||
        line[start] == '\0') {
        return false;
    }

    size_t end = start;
    while (end < len &&
        line[end] != ';' &&
        line[end] != '#' &&
        line[end] != '\r' &&
        line[end] != '\n' &&
        line[end] != '\0') {
        ++end;
    }

    while (end > start && (line[end - 1] == ' ' || line[end - 1] == '\t')) {
        --end;
    }

    if (end <= start) {
        return false;
    }

    const size_t copyLen = min(end - start, outSize - 1);
    std::memcpy(out, line + start, copyLen);
    out[copyLen] = '\0';
    return true;
}

void GuardOpenLimitAdjusterModuleLoad(const char* phase)
{
    if (!g_config.enableOpenLimitAdjusterModuleGuard) {
        return;
    }

    const char* olaAsi = "modloader\\OLA\\III.VC.SA.LimitAdjuster.asi";
    if (GetFileAttributesA(olaAsi) == INVALID_FILE_ATTRIBUTES) {
        Log("OLA module guard: ASI not found path=%s phase=%s", olaAsi, phase ? phase : "");
        return;
    }

    const char* path = "modloader\\modloader.ini";
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        Log("OLA module guard: modloader config open failed path=%s gle=%lu phase=%s",
            path,
            GetLastError(),
            phase ? phase : "");
        return;
    }

    const DWORD fileSize = GetFileSize(file, nullptr);
    if (fileSize == INVALID_FILE_SIZE || fileSize > 1024 * 1024) {
        Log("OLA module guard: refusing modloader config size=%lu path=%s phase=%s",
            fileSize,
            path,
            phase ? phase : "");
        CloseHandle(file);
        return;
    }

    char* input = new char[fileSize + 1];
    DWORD bytesRead = 0;
    const BOOL readOk = ReadFile(file, input, fileSize, &bytesRead, nullptr);
    CloseHandle(file);
    if (!readOk) {
        Log("OLA module guard: modloader config read failed path=%s gle=%lu phase=%s",
            path,
            GetLastError(),
            phase ? phase : "");
        delete[] input;
        return;
    }
    input[bytesRead] = '\0';

    const char* newline = std::strstr(input, "\r\n") ? "\r\n" : "\n";
    const char* comment = "; FLA++: OLA ASI is isolated because FLA++ owns SA limits in this setup.";
    const char* entry = "OLA";

    const size_t outputCapacity = static_cast<size_t>(bytesRead) + 4096;
    char* output = new char[outputCapacity];
    size_t outputUsed = 0;
    output[0] = '\0';

    bool inIgnoreMods = false;
    bool foundIgnoreMods = false;
    bool hasOlaIgnore = false;
    bool inserted = false;
    bool appendOk = true;

    auto appendOlaIgnore = [&]() -> bool {
        if (!AppendText(output, outputCapacity, &outputUsed, comment, std::strlen(comment)) ||
            !AppendText(output, outputCapacity, &outputUsed, newline, std::strlen(newline)) ||
            !AppendText(output, outputCapacity, &outputUsed, entry, std::strlen(entry)) ||
            !AppendText(output, outputCapacity, &outputUsed, newline, std::strlen(newline))) {
            return false;
        }
        inserted = true;
        return true;
    };

    size_t pos = 0;
    while (pos < bytesRead) {
        const size_t lineStart = pos;
        while (pos < bytesRead && input[pos] != '\n') {
            ++pos;
        }
        if (pos < bytesRead && input[pos] == '\n') {
            ++pos;
        }

        const char* line = input + lineStart;
        const size_t lineLen = pos - lineStart;

        char section[96]{};
        const bool isSection = ExtractIniSectionName(line, lineLen, section, sizeof(section));
        if (isSection && inIgnoreMods && !hasOlaIgnore && !inserted) {
            appendOk = appendOlaIgnore();
            if (!appendOk) {
                break;
            }
        }

        appendOk = AppendText(output, outputCapacity, &outputUsed, line, lineLen);
        if (!appendOk) {
            break;
        }

        if (isSection) {
            inIgnoreMods = _stricmp(section, "Profiles.Default.IgnoreMods") == 0;
            if (inIgnoreMods) {
                foundIgnoreMods = true;
            }
            continue;
        }

        if (inIgnoreMods) {
            char ignoreEntry[260]{};
            if (ExtractModloaderIgnoreEntry(line, lineLen, ignoreEntry, sizeof(ignoreEntry)) &&
                _stricmp(ignoreEntry, "OLA") == 0) {
                hasOlaIgnore = true;
            }
        }
    }

    if (appendOk && !hasOlaIgnore && !inserted) {
        if (!foundIgnoreMods) {
            appendOk = AppendText(output, outputCapacity, &outputUsed, newline, std::strlen(newline)) &&
                AppendText(output, outputCapacity, &outputUsed, "[Profiles.Default.IgnoreMods]", 29) &&
                AppendText(output, outputCapacity, &outputUsed, newline, std::strlen(newline));
        }
        if (appendOk) {
            appendOk = appendOlaIgnore();
        }
    }

    if (!appendOk) {
        Log("OLA module guard: output buffer exhausted path=%s phase=%s", path, phase ? phase : "");
        delete[] output;
        delete[] input;
        return;
    }

    if (hasOlaIgnore) {
        Log("OLA module guard: already ignored in modloader config path=%s phase=%s", path, phase ? phase : "");
        delete[] output;
        delete[] input;
        return;
    }

    if (inserted) {
        CopyFileA(path, "modloader\\modloader.ini.fla++bak", FALSE);

        char tempPath[MAX_PATH]{};
        sprintf_s(tempPath, "%s.fla++tmp", path);
        HANDLE outFile = CreateFileA(tempPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (outFile == INVALID_HANDLE_VALUE) {
            Log("OLA module guard: temp open failed path=%s gle=%lu phase=%s",
                tempPath,
                GetLastError(),
                phase ? phase : "");
            delete[] output;
            delete[] input;
            return;
        }

        DWORD written = 0;
        const BOOL writeOk = WriteFile(outFile, output, static_cast<DWORD>(outputUsed), &written, nullptr);
        CloseHandle(outFile);

        if (!writeOk || written != outputUsed) {
            Log("OLA module guard: temp write failed path=%s written=%lu expected=%u gle=%lu phase=%s",
                tempPath,
                written,
                static_cast<unsigned>(outputUsed),
                GetLastError(),
                phase ? phase : "");
            DeleteFileA(tempPath);
            delete[] output;
            delete[] input;
            return;
        }

        if (!MoveFileExA(tempPath, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
            Log("OLA module guard: replace failed temp=%s path=%s gle=%lu phase=%s",
                tempPath,
                path,
                GetLastError(),
                phase ? phase : "");
            DeleteFileA(tempPath);
            delete[] output;
            delete[] input;
            return;
        }

        Log("OLA module guard: added OLA to [Profiles.Default.IgnoreMods] path=%s phase=%s",
            path,
            phase ? phase : "");
    }

    delete[] output;
    delete[] input;
}

void GuardOpenLimitAdjusterSaLimitsAtPath(const char* path, const char* phase)
{
    if (!g_config.enableOpenLimitAdjusterSaLimitGuard) {
        return;
    }

    char resolvedPath[MAX_PATH]{};
    ResolveGamePath(path, resolvedPath, sizeof(resolvedPath));
    if (!resolvedPath[0] || GetFileAttributesA(resolvedPath) == INVALID_FILE_ATTRIBUTES) {
        Log("OLA SA limit guard: config not found path=%s phase=%s", resolvedPath[0] ? resolvedPath : path, phase ? phase : "");
        return;
    }

    HANDLE file = CreateFileA(resolvedPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        Log("OLA SA limit guard: open failed path=%s gle=%lu phase=%s", resolvedPath, GetLastError(), phase ? phase : "");
        return;
    }

    const DWORD fileSize = GetFileSize(file, nullptr);
    if (fileSize == INVALID_FILE_SIZE || fileSize > 1024 * 1024) {
        Log("OLA SA limit guard: refusing file size=%lu path=%s phase=%s", fileSize, resolvedPath, phase ? phase : "");
        CloseHandle(file);
        return;
    }

    char* input = new char[fileSize + 1];
    DWORD bytesRead = 0;
    const BOOL readOk = ReadFile(file, input, fileSize, &bytesRead, nullptr);
    CloseHandle(file);
    if (!readOk) {
        Log("OLA SA limit guard: read failed path=%s gle=%lu phase=%s", resolvedPath, GetLastError(), phase ? phase : "");
        delete[] input;
        return;
    }
    input[bytesRead] = '\0';

    const size_t outputCapacity = static_cast<size_t>(bytesRead) + 65536;
    char* output = new char[outputCapacity];
    size_t outputUsed = 0;
    output[0] = '\0';

    bool inSaLimits = false;
    int disabled = 0;
    int allowed = 0;
    int alreadyCommented = 0;
    int removedAutoComments = 0;
    bool appendOk = true;
    static const char disabledPrefix[] = "; FLA++ disabled OLA SA overlap: ";
    constexpr size_t disabledPrefixLen = sizeof(disabledPrefix) - 1;

    size_t pos = 0;
    while (pos < bytesRead) {
        const size_t lineStart = pos;
        while (pos < bytesRead && input[pos] != '\n') {
            ++pos;
        }
        if (pos < bytesRead && input[pos] == '\n') {
            ++pos;
        }

        const char* line = input + lineStart;
        const size_t lineLen = pos - lineStart;

        char section[64]{};
        if (ExtractIniSectionName(line, lineLen, section, sizeof(section))) {
            inSaLimits = _stricmp(section, "SALIMITS") == 0;
        }

        const char* trimmed = line;
        while (trimmed < line + lineLen && (*trimmed == ' ' || *trimmed == '\t')) {
            ++trimmed;
        }
        if (inSaLimits &&
            static_cast<size_t>((line + lineLen) - trimmed) >= disabledPrefixLen &&
            _strnicmp(trimmed, disabledPrefix, disabledPrefixLen) == 0) {
            ++removedAutoComments;
            continue;
        }

        const OlaSaOverlapSpec* spec = nullptr;
        char key[96]{};
        if (inSaLimits && ExtractIniKeyName(line, lineLen, key, sizeof(key))) {
            spec = FindOlaSaOverlapSpec(key);
        }

        if (spec && TokenListContainsExact(g_config.openLimitAdjusterSaLimitAllowlist, spec->key)) {
            ++allowed;
            Log("OLA SA limit guard: allowlisted key='%s' phase=%s", spec->key, phase ? phase : "");
            spec = nullptr;
        }

        if (spec && !FlaOwnsOlaSaLimit(*spec)) {
            ++allowed;
            Log("OLA SA limit guard: allowed key='%s' owner='OLA' reason='FLA key inactive or unmapped' flaKey='%s' phase=%s",
                spec->key,
                spec->flaKey ? spec->flaKey : "",
                phase ? phase : "");
            spec = nullptr;
        }

        if (spec) {
            appendOk = AppendText(output, outputCapacity, &outputUsed, "#", 1) &&
                AppendText(output, outputCapacity, &outputUsed, line, lineLen);
            ++disabled;
            Log("OLA SA limit guard: disabled key='%s' owner='%s' phase=%s",
                spec->key,
                spec->owner,
                phase ? phase : "");
        } else {
            appendOk = AppendText(output, outputCapacity, &outputUsed, line, lineLen);
        }

        if (!appendOk) {
            break;
        }

        if (inSaLimits && (*trimmed == ';' || *trimmed == '#')) {
            char commentedKey[96]{};
            const char* p = trimmed + 1;
            while (p < line + lineLen && (*p == ' ' || *p == '\t')) {
                ++p;
            }
            if (ExtractIniKeyName(p, static_cast<size_t>((line + lineLen) - p), commentedKey, sizeof(commentedKey)) &&
                FindOlaSaOverlapSpec(commentedKey)) {
                ++alreadyCommented;
            }
        }
    }

    if (!appendOk) {
        Log("OLA SA limit guard: output buffer exhausted path=%s phase=%s", resolvedPath, phase ? phase : "");
        delete[] output;
        delete[] input;
        return;
    }

    if (disabled > 0 || removedAutoComments > 0) {
        char backupPath[MAX_PATH]{};
        sprintf_s(backupPath, "%s.fla++bak", resolvedPath);
        CopyFileA(resolvedPath, backupPath, FALSE);

        char tempPath[MAX_PATH]{};
        sprintf_s(tempPath, "%s.fla++tmp", resolvedPath);
        HANDLE outFile = CreateFileA(tempPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (outFile == INVALID_HANDLE_VALUE) {
            Log("OLA SA limit guard: temp open failed path=%s gle=%lu phase=%s", tempPath, GetLastError(), phase ? phase : "");
            delete[] output;
            delete[] input;
            return;
        }

        DWORD written = 0;
        const BOOL writeOk = WriteFile(outFile, output, static_cast<DWORD>(outputUsed), &written, nullptr);
        CloseHandle(outFile);

        if (!writeOk || written != outputUsed) {
            Log("OLA SA limit guard: temp write failed path=%s written=%lu expected=%u gle=%lu phase=%s",
                tempPath,
                written,
                static_cast<unsigned>(outputUsed),
                GetLastError(),
                phase ? phase : "");
            DeleteFileA(tempPath);
            delete[] output;
            delete[] input;
            return;
        }

        if (!MoveFileExA(tempPath, resolvedPath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
            Log("OLA SA limit guard: replace failed temp=%s path=%s gle=%lu phase=%s",
                tempPath,
                resolvedPath,
                GetLastError(),
                phase ? phase : "");
            DeleteFileA(tempPath);
            delete[] output;
            delete[] input;
            return;
        }
    }

    Log("OLA SA limit guard: done disabled=%d allowlisted=%d alreadyCommented=%d removedAutoComments=%d path=%s phase=%s",
        disabled,
        allowed,
        alreadyCommented,
        removedAutoComments,
        resolvedPath,
        phase ? phase : "");

    delete[] output;
    delete[] input;
}

void GuardOpenLimitAdjusterSaLimits(const char* phase)
{
    static const char* paths[] = {
        "III.VC.SA.LimitAdjuster.ini",
        "modloader\\OLA\\III.VC.SA.LimitAdjuster.ini",
        "Temp\\OLA\\III.VC.SA.LimitAdjuster.ini",
    };

    for (const char* path : paths) {
        GuardOpenLimitAdjusterSaLimitsAtPath(path, phase);
    }
}

void AuditOpenLimitAdjusterSaOverlapsAtPath(const char* path)
{
    if (!g_config.enableOpenLimitAdjusterOverlapAudit) {
        return;
    }

    char resolvedPath[MAX_PATH]{};
    ResolveGamePath(path, resolvedPath, sizeof(resolvedPath));
    if (!resolvedPath[0] || GetFileAttributesA(resolvedPath) == INVALID_FILE_ATTRIBUTES) {
        Log("OLA overlap audit: config not found path=%s", resolvedPath[0] ? resolvedPath : path);
        return;
    }

    int active = 0;
    int conflicts = 0;
    int allowed = 0;
    for (const OlaSaOverlapSpec& spec : kOlaSaOverlapSpecs) {
        char value[128]{};
        if (!ReadSectionTextValue(resolvedPath, "SALIMITS", spec.key, value, sizeof(value))) {
            continue;
        }
        if (!IniValueLooksEnabled(value)) {
            continue;
        }

        ++active;
        if (FlaOwnsOlaSaLimit(spec)) {
            ++conflicts;
            Log("OLA overlap audit: active SA limit key='%s' value='%s' conflictsWith='%s' flaKey='%s' action=guard-will-comment",
                spec.key,
                value,
                spec.owner,
                spec.flaKey ? spec.flaKey : "");
        } else {
            ++allowed;
            Log("OLA overlap audit: active SA limit key='%s' value='%s' owner=OLA allowed=1 flaKey='%s'",
                spec.key,
                value,
                spec.flaKey ? spec.flaKey : "");
        }
    }

    if (active == 0) {
        Log("OLA overlap audit: OK no active [SALIMITS] entries");
    } else if (conflicts == 0) {
        Log("OLA overlap audit: OK active=%d allowedByInactiveFla=%d conflicts=0", active, allowed);
    } else {
        Log("OLA overlap audit: WARNING active=%d conflicts=%d allowedByInactiveFla=%d; OLA and FLA are both trying to own these limits", active, conflicts, allowed);
    }
}

void AuditOpenLimitAdjusterSaOverlaps()
{
    static const char* paths[] = {
        "III.VC.SA.LimitAdjuster.ini",
        "modloader\\OLA\\III.VC.SA.LimitAdjuster.ini",
        "Temp\\OLA\\III.VC.SA.LimitAdjuster.ini",
    };

    for (const char* path : paths) {
        AuditOpenLimitAdjusterSaOverlapsAtPath(path);
    }
}

bool IsOpenLimitAdjusterAddress(uintptr_t address)
{
    if (!address) {
        return false;
    }

    char moduleName[MAX_PATH]{};
    ModuleBaseFromAddress(address, moduleName, sizeof(moduleName));
    return _stricmp(BaseName(moduleName), "III.VC.SA.LimitAdjuster.asi") == 0;
}

bool PatchEntryLooksOpenLimitAdjusterOwned(uintptr_t address)
{
    if (!IsReadableCommitted(address, 5)) {
        return false;
    }

    const uint8_t opcode = *reinterpret_cast<const uint8_t*>(address);
    if (opcode != 0xE8 && opcode != 0xE9) {
        return false;
    }

    const uintptr_t target = DecodeRel32JumpTarget(address);
    return IsOpenLimitAdjusterAddress(target);
}

bool RestoreOpenLimitAdjusterPatchIfOwned(const char* label, uintptr_t address, const uint8_t* originalBytes, size_t size, const char* phase)
{
    if (!PatchEntryLooksOpenLimitAdjusterOwned(address)) {
        return false;
    }

    const uintptr_t target = DecodeRel32JumpTarget(address);
    char moduleName[MAX_PATH]{};
    ModuleBaseFromAddress(target, moduleName, sizeof(moduleName));

    if (WriteBytesWithProtect(address, originalBytes, size)) {
        Log("OLA hook repair: restored %s at 0x%08X size=%u oldTarget=0x%08X module=%s phase=%s",
            label,
            static_cast<unsigned>(address),
            static_cast<unsigned>(size),
            static_cast<unsigned>(target),
            moduleName,
            phase ? phase : "");
        return true;
    } else {
        Log("OLA hook repair: restore failed %s at 0x%08X size=%u oldTarget=0x%08X module=%s gle=%lu phase=%s",
            label,
            static_cast<unsigned>(address),
            static_cast<unsigned>(size),
            static_cast<unsigned>(target),
            moduleName,
            GetLastError(),
            phase ? phase : "");
        return false;
    }
}

int RepairOpenLimitAdjusterSaPoolHooks(const char* phase)
{
    static LONG logOnce = 0;
    if (InterlockedCompareExchange(&logOnce, 1, 0) == 0) {
        Log("OLA hook repair: runtime vanilla-byte restoration disabled; FLA hooks remain authoritative phase=%s",
            phase ? phase : "");
    }
    return 0;
}

DWORD WINAPI OpenLimitAdjusterRepairThread(void*)
{
    return 0;
}

