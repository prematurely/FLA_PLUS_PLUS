#include "FLACompatBridgeInternal.h"

const char* GetBuiltInSpecialActorName(uint32_t modelId)
{
    switch (modelId) {
    case 290: return "SWEET";
    case 291: return "RYDER";
    case 292: return "SMOKEV";
    case 293: return "ZERO";
    case 294: return "CESAR";
    case 295: return "TENPEN";
    case 296: return "PULASKI";
    case 297: return "TRUTH";
    case 298: return "OGLOC";
    case 299: return "KENDL";
    default: return nullptr;
    }
}

const char* GetBuiltInSpecialActorCatalogName(uint32_t actorCode)
{
    for (const auto& entry : kBuiltInSpecialActorCatalog) {
        if (entry.code == actorCode) {
            return entry.name;
        }
    }

    return nullptr;
}

bool IsBuiltInSpecialActorCatalogName(const char* name)
{
    if (!name || !name[0]) {
        return false;
    }

    for (const auto& entry : kBuiltInSpecialActorCatalog) {
        if (_stricmp(entry.name, name) == 0) {
            return true;
        }
    }

    return false;
}

bool NormalizeSpecialActorName(const char* input, char* out, size_t outSize)
{
    if (!input || !out || outSize == 0) {
        return false;
    }

    size_t w = 0;
    for (size_t r = 0; input[r] && w + 1 < outSize; ++r) {
        const unsigned char c = static_cast<unsigned char>(input[r]);
        if (c >= 'a' && c <= 'z') {
            out[w++] = static_cast<char>(c - 'a' + 'A');
        } else if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
            out[w++] = static_cast<char>(c);
        } else if (c == '.') {
            break;
        }
    }
    out[w] = '\0';
    return w > 0;
}

bool AutoScanFilterAllowsName(const char* name)
{
    if (!name || !name[0]) {
        return false;
    }
    if (std::strcmp(g_config.specialActorAutoScanFilter, "*") == 0) {
        return true;
    }

    char filter[sizeof(g_config.specialActorAutoScanFilter)]{};
    strncpy_s(filter, g_config.specialActorAutoScanFilter, _TRUNCATE);

    char* context = nullptr;
    for (char* token = strtok_s(filter, ";,", &context); token; token = strtok_s(nullptr, ";,", &context)) {
        while (*token == ' ' || *token == '\t') {
            ++token;
        }
        char* end = token + std::strlen(token);
        while (end > token && (end[-1] == ' ' || end[-1] == '\t')) {
            --end;
        }
        *end = '\0';
        if (!token[0]) {
            continue;
        }
        const size_t tokenLen = std::strlen(token);
        bool wildcardPrefix = false;
        if (tokenLen > 0 && token[tokenLen - 1] == '*') {
            token[tokenLen - 1] = '\0';
            wildcardPrefix = true;
        }
        const size_t matchLen = std::strlen(token);
        if (!matchLen) {
            continue;
        }

        if (_stricmp(name, token) == 0 ||
            (wildcardPrefix && _strnicmp(name, token, matchLen) == 0) ||
            (_strnicmp(name, token, matchLen) == 0 && name[matchLen] >= '0' && name[matchLen] <= '9')) {
            return true;
        }
    }

    return false;
}

bool AddRuntimeSpecialActorName(const char* rawName, const char* source)
{
    char name[kSpecialActorNameLength]{};
    if (!NormalizeSpecialActorName(rawName, name, sizeof(name)) || !AutoScanFilterAllowsName(name)) {
        return false;
    }

    if (IsBuiltInSpecialActorCatalogName(name)) {
        return false;
    }

    for (uint32_t i = 0; i < g_runtimeSpecialActorNameCount; ++i) {
        if (_stricmp(g_runtimeSpecialActorNames[i].name, name) == 0) {
            return false;
        }
    }

    if (g_runtimeSpecialActorNameCount >= kMaxRuntimeSpecialActorNames ||
        g_runtimeSpecialActorNameCount >= static_cast<uint32_t>(g_config.specialActorMaxAutoCatalogNames)) {
        return false;
    }

    RuntimeSpecialActorName& entry = g_runtimeSpecialActorNames[g_runtimeSpecialActorNameCount];
    entry.code = static_cast<uint32_t>(g_config.specialActorAutoCatalogFirstCode) + g_runtimeSpecialActorNameCount;
    strncpy_s(entry.name, name, _TRUNCATE);
    strncpy_s(entry.source, source ? source : "scan", _TRUNCATE);
    ++g_runtimeSpecialActorNameCount;
    return true;
}

const char* GetRuntimeSpecialActorCatalogName(uint32_t actorCode)
{
    for (uint32_t i = 0; i < g_runtimeSpecialActorNameCount; ++i) {
        if (g_runtimeSpecialActorNames[i].code == actorCode) {
            return g_runtimeSpecialActorNames[i].name;
        }
    }

    return nullptr;
}

bool ExtractImgEntryBaseName(const uint8_t* bytes, size_t size, const char* extension, char* out, size_t outSize)
{
    if (!bytes || !extension || !out || outSize == 0) {
        return false;
    }

    char text[32]{};
    size_t w = 0;
    for (size_t i = 0; i < size && w + 1 < sizeof(text); ++i) {
        const unsigned char c = bytes[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '.') {
            text[w++] = static_cast<char>(c);
        } else {
            break;
        }
    }
    text[w] = '\0';

    const size_t extLen = std::strlen(extension);
    const size_t textLen = std::strlen(text);
    if (textLen <= extLen + 1 || text[textLen - extLen - 1] != '.' ||
        _stricmp(text + textLen - extLen, extension) != 0) {
        return false;
    }

    text[textLen - extLen - 1] = '\0';
    return NormalizeSpecialActorName(text, out, outSize);
}

bool ImgArchiveHasTxdName(const char* imgPath, const char* wantedName)
{
    if (!imgPath || !wantedName || !wantedName[0]) {
        return false;
    }

    FILE* file = nullptr;
    if (fopen_s(&file, imgPath, "rb") != 0 || !file) {
        return false;
    }

    char magic[4]{};
    uint32_t count = 0;
    if (std::fread(magic, 1, sizeof(magic), file) != sizeof(magic) ||
        std::fread(&count, sizeof(count), 1, file) != 1 ||
        std::memcmp(magic, "VER2", 4) != 0 ||
        count > 200000) {
        std::fclose(file);
        return false;
    }

    bool found = false;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t offset = 0;
        uint32_t size = 0;
        uint8_t bytes[24]{};
        if (std::fread(&offset, sizeof(offset), 1, file) != 1 ||
            std::fread(&size, sizeof(size), 1, file) != 1 ||
            std::fread(bytes, 1, sizeof(bytes), file) != sizeof(bytes)) {
            break;
        }
        char name[kSpecialActorNameLength]{};
        if (ExtractImgEntryBaseName(bytes, sizeof(bytes), "txd", name, sizeof(name)) &&
            _stricmp(name, wantedName) == 0) {
            found = true;
            break;
        }
    }

    std::fclose(file);
    return found;
}

void ScanSpecialActorsFromImgArchive(const char* imgPath, const char* sourceLabel)
{
    if (!imgPath) {
        return;
    }

    FILE* file = nullptr;
    if (fopen_s(&file, imgPath, "rb") != 0 || !file) {
        return;
    }

    char magic[4]{};
    uint32_t count = 0;
    if (std::fread(magic, 1, sizeof(magic), file) != sizeof(magic) ||
        std::fread(&count, sizeof(count), 1, file) != 1 ||
        std::memcmp(magic, "VER2", 4) != 0 ||
        count > 200000) {
        std::fclose(file);
        return;
    }

    uint32_t added = 0;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t offset = 0;
        uint32_t size = 0;
        uint8_t bytes[24]{};
        if (std::fread(&offset, sizeof(offset), 1, file) != 1 ||
            std::fread(&size, sizeof(size), 1, file) != 1 ||
            std::fread(bytes, 1, sizeof(bytes), file) != sizeof(bytes)) {
            break;
        }

        char name[kSpecialActorNameLength]{};
        if (!ExtractImgEntryBaseName(bytes, sizeof(bytes), "dff", name, sizeof(name))) {
            continue;
        }
        if (!g_config.specialActorAllowMissingTxd && !ImgArchiveHasTxdName(imgPath, name)) {
            continue;
        }
        if (AddRuntimeSpecialActorName(name, sourceLabel)) {
            ++added;
        }
    }

    std::fclose(file);
    Log("special actor catalog: scanned IMG '%s' added=%u totalAuto=%u", imgPath, added, g_runtimeSpecialActorNameCount);
}

bool HasSiblingTxdForDff(const char* dffPath, const char* baseName)
{
    char txdPath[MAX_PATH]{};
    strncpy_s(txdPath, dffPath, _TRUNCATE);
    char* slash1 = std::strrchr(txdPath, '\\');
    char* slash2 = std::strrchr(txdPath, '/');
    char* slash = slash1 > slash2 ? slash1 : slash2;
    char* fileName = slash ? slash + 1 : txdPath;
    sprintf_s(fileName, MAX_PATH - (fileName - txdPath), "%s.txd", baseName);
    if (GetFileAttributesA(txdPath) != INVALID_FILE_ATTRIBUTES) {
        return true;
    }
    sprintf_s(fileName, MAX_PATH - (fileName - txdPath), "%s.TXD", baseName);
    return GetFileAttributesA(txdPath) != INVALID_FILE_ATTRIBUTES;
}

void ScanSpecialActorsFromDirectory(const char* root, const char* sourceLabel, int depth) {
    if (!root || depth > 12) {
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
            ScanSpecialActorsFromDirectory(path, sourceLabel, depth + 1);
            continue;
        }

        const char* dot = std::strrchr(data.cFileName, '.');
        if (!dot) {
            continue;
        }
        if (_stricmp(dot, ".img") == 0) {
            ScanSpecialActorsFromImgArchive(path, sourceLabel);
            continue;
        }
        if (_stricmp(dot, ".dff") != 0) {
            continue;
        }

        char base[kSpecialActorNameLength]{};
        char rawName[64]{};
        strncpy_s(rawName, data.cFileName, _TRUNCATE);
        char* rawDot = std::strrchr(rawName, '.');
        if (rawDot) {
            *rawDot = '\0';
        }
        if (!NormalizeSpecialActorName(rawName, base, sizeof(base))) {
            continue;
        }
        if (!g_config.specialActorAllowMissingTxd && !HasSiblingTxdForDff(path, base)) {
            continue;
        }
        AddRuntimeSpecialActorName(base, sourceLabel);
    } while (FindNextFileA(find, &data));

    FindClose(find);
}

void BuildSpecialActorRuntimeCatalog()
{
    g_runtimeSpecialActorNameCount = 0;

    if (g_config.specialActorMaxAutoCatalogNames <= 0) {
        return;
    }

    if (g_config.specialActorAutoScanImgArchives) {
        ScanSpecialActorsFromDirectory("models", "models");
    }

    if (g_config.specialActorAutoScanModloader) {
        ScanSpecialActorsFromDirectory("modloader", "modloader");
    }

    Log("special actor catalog: built builtIn=%u auto=%u firstAutoCode=%d",
        static_cast<unsigned>(sizeof(kBuiltInSpecialActorCatalog) / sizeof(kBuiltInSpecialActorCatalog[0])),
        g_runtimeSpecialActorNameCount,
        g_config.specialActorAutoCatalogFirstCode);
    if (g_config.specialActorLogCatalog) {
        for (uint32_t i = 0; i < g_runtimeSpecialActorNameCount && i < 128; ++i) {
            Log("special actor catalog: code=%u name=%s source=%s",
                g_runtimeSpecialActorNames[i].code,
                g_runtimeSpecialActorNames[i].name,
                g_runtimeSpecialActorNames[i].source);
        }
    }
}

DWORD WINAPI SpecialActorRuntimeCatalogThread(void*)
{
    Sleep(static_cast<DWORD>(g_config.specialActorCatalogBuildDelayMs));
    BuildSpecialActorRuntimeCatalog();
    return 0;
}

bool IsSpecialActorSlotUsable(uint32_t modelId)
{
    const bool configuredRange =
        modelId >= static_cast<uint32_t>(g_config.specialActorMinModelId) &&
        modelId <= static_cast<uint32_t>(g_config.specialActorMaxModelId);
    const bool autoRange =
        g_config.specialActorAutoDetectExtendedSlots &&
        modelId <= static_cast<uint32_t>(g_config.specialActorAutoMaxModelId);

    if (!configuredRange && !autoRange) {
        const long count = InterlockedIncrement(&g_specialActorSlotRangeLogs);
        if (count <= 64) {
            Log("special actor bridge: rejected slot model=%u outside configured=[%d,%d] autoMax=%d autoDetect=%d",
                modelId,
                g_config.specialActorMinModelId,
                g_config.specialActorMaxModelId,
                g_config.specialActorAutoMaxModelId,
                g_config.specialActorAutoDetectExtendedSlots ? 1 : 0);
        }
        return false;
    }

    const uintptr_t streamingEntry = SafeStreamingInfoEntryAddress(modelId);
    if (!streamingEntry) {
        const long count = InterlockedIncrement(&g_specialActorSlotRangeLogs);
        if (count <= 64) {
            Log("special actor bridge: rejected slot model=%u missing streamingEntry", modelId);
        }
        return false;
    }

    const uintptr_t modelEntry = SafeModelInfoEntryAddress(modelId);
    uint32_t modelInfo = 0;
    if (g_config.specialActorRequireModelInfoSlot &&
        (!modelEntry || !SafeReadU32(modelEntry, &modelInfo) || !modelInfo)) {
        const long count = InterlockedIncrement(&g_specialActorSlotRangeLogs);
        if (count <= 64) {
            Log("special actor bridge: rejected slot model=%u modelEntry=0x%08X modelInfo=0x%08X requireModelInfo=1",
                modelId, modelEntry, modelInfo);
        }
        return false;
    }

    return true;
}

bool ReadConfiguredSpecialActorName(uint32_t modelId, uint32_t actorCode, char* out, size_t outSize)
{
    if (!out || outSize == 0) {
        return false;
    }

    out[0] = '\0';
    char key[64]{};
    if (actorCode > 0) {
        sprintf_s(key, "SpecialActorName%03u", actorCode);
        if (ReadSmallTextValue(g_configPath, key, out, outSize) && out[0]) {
            return true;
        }

        const char* builtInCatalogName = GetBuiltInSpecialActorCatalogName(actorCode);
        if (builtInCatalogName) {
            strncpy_s(out, outSize, builtInCatalogName, _TRUNCATE);
            return true;
        }

        const char* runtimeCatalogName = GetRuntimeSpecialActorCatalogName(actorCode);
        if (runtimeCatalogName) {
            strncpy_s(out, outSize, runtimeCatalogName, _TRUNCATE);
            return true;
        }

        return false;
    }

    sprintf_s(key, "SpecialActor%u", modelId);
    if (ReadSmallTextValue(g_configPath, key, out, outSize) && out[0]) {
        return true;
    }

    const char* builtIn = GetBuiltInSpecialActorName(modelId);
    if (!builtIn) {
        return false;
    }

    strncpy_s(out, outSize, builtIn, _TRUNCATE);
    return true;
}

bool RequestSpecialActorName(uint32_t modelId, const char* name)
{
    if (!name || !name[0]) {
        return false;
    }

    if (!IsSpecialActorSlotUsable(modelId)) {
        Log("special actor bridge: missing model/streaming entry model=%u name=%s", modelId, name);
        return false;
    }

    using RequestSpecialModelFn = void(__cdecl*)(int, const char*, int);
    auto requestSpecialModel = reinterpret_cast<RequestSpecialModelFn>(kCStreamingRequestSpecialModel);
    constexpr int kStreamingMissionKeepPriority = 0x04 | 0x08 | 0x10;

    __try {
        requestSpecialModel(static_cast<int>(modelId), name, kStreamingMissionKeepPriority);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("special actor bridge: RequestSpecialModel exception model=%u name=%s", modelId, name);
        return false;
    }
}

bool RequestConfiguredSpecialActor(uint32_t modelId, uint32_t actorCode)
{
    if (actorCode > 0 && !g_runtimeSpecialActorNameCount) {
        BuildSpecialActorRuntimeCatalog();
    }

    char name[64]{};
    if (!ReadConfiguredSpecialActorName(modelId, actorCode, name, sizeof(name))) {
        Log("special actor bridge: no configured name for model=%u actorCode=%u; add SpecialActor%u = NAME or SpecialActorName%03u = NAME",
            modelId, actorCode, modelId, actorCode);
        return false;
    }

    return RequestSpecialActorName(modelId, name);
}

bool ReleaseConfiguredSpecialActor(uint32_t modelId)
{
    if (!SafeStreamingInfoEntryAddress(modelId)) {
        return false;
    }

    using SetMissionDoesntRequireModelFn = void(__cdecl*)(int);
    auto setMissionDoesntRequireModel = reinterpret_cast<SetMissionDoesntRequireModelFn>(kCStreamingSetMissionDoesntRequireModel);

    __try {
        setMissionDoesntRequireModel(static_cast<int>(modelId));
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("special actor bridge: SetMissionDoesntRequireModel exception model=%u", modelId);
        return false;
    }
}

